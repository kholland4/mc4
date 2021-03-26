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

#include "player.h"

#include <cstring>

PlayerState::PlayerState(connection_hdl hdl)
    : auth(false), auth_guest(false), just_tp(false), m_connection_hdl(hdl), m_tag(boost::uuids::random_generator()()), m_name(get_tag())
{
  
}

std::string PlayerState::pos_as_json() {
  std::ostringstream out;
  
  out << "{\"type\":\"set_player_pos\","
      <<  "\"pos\":{\"x\":" << pos.x << ",\"y\":" << pos.y << ",\"z\":" << pos.z << ",\"w\":" << pos.w << ",\"world\":" << pos.world << ",\"universe\":" << pos.universe << "},"
      <<  "\"vel\":{\"x\":" << vel.x << ",\"y\":" << vel.y << ",\"z\":" << vel.z << ",\"w\":" << vel.w << ",\"world\":" << vel.world << ",\"universe\":" << vel.universe << "},"
      <<  "\"rot\":{\"x\":" << rot.x << ",\"y\":" << rot.y << ",\"z\":" << rot.z << ",\"w\":" << rot.w << "}}";
  
  return out.str();
}

std::string PlayerState::entity_data_as_json() {
  std::ostringstream out;
  
  out << "{\"id\":\"" << json_escape(get_tag()) << "\","
      <<  "\"pos\":{\"x\":" << pos.x << ",\"y\":" << pos.y << ",\"z\":" << pos.z << ",\"w\":" << pos.w << ",\"world\":" << pos.world << ",\"universe\":" << pos.universe << "},"
      <<  "\"vel\":{\"x\":" << vel.x << ",\"y\":" << vel.y << ",\"z\":" << vel.z << ",\"w\":" << vel.w << ",\"world\":" << vel.world << ",\"universe\":" << vel.universe << "},"
      <<  "\"rot\":{\"x\":" << rot.x << ",\"y\":" << rot.y << ",\"z\":" << rot.z << ",\"w\":" << rot.w << "}}";
  
  return out.str();
}

std::string PlayerState::privs_as_json() {
  std::ostringstream out;
  
  out << "{\"type\":\"set_player_privs\",\"privs\":[";
  bool first = true;
  for(auto p : data.privs) {
    if(!first) { out << ","; }
    first = false;
    out << "\"" + json_escape(p) + "\"";
  }
  out << "]}";
  return out.str();
}

void PlayerState::send_pos(WsServer& sender) {
  try {
    sender.send(m_connection_hdl, pos_as_json(), websocketpp::frame::opcode::text);
  } catch(std::exception const& e) {
    log(LogSource::PLAYER, LogLevel::ERR, "Socket send error");
  }
}
void PlayerState::send_privs(WsServer& sender) {
  try {
    sender.send(m_connection_hdl, privs_as_json(), websocketpp::frame::opcode::text);
  } catch(std::exception const& e) {
    log(LogSource::PLAYER, LogLevel::ERR, "Socket send error");
  }
}

bool PlayerState::needs_mapblock_update(MapblockUpdateInfo info) {
  auto search = known_mapblocks.find(info.pos);
  if(search != known_mapblocks.end()) {
    MapblockUpdateInfo curr_info = search->second;
    return info != curr_info;
  }
  return true;
}

unsigned int PlayerState::send_mapblock(Mapblock *mb, WsServer& sender) {
  known_mapblocks[mb->pos] = MapblockUpdateInfo(mb);
  
  //sender.send(m_connection_hdl, mb->as_json(), websocketpp::frame::opcode::text);
  
  uint32_t data_compressed[MAPBLOCK_SIZE_X * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z];
  
  uint32_t run_val = 0;
  size_t run_length = 0;
  size_t out_cursor = 0;
  
  uint16_t light_data_compressed[MAPBLOCK_SIZE_X * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z];
  
  uint16_t light_run_val = 0;
  size_t light_run_length = 0;
  size_t light_cursor = 0;
  
  for(int y = 0; y < MAPBLOCK_SIZE_Y; y++) {
    for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
      for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
        uint32_t val = mb->data[x][y][z] & 0b00000000011111111111111111111111; //exclude lighting information
        if(run_length == 0) {
          run_val = val;
          run_length++;
        } else if(val == run_val && run_length < 512) { //only 9 bits are used to store the run length, so 512 is the max possible.
          run_length++;
        } else {
          data_compressed[out_cursor] = run_val | ((run_length - 1) << 23);
          out_cursor++;
          run_length = 0;
          
          run_val = val;
          run_length++;
        }
        
        uint16_t light_val = (mb->data[x][y][z] >> 23) & 255;
        if(light_run_length == 0) {
          light_run_val = light_val;
          light_run_length++;
        } else if(light_val == light_run_val && light_run_length < 256) { //only 8 bits are used to store the run length, so 256 is the max possible.
          light_run_length++;
        } else {
          light_data_compressed[light_cursor] = light_run_val | ((light_run_length - 1) << 8);
          light_cursor++;
          light_run_length = 0;
          
          light_run_val = light_val;
          light_run_length++;
        }
      }
    }
  }
  if(run_length > 0) {
    data_compressed[out_cursor] = run_val | ((run_length - 1) << 23);
    out_cursor++;
    run_length = 0;
  }
  if(light_run_length > 0) {
    light_data_compressed[light_cursor] = light_run_val | ((light_run_length - 1) << 8);
    light_cursor++;
    light_run_length = 0;
  }
  
  std::ostringstream IDtoIS_ss;
  IDtoIS_ss << "[";
  for(size_t i = 0; i < mb->IDtoIS.size(); i++) {
    if(i != 0) { IDtoIS_ss << ","; }
    IDtoIS_ss << "\"" << mb->IDtoIS[i] << "\"";
  }
  IDtoIS_ss << "]";
  std::string IDtoIS_str = IDtoIS_ss.str();
  const uint8_t *IDtoIS_data = reinterpret_cast<const uint8_t*>(&IDtoIS_str[0]);
  size_t IDtoIS_len = IDtoIS_str.size();
  
  //Format:
  //0   Magic number (uint32_t)
  //4   Position (6 * int32_t) -- x, y, z, w, world, universe
  //28  updateNum (uint32_t)
  //32  lightUpdateNum (uint32_t)
  //36  lightNeedsUpdate (uint32_t)
  //40  flags (uint32_t) -- lowest bit is 'sunlit', others are reserved
  //44  data len (uint32_t)
  //48  data (string of uint32_ts)
  //?   light len (# of uint32_ts) (uint32_t)
  //+4   light len (# of uint16_ts) (uint32_t)
  //+8  light data (string of uint16_ts)
  //?   IDtoIS len (uint32_t)
  //+4  IDtoIS data (utf-8 chars)
  
  uint8_t out_buf[sizeof(uint32_t) +
                  sizeof(int32_t) * 6 +
                  sizeof(uint32_t) +
                  sizeof(uint32_t) +
                  sizeof(uint32_t) +
                  sizeof(uint32_t) +
                  sizeof(uint32_t) +
                  MAPBLOCK_SIZE_X * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z * sizeof(uint32_t) + 4 +
                  sizeof(uint32_t) +
                  sizeof(uint32_t) +
                  MAPBLOCK_SIZE_X * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z * sizeof(uint16_t) + 4 +
                  sizeof(uint32_t) +
                  IDtoIS_len + 4] = {0};
  
  uint32_t *out_buf_32 = (uint32_t*) out_buf;
  int32_t *out_buf_i32 = (int32_t*) out_buf;
  
  out_buf_32[0] = 0xABCD5678;
  out_buf_i32[1] = mb->pos.x;
  out_buf_i32[2] = mb->pos.y;
  out_buf_i32[3] = mb->pos.z;
  out_buf_i32[4] = mb->pos.w;
  out_buf_i32[5] = mb->pos.world;
  out_buf_i32[6] = mb->pos.universe;
  out_buf_32[7] = mb->update_num;
  out_buf_32[8] = mb->light_update_num;
  out_buf_32[9] = mb->light_needs_update;
  out_buf_32[10] = mb->sunlit ? 1 : 0;
  out_buf_32[11] = out_cursor;
  size_t arr_pos = 12;
  memcpy(out_buf_32 + arr_pos, data_compressed, out_cursor * sizeof(uint32_t));
  arr_pos += out_cursor;
  out_buf_32[arr_pos] = (light_cursor * sizeof(uint16_t) / sizeof(uint32_t)) + 1;
  arr_pos++;
  out_buf_32[arr_pos] = light_cursor;
  arr_pos++;
  memcpy(out_buf_32 + arr_pos, light_data_compressed, light_cursor * sizeof(uint16_t));
  arr_pos += (light_cursor * sizeof(uint16_t) / sizeof(uint32_t)) + 1;
  out_buf_32[arr_pos] = IDtoIS_len;
  arr_pos++;
  memcpy(out_buf_32 + arr_pos, IDtoIS_data, IDtoIS_len);
  arr_pos += (IDtoIS_len / sizeof(uint32_t)) + 1;
  
  try {
    sender.send(m_connection_hdl, out_buf, arr_pos * sizeof(uint32_t), websocketpp::frame::opcode::binary);
  } catch(std::exception const& e) {
    log(LogSource::PLAYER, LogLevel::ERR, "Socket send error");
  }
  
  return arr_pos * sizeof(uint32_t);
}

void PlayerState::prepare_mapblocks(std::vector<MapPos<int>> mapblock_list, Map& map) {
  //Batch update light.
  std::set<MapPos<int>> mb_need_light;
  for(size_t i = 0; i < mapblock_list.size(); i++) {
    MapPos<int> mb_pos = mapblock_list[i];
    MapblockUpdateInfo info = map.get_mapblockupdateinfo(mb_pos);
    if(info.light_needs_update == 1) { //TODO what about 2?
      mb_need_light.insert(mb_pos);
    }
  }
  if(mb_need_light.size() > 0) {
#ifdef DEBUG_PERF
    auto start = std::chrono::steady_clock::now();
#endif
    map.update_mapblock_light(mb_need_light);
#ifdef DEBUG_PERF
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    double ms = std::chrono::duration<double, std::milli>(diff).count();
    std::cout << "prepped " << mb_need_light.size() << " mapblocks for " << get_name() << " in " << ms << " ms" << std::endl;
#endif
  }
}

//mb_radius indicates the radius of a cube of mapblocks to prepare (i. e. mb_radius=2 yields a 5x5x5 cube centered around the player's current mapblock)
//mb_radius_outer indicates the radius of a sort of 3-dimensional plus shape extending outward from this cube
//  (if at least two dimensions are within the bounds of the inner cube, the mapblocks will be loaded)
//  this is used because mapblock rendering on the client requires access all 6 mapblocks immediately adjacent to the one being rendered
void PlayerState::prepare_nearby_mapblocks(int mb_radius, int mb_radius_outer, int mb_radius_w, Map& map) {
  MapPos<int> mb_pos = containing_mapblock();
  MapPos<int> min_inner(mb_pos.x - mb_radius, mb_pos.y - mb_radius, mb_pos.z - mb_radius, mb_pos.w - mb_radius_w, mb_pos.world, mb_pos.universe);
  MapPos<int> max_inner(mb_pos.x + mb_radius, mb_pos.y + mb_radius, mb_pos.z + mb_radius, mb_pos.w + mb_radius_w, mb_pos.world, mb_pos.universe);
  MapPos<int> min_outer(mb_pos.x - mb_radius_outer, mb_pos.y - mb_radius_outer, mb_pos.z - mb_radius_outer, mb_pos.w - mb_radius_w, mb_pos.world, mb_pos.universe);
  MapPos<int> max_outer(mb_pos.x + mb_radius_outer, mb_pos.y + mb_radius_outer, mb_pos.z + mb_radius_outer, mb_pos.w + mb_radius_w, mb_pos.world, mb_pos.universe);
  
  std::vector<MapPos<int>> mb_to_update;
  for(int universe = min_outer.universe; universe <= max_outer.universe; universe++) {
    for(int world = min_outer.world; world <= max_outer.world; world++) {
      for(int w = min_outer.w; w <= max_outer.w; w++) {
        for(int x = min_outer.x; x <= max_outer.x; x++) {
          for(int y = min_outer.y; y <= max_outer.y; y++) {
            for(int z = min_outer.z; z <= max_outer.z; z++) {
              if(((x >= min_inner.x && x <= max_inner.x) && (y >= min_inner.y && y <= max_inner.y)) ||
                 ((y >= min_inner.y && y <= max_inner.y) && (z >= min_inner.z && z <= max_inner.z)) ||
                 ((x >= min_inner.x && x <= max_inner.x) && (z >= min_inner.z && z <= max_inner.z))) {
                mb_to_update.push_back(MapPos<int>(x, y, z, w, world, universe));
              }
            }
          }
        }
      }
    }
  }
  
  prepare_mapblocks(mb_to_update, map);
}

void PlayerState::update_mapblocks(std::vector<MapPos<int>> mapblock_list, Map& map, WsServer& sender) {
  prepare_mapblocks(mapblock_list, map);
  
  for(size_t i = 0; i < mapblock_list.size(); i++) {
    MapPos<int> mb_pos = mapblock_list[i];
    MapblockUpdateInfo info = map.get_mapblockupdateinfo(mb_pos);
    if(needs_mapblock_update(info)) {
      Mapblock *mb = map.get_mapblock(mb_pos);
      send_mapblock(mb, sender);
      delete mb;
    }
  }
}



void PlayerState::update_nearby_mapblocks(int mb_radius, int mb_radius_w, Map& map, WsServer& sender) {
  MapPos<int> mb_pos = containing_mapblock();
  MapPos<int> min_pos(mb_pos.x - mb_radius, mb_pos.y - mb_radius, mb_pos.z - mb_radius, mb_pos.w - mb_radius_w, mb_pos.world, mb_pos.universe);
  MapPos<int> max_pos(mb_pos.x + mb_radius, mb_pos.y + mb_radius, mb_pos.z + mb_radius, mb_pos.w + mb_radius_w, mb_pos.world, mb_pos.universe);
  
  std::vector<MapPos<int>> mb_to_update;
  for(int universe = min_pos.universe; universe <= max_pos.universe; universe++) {
    for(int world = min_pos.world; world <= max_pos.world; world++) {
      for(int w = min_pos.w; w <= max_pos.w; w++) {
        for(int x = min_pos.x; x <= max_pos.x; x++) {
          for(int y = min_pos.y; y <= max_pos.y; y++) {
            for(int z = min_pos.z; z <= max_pos.z; z++) {
              mb_to_update.push_back(MapPos<int>(x, y, z, w, world, universe));
            }
          }
        }
      }
    }
  }
  update_mapblocks(mb_to_update, map, sender);
}
std::vector<MapPos<int>> PlayerState::list_nearby_known_mapblocks(int mb_radius, int mb_radius_w) {
  MapPos<int> mb_pos = containing_mapblock();
  MapPos<int> min_pos(mb_pos.x - mb_radius, mb_pos.y - mb_radius, mb_pos.z - mb_radius, mb_pos.w - mb_radius_w, mb_pos.world, mb_pos.universe);
  MapPos<int> max_pos(mb_pos.x + mb_radius, mb_pos.y + mb_radius, mb_pos.z + mb_radius, mb_pos.w + mb_radius_w, mb_pos.world, mb_pos.universe);
  
  std::vector<MapPos<int>> mb_to_update;
  
  for(int universe = min_pos.universe; universe <= max_pos.universe; universe++) {
    for(int world = min_pos.world; world <= max_pos.world; world++) {
      for(int w = min_pos.w; w <= max_pos.w; w++) {
        for(int x = min_pos.x; x <= max_pos.x; x++) {
          for(int y = min_pos.y; y <= max_pos.y; y++) {
            for(int z = min_pos.z; z <= max_pos.z; z++) {
              MapPos<int> where(x, y, z, w, world, universe);
              auto search = known_mapblocks.find(where);
              if(search != known_mapblocks.end()) {
                mb_to_update.push_back(where);
              }
            }
          }
        }
      }
    }
  }
  
  return mb_to_update;
}
void PlayerState::update_nearby_known_mapblocks(int mb_radius, int mb_radius_w, Map& map, WsServer& sender) {
  std::vector<MapPos<int>> mb_to_update = list_nearby_known_mapblocks(mb_radius, mb_radius_w);
  update_mapblocks(mb_to_update, map, sender);
}
void PlayerState::update_nearby_known_mapblocks(std::vector<MapPos<int>> mb_to_update, Map& map, WsServer& sender) {
  update_mapblocks(mb_to_update, map, sender);
}


MapPos<int> PlayerState::containing_mapblock() {
  return MapPos<int>(
      (int)std::round(pos.x / MAPBLOCK_SIZE_X),
      (int)std::round(pos.y / MAPBLOCK_SIZE_Y),
      (int)std::round(pos.z / MAPBLOCK_SIZE_Z),
      pos.w, pos.world, pos.universe);
}
