/*
    mc4 server
    Copyright (C) 2021 kholland4

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "mapgen.h"
#include "log.h"

#include <cmath>
#include <math.h>

#define PI 3.14159265

#define TREE_ITERATION_DEPTH 3

//Recommended parameters are 0 for water depth and 3 for sand depth.
#define MAPGENALPHA_WATER_DEPTH -16
#define MAPGENALPHA_SAND_DEPTH -15

//L-system trees [https://en.wikipedia.org/wiki/L-system] [https://dev.minetest.net/Introduction_to_L-system_trees]
//Defined characters are as follows, same as Minetest:
//  G  move forward 1 unit
//
//  +  yaw right by angle
//  -  yaw left
//  &  pitch down
//  ^  pitch up
//  /  roll right
//  *  roll left
//
//  [  push state to stack
//  ]  pop state from stack
//
//Other characters can be defined as rules or placement rules.
//Placement rules place a given itemstring then move forward one.

class TreePlaceRule {
  public:
    TreePlaceRule() : trunk(""), leaves(""), leaf_amount(Vector3<double>(0, 0, 0)), leaf_fuzz(Vector3<double>(0, 0, 0)) {}
    TreePlaceRule(std::string _trunk) : trunk(_trunk), leaves(""), leaf_amount(Vector3<double>(0, 0, 0)), leaf_fuzz(Vector3<double>(0, 0, 0)) {}
    TreePlaceRule(std::string _trunk, std::string _leaves, Vector3<double> _leaf_amount) : trunk(_trunk), leaves(_leaves), leaf_amount(_leaf_amount), leaf_fuzz(Vector3<double>(2, 2, 2)) {}
    TreePlaceRule(std::string _trunk, std::string _leaves, Vector3<double> _leaf_amount, Vector3<double> _leaf_fuzz) : trunk(_trunk), leaves(_leaves), leaf_amount(_leaf_amount), leaf_fuzz(_leaf_fuzz) {}
    
    std::string trunk;
    std::string leaves;
    Vector3<double> leaf_amount;
    Vector3<double> leaf_fuzz;
};

class TreeDef {
  public:
    TreeDef(std::string _start, std::map<char, std::pair<double, std::string>> _rules, std::map<char, TreePlaceRule> _place_rules, int _angle)
        : start(_start), rules(_rules), place_rules(_place_rules), angle(_angle)
    {}
    
    std::string expand(unsigned int depth, uint32_t seed) {
      std::mt19937 rng(seed);
      std::uniform_real_distribution<double> rng_dist;
      
      std::string state = start;
      for(unsigned int i = 0; i < depth; i++) {
        std::string new_state;
        
        for(auto c : state) {
          auto search = rules.find(c);
          if(search != rules.end()) {
            double prob = search->second.first;
            if(prob == 1) {
              new_state += search->second.second;
            } else if(prob >= rng_dist(rng)) {
              new_state += search->second.second;
            }
          } else {
            new_state += c;
          }
        }
        
        state = new_state;
      }
      
      return state;
    }
    
    std::string start;
    std::map<char, std::pair<double, std::string>> rules;
    std::map<char, TreePlaceRule> place_rules;
    int angle;
};

class TreeGenState {
  public:
    TreeGenState(double _x, double _y, double _z) : roll(0), pitch(90), yaw(0), x(_x), y(_y), z(_z) {}
    
    int roll;
    int pitch;
    int yaw;
    double x;
    double y;
    double z;
};

class TreeGenerator {
  public:
    TreeGenerator(MapPos<int> _root_pos, uint32_t _seed, TreeDef _def)
        : root_pos(_root_pos), seed(_seed), def(_def)
    {
      actions = def.expand(TREE_ITERATION_DEPTH, seed);
    }
    
    void write_to_mapblocks(std::map<MapPos<int>, Mapblock*>& mapblocks) {
      TreeGenState s(root_pos.x, root_pos.y, root_pos.z);
      
      std::mt19937 rng(seed);
      std::uniform_real_distribution<double> rng_dist;
      
      std::vector<TreeGenState> stack;
      
      for(auto c : actions) {
        bool do_move = false;
        TreePlaceRule to_place;
        
        switch(c) {
          case '[':
            stack.push_back(s);
            break;
          case ']':
            if(stack.size() == 0) {
              log(LogSource::MAPGEN, LogLevel::WARNING, "stack underflow in tree generation, rule is " + actions);
            } else {
              s = stack[stack.size() - 1];
              stack.pop_back();
            }
            break;
          case '+':
            s.yaw = (((s.yaw + def.angle) % 360) + 360) % 360;
            break;
          case '-':
            s.yaw = (((s.yaw - def.angle) % 360) + 360) % 360;
            break;
          case '&':
            s.pitch = (((s.pitch - def.angle) % 360) + 360) % 360;
            break;
          case '^':
            s.pitch = (((s.pitch + def.angle) % 360) + 360) % 360;
            break;
          case '/':
            s.roll = (((s.roll + def.angle) % 360) + 360) % 360;
            break;
          case '*':
            s.roll = (((s.roll - def.angle) % 360) + 360) % 360;
            break;
          case 'G':
            do_move = true;
            break;
          default:
            auto search = def.place_rules.find(c);
            if(search == def.place_rules.end()) {
              //could be an unexpanded rule
              auto search_r = def.rules.find(c);
              if(search_r == def.rules.end()) {
                log(LogSource::MAPGEN, LogLevel::WARNING, "undefined rule while generating tree: '" + std::string(1, c) + "'");
              }
              break;
            }
            to_place = search->second;
            do_move = true;
            break;
        }
        
        double old_x = s.x;
        double old_y = s.y;
        double old_z = s.z;
        
        double move_x = 0;
        double move_y = 0;
        double move_z = 0;
        
        if(do_move) {
          //https://learnopengl.com/Getting-started/Camera
          move_x = cos(s.yaw*PI/180) * cos(s.pitch*PI/180);
          move_y = sin(s.pitch*PI/180);
          move_z = sin(s.yaw*PI/180) * cos(s.pitch*PI/180);
          s.x += move_x;
          s.y += move_y;
          s.z += move_z;
        }
        
        if(to_place.trunk != "") {
          //place the requested node
          
          std::string air_is = "air";
          
          std::string trunk_is = to_place.trunk;
          std::string leaf_is = air_is;
          Vector3<double> leaf_amount(0, 0, 0);
          Vector3<double> leaf_fuzz(0, 0, 0);
          bool has_leaves = false;
          if(to_place.leaves != "") {
            leaf_is = to_place.leaves;
            leaf_amount = to_place.leaf_amount;
            leaf_fuzz = to_place.leaf_fuzz;
            has_leaves = true;
          }
          
          int new_x = round(s.x);
          int new_y = round(s.y);
          int new_z = round(s.z);
          
          for(double prop = 0; prop <= 1.0; prop += 0.5) {
            int place_x = round(old_x + move_x * prop);
            int place_y = round(old_y + move_y * prop);
            int place_z = round(old_z + move_z * prop);
            if(place_x == new_x && place_y == new_y && place_z == new_z) { continue; }
            
            for(auto it : mapblocks) {
              Mapblock *mb = it.second;
              MapPos<int> lower(it.first.x * MAPBLOCK_SIZE_X, it.first.y * MAPBLOCK_SIZE_Y, it.first.z * MAPBLOCK_SIZE_Z, it.first.w, it.first.world, it.first.universe);
              MapPos<int> upper(lower.x + MAPBLOCK_SIZE_X, lower.y + MAPBLOCK_SIZE_Y, lower.z + MAPBLOCK_SIZE_Z, lower.w, lower.world, lower.universe);
              if(place_x >= lower.x && place_x < upper.x &&
                 place_y >= lower.y && place_y < upper.y &&
                 place_z >= lower.z && place_z < upper.z) {
                unsigned int air_id = mb->itemstring_to_id(air_is);
                unsigned int trunk_id = mb->itemstring_to_id(trunk_is);
                unsigned int leaf_id = mb->itemstring_to_id(leaf_is);
                unsigned int old_id = mb->data[place_x - lower.x][place_y - lower.y][place_z - lower.z];
                std::string old_is = mb->id_to_itemstring(old_id);
                if(old_id == air_id || old_id == leaf_id || old_is.rfind("flowers:", 0) == 0) {
                  mb->data[place_x - lower.x][place_y - lower.y][place_z - lower.z] = trunk_id;
                }
                break;
              }
            }
            
            if(has_leaves) {
              Vector3<double> x_vec(cos(s.yaw*PI/180) * cos((s.pitch - 90)*PI/180),
                                    sin((s.pitch - 90)*PI/180),
                                    sin(s.yaw*PI/180) * cos((s.pitch - 90)*PI/180));
              Vector3<double> y_vec(cos(s.yaw*PI/180) * cos(s.pitch*PI/180),
                                    sin(s.pitch*PI/180),
                                    sin(s.yaw*PI/180) * cos(s.pitch*PI/180));
              Vector3<double> z_vec(cos((s.yaw + 90)*PI/180) * cos((s.pitch - 90)*PI/180),
                                    sin((s.pitch - 90)*PI/180),
                                    sin((s.yaw + 90)*PI/180) * cos((s.pitch - 90)*PI/180));
              
              double fuzz[6];
              for(int i = 0; i < 6; i += 3) {
                fuzz[i]     = rng_dist(rng) * leaf_fuzz.x * 2 - leaf_fuzz.x;
                fuzz[i + 1] = rng_dist(rng) * leaf_fuzz.y * 2 - leaf_fuzz.y;
                fuzz[i + 2] = rng_dist(rng) * leaf_fuzz.z * 2 - leaf_fuzz.z;
              }
              
              for(double x_delta = -leaf_amount.x - fuzz[0]; x_delta <= leaf_amount.x + fuzz[1]; x_delta += 0.5) {
                for(double y_delta = -leaf_amount.y - fuzz[2]; y_delta <= leaf_amount.y + fuzz[3]; y_delta += 0.5) {
                  for(double z_delta = -leaf_amount.z - fuzz[4]; z_delta <= leaf_amount.z + fuzz[5]; z_delta += 0.5) {
                    double x_fuzz = (rng_dist(rng) * 0.7) + 0.65;
                    double y_fuzz = (rng_dist(rng) * 0.7) + 0.65;
                    double z_fuzz = (rng_dist(rng) * 0.7) + 0.65;
                    int x = round(place_x + x_vec.x*x_delta*x_fuzz + y_vec.x*y_delta*y_fuzz + z_vec.x*z_delta*z_fuzz);
                    int y = round(place_y + x_vec.y*x_delta*x_fuzz + y_vec.y*y_delta*y_fuzz + z_vec.y*z_delta*z_fuzz);
                    int z = round(place_z + x_vec.z*x_delta*x_fuzz + y_vec.z*y_delta*y_fuzz + z_vec.z*z_delta*z_fuzz);
                    
                    for(auto it : mapblocks) {
                      Mapblock *mb = it.second;
                      MapPos<int> lower(it.first.x * MAPBLOCK_SIZE_X, it.first.y * MAPBLOCK_SIZE_Y, it.first.z * MAPBLOCK_SIZE_Z, it.first.w, it.first.world, it.first.universe);
                      MapPos<int> upper(lower.x + MAPBLOCK_SIZE_X, lower.y + MAPBLOCK_SIZE_Y, lower.z + MAPBLOCK_SIZE_Z, lower.w, lower.world, lower.universe);
                      if(x >= lower.x && x < upper.x &&
                         y >= lower.y && y < upper.y &&
                         z >= lower.z && z < upper.z) {
                        unsigned int air_id = mb->itemstring_to_id(air_is);
                        unsigned int leaf_id = mb->itemstring_to_id(leaf_is);
                        if(mb->data[x - lower.x][y - lower.y][z - lower.z] == air_id) {
                          mb->data[x - lower.x][y - lower.y][z - lower.z] = leaf_id;
                        }
                        break;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  
  private:
    MapPos<int> root_pos;
    uint32_t seed;
    TreeDef def;
    std::string actions;
};

TreeDef tree_regular("TTTT&A+B+A+A+B+A",
      {
        {'A', std::make_pair(0.5, "[LLL]")},
        {'B', std::make_pair(1, "[L]")}
      },
      {
        {'T', TreePlaceRule("default:tree")},
        {'L', TreePlaceRule("default:tree", "default:leaves", Vector3<double>(1.5, 0.5, 1.5), Vector3<double>(0.5, 0, 0.5))}
      }, 60);

TreeDef tree_aspen("TTTTT&LLL",
      {},
      {
        {'T', TreePlaceRule("default:aspen_tree")},
        {'L', TreePlaceRule("default:aspen_tree", "default:aspen_leaves", Vector3<double>(1.5, 0.5, 1.5), Vector3<double>(0.5, 0, 0.5))}
      }, 20);

TreeDef tree_pine("TTTTLTTMTTN",
      {},
      {
        {'T', TreePlaceRule("default:pine_tree")},
        {'L', TreePlaceRule("default:pine_tree", "default:pine_needles", Vector3<double>(3, 0.5, 3), Vector3<double>(1, 0.5, 1))},
        {'M', TreePlaceRule("default:pine_tree", "default:pine_needles", Vector3<double>(2, 0.5, 2), Vector3<double>(1, 0.5, 1))},
        {'N', TreePlaceRule("default:pine_tree", "default:pine_needles", Vector3<double>(1, 1, 1), Vector3<double>(0.5, 0, 0.5))}
      }, 20);

std::map<MapPos<int>, Mapblock*> MapgenAlpha::generate_near(MapPos<int> pos) {
  std::map<MapPos<int>, Mapblock*> to_generate;
  int start_y = pos.y - (((pos.y % 5) + 5) % 5);
  int end_y = start_y + 5;
  for(int y = start_y; y < end_y; y++) {
    MapPos<int> where(pos.x, y, pos.z, pos.w, pos.world, pos.universe);
    to_generate[where] = new Mapblock(where);
  }
  MapPos<int> global_offset = MapPos<int>(pos.x * MAPBLOCK_SIZE_X, start_y * MAPBLOCK_SIZE_Y, pos.z * MAPBLOCK_SIZE_Z, pos.w, pos.world, pos.universe);
  
  uint32_t seed = pos.x * 1234 + start_y * 7 + pos.z * 420 + pos.w * 24;
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> rng_dist;
  
  std::pair<std::string, double> flower_is_list[] = {
    {"flowers:grass_1", 0.10},
    {"flowers:grass_2", 0.10},
    {"flowers:grass_3", 0.15},
    {"flowers:grass_4", 0.08},
    {"flowers:grass_5", 0.05},
    {"flowers:chrysanthemum_green", 0.01},
    {"flowers:dandelion_white", 0.02},
    {"flowers:dandelion_yellow", 0.03},
    {"flowers:geranium", 0.03},
    {"flowers:mushroom_brown", 0.02},
    {"flowers:mushroom_red", 0.015},
    {"flowers:rose", 0.06},
    {"flowers:tulip", 0.04},
    {"flowers:tulip_black", 0.01},
    {"flowers:viola", 0.03}
  };
  
  double flower_total = 0;
  for(auto it : flower_is_list) {
    flower_total += it.second;
  }
  
  int height_map[MAPBLOCK_SIZE_X][MAPBLOCK_SIZE_Z];
  for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
    for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
      double val = perlin.accumulatedOctaveNoise3D((x + global_offset.x) / 500.0, (z + global_offset.z) / 500.0, global_offset.w / 300.0, 6);
      int height = std::floor(val * 50);
      height_map[x][z] = height;
    }
  }
  
  for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
    for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
      int height = height_map[x][z];
      for(int mb_y = start_y; mb_y < end_y; mb_y++) {
        MapPos<int> where(pos.x, mb_y, pos.z, pos.w, pos.world, pos.universe);
        if(height >= where.y * MAPBLOCK_SIZE_Y || where.y * MAPBLOCK_SIZE_Y < MAPGENALPHA_WATER_DEPTH) {
          Mapblock *mb = to_generate[where];
          mb->sunlit = false;
          
          unsigned int grass_id = mb->itemstring_to_id("default:grass");
          unsigned int dirt_id = mb->itemstring_to_id("default:dirt");
          unsigned int stone_id = mb->itemstring_to_id("default:stone");
          unsigned int sand_id = mb->itemstring_to_id("default:sand");
          unsigned int water_id = mb->itemstring_to_id("default:water_source");
          
          for(int y = 0; y < MAPBLOCK_SIZE_Y; y++) {
            int global_y = y + mb_y * MAPBLOCK_SIZE_Y;
            if(global_y > height) {
              if(global_y < MAPGENALPHA_WATER_DEPTH) {
                //water
                mb->data[x][y][z] = water_id;
              } else if(global_y == height + 1 && global_y > MAPGENALPHA_SAND_DEPTH) {
                //possibly flowers/grass, otherwise air
                double flowerNoise = perlin.noise3D((x + global_offset.x) / 2.0, (z + global_offset.z) / 2.0, global_offset.w / 2.0);
                if(flowerNoise > 0.4) {
                  double n = rng_dist(rng);
                  n *= flower_total;
                  
                  std::string flower_is = flower_is_list[0].first;
                  double acc = 0;
                  for(auto it : flower_is_list) {
                    acc += it.second;
                    if(acc >= n) {
                      flower_is = it.first;
                      break;
                    }
                  }
                  
                  mb->data[x][y][z] = mb->itemstring_to_id(flower_is);
                } else {
                  //air
                }
              } else {
                //air, which is already there
              }
            } else if(global_y == height) {
              if(global_y < MAPGENALPHA_SAND_DEPTH) {
                mb->data[x][y][z] = sand_id;
              } else {
                mb->data[x][y][z] = grass_id;
              }
            } else if(global_y >= height - 2) {
              if(global_y < MAPGENALPHA_SAND_DEPTH) {
                mb->data[x][y][z] = sand_id;
              } else {
                mb->data[x][y][z] = dirt_id;
              }
            } else {
              mb->data[x][y][z] = stone_id;
            }
          }
        }
      }
    }
  }
  
  if(end_y * MAPBLOCK_SIZE_Y > -100 && global_offset.y < 100) {
    //trees
    
    std::vector<MapPos<int>> tree_list;
    MapPos<int> max_tree_spread_down(12, 1, 12, 0, 0, 0);
    MapPos<int> max_tree_spread_up(12, 20, 12, 0, 0, 0);
    for(int w = global_offset.w - max_tree_spread_down.w; w <= global_offset.w + max_tree_spread_up.w; w++) {
      for(int x = global_offset.x - max_tree_spread_down.x; x < global_offset.x + MAPBLOCK_SIZE_X + max_tree_spread_up.x; x++) {
        for(int z = global_offset.z - max_tree_spread_down.z; z < global_offset.z + MAPBLOCK_SIZE_Z + max_tree_spread_up.z; z++) {
          int height;
          if(w == global_offset.w && x >= global_offset.x && x < global_offset.x + MAPBLOCK_SIZE_X && z >= global_offset.z && z < global_offset.z + MAPBLOCK_SIZE_Z) {
            height = height_map[x - global_offset.x][z - global_offset.z];
          } else {
            double val = perlin.accumulatedOctaveNoise3D(x / 500.0, z / 500.0, w / 300.0, 6);
            height = std::floor(val * 50);
          }
          
          //no trees below shoreline
          if(height <= MAPGENALPHA_SAND_DEPTH) { continue; }
          
          if(height < global_offset.y - max_tree_spread_up.y || height > end_y * MAPBLOCK_SIZE_Y + MAPBLOCK_SIZE_Y + max_tree_spread_down.y) { continue; }
          
          double treeNoise = perlin.noise3D(x / 1.5, z / 1.5, w / 1.5);
          if(treeNoise > 0.7) {
            tree_list.push_back(MapPos<int>(x, height + 1, z, w, global_offset.world, global_offset.universe));
          }
        }
      }
    }
    
    
    
    for(size_t i = 0; i < tree_list.size(); i++) {
      MapPos t = tree_list[i];
      if(t.w != global_offset.w) { continue; }
      
      double num = (t.x % 2345) * 312.32 + (t.z % 690) * 123.45 + (t.w % 53) * 234.67;
      double n = num - (long)num; //[0, 1)
      if(n < 0) { n += 1; }
      
      if(n < 0.3) {
        TreeGenerator gen(t, t.x * 3897 + t.z + t.w * 42, tree_aspen);
        gen.write_to_mapblocks(to_generate);
      } else if(n < 0.5) {
        TreeGenerator gen(t, t.x * 3897 + t.z + t.w * 42, tree_pine);
        gen.write_to_mapblocks(to_generate);
      } else {
        TreeGenerator gen(t, t.x * 3897 + t.z + t.w * 42, tree_regular);
        gen.write_to_mapblocks(to_generate);
      }
    }
  }
  
  for(auto it : to_generate) {
    it.second->is_nil = false;
  }
  
  return to_generate;
}
