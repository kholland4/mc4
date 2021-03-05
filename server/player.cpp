#include "player.h"

PlayerState::PlayerState(connection_hdl hdl)
    : auth(false), m_connection_hdl(hdl), m_tag(boost::uuids::random_generator()()), m_name(get_tag())
{
  
}

std::string PlayerState::pos_as_json() {
  std::ostringstream out;
  
  out << "{\"type\":\"set_player_pos\","
      <<  "\"pos\":{\"x\":" << pos.x << ",\"y\":" << pos.y << ",\"z\":" << pos.z << "},"
      <<  "\"vel\":{\"x\":" << vel.x << ",\"y\":" << vel.y << ",\"z\":" << vel.z << "},"
      <<  "\"rot\":{\"x\":" << rot.x << ",\"y\":" << rot.y << ",\"z\":" << rot.z << ",\"w\":" << rot.w << "}}";
  
  return out.str();
}

std::string PlayerState::entity_data_as_json() {
  std::ostringstream out;
  
  out << "{\"id\":\"" << json_escape(get_tag()) << "\","
      <<  "\"pos\":{\"x\":" << pos.x << ",\"y\":" << pos.y << ",\"z\":" << pos.z << "},"
      <<  "\"vel\":{\"x\":" << vel.x << ",\"y\":" << vel.y << ",\"z\":" << vel.z << "},"
      <<  "\"rot\":{\"x\":" << rot.x << ",\"y\":" << rot.y << ",\"z\":" << rot.z << ",\"w\":" << rot.w << "}}";
  
  return out.str();
}

bool PlayerState::needs_mapblock_update(MapblockUpdateInfo info) {
  auto search = known_mapblocks.find(info.pos);
  if(search != known_mapblocks.end()) {
    MapblockUpdateInfo curr_info = search->second;
    return info != curr_info;
  }
  return true;
}

void PlayerState::send_mapblock(Mapblock *mb, WsServer& sender) {
  known_mapblocks[mb->pos] = MapblockUpdateInfo(mb);
  
  sender.send(m_connection_hdl, mb->as_json(), websocketpp::frame::opcode::text);
}

void PlayerState::prepare_mapblocks(std::vector<Vector3<int>> mapblock_list, Map& map) {
  //Batch update light.
  std::set<Vector3<int>> mb_need_light;
  for(size_t i = 0; i < mapblock_list.size(); i++) {
    Vector3<int> mb_pos = mapblock_list[i];
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
void PlayerState::prepare_nearby_mapblocks(int mb_radius, int mb_radius_outer, Map& map) {
  Vector3<int> mb_pos(
      (int)std::round(pos.x / MAPBLOCK_SIZE_X),
      (int)std::round(pos.y / MAPBLOCK_SIZE_Y),
      (int)std::round(pos.z / MAPBLOCK_SIZE_Z));
  Vector3<int> min_inner(mb_pos.x - mb_radius, mb_pos.y - mb_radius, mb_pos.z - mb_radius);
  Vector3<int> max_inner(mb_pos.x + mb_radius, mb_pos.y + mb_radius, mb_pos.z + mb_radius);
  Vector3<int> min_outer(mb_pos.x - mb_radius_outer, mb_pos.y - mb_radius_outer, mb_pos.z - mb_radius_outer);
  Vector3<int> max_outer(mb_pos.x + mb_radius_outer, mb_pos.y + mb_radius_outer, mb_pos.z + mb_radius_outer);
  
  std::vector<Vector3<int>> mb_to_update;
  for(int x = min_outer.x; x <= max_outer.x; x++) {
    for(int y = min_outer.y; y <= max_outer.y; y++) {
      for(int z = min_outer.z; z <= max_outer.z; z++) {
        if(((x >= min_inner.x && x <= max_inner.x) && (y >= min_inner.y && y <= max_inner.y)) ||
           ((y >= min_inner.y && y <= max_inner.y) && (z >= min_inner.z && z <= max_inner.z)) ||
           ((x >= min_inner.x && x <= max_inner.x) && (z >= min_inner.z && z <= max_inner.z))) {
          mb_to_update.push_back(Vector3<int>(x, y, z));
        }
      }
    }
  }
  
  prepare_mapblocks(mb_to_update, map);
}

void PlayerState::update_mapblocks(std::vector<Vector3<int>> mapblock_list, Map& map, WsServer& sender) {
  prepare_mapblocks(mapblock_list, map);
  
  for(size_t i = 0; i < mapblock_list.size(); i++) {
    Vector3<int> mb_pos = mapblock_list[i];
    MapblockUpdateInfo info = map.get_mapblockupdateinfo(mb_pos);
    if(needs_mapblock_update(info)) {
      Mapblock *mb = map.get_mapblock(mb_pos);
      send_mapblock(mb, sender);
      delete mb;
    }
  }
}



void PlayerState::update_nearby_mapblocks(int mb_radius, Map& map, WsServer& sender) {
  Vector3<int> abs_pos((int)pos.x, (int)pos.y, (int)pos.z);
  
  Vector3<int> mb_pos(
      (abs_pos.x < 0 && abs_pos.x % MAPBLOCK_SIZE_X != 0) ? (abs_pos.x / MAPBLOCK_SIZE_X - 1) : (abs_pos.x / MAPBLOCK_SIZE_X),
      (abs_pos.y < 0 && abs_pos.y % MAPBLOCK_SIZE_Y != 0) ? (abs_pos.y / MAPBLOCK_SIZE_Y - 1) : (abs_pos.y / MAPBLOCK_SIZE_Y),
      (abs_pos.z < 0 && abs_pos.z % MAPBLOCK_SIZE_Z != 0) ? (abs_pos.z / MAPBLOCK_SIZE_Z - 1) : (abs_pos.z / MAPBLOCK_SIZE_Z));
  Vector3<int> min_pos(mb_pos.x - mb_radius, mb_pos.y - mb_radius, mb_pos.z - mb_radius);
  Vector3<int> max_pos(mb_pos.x + mb_radius, mb_pos.y + mb_radius, mb_pos.z + mb_radius);
  
  std::vector<Vector3<int>> mb_to_update;
  for(int x = min_pos.x; x <= max_pos.x; x++) {
    for(int y = min_pos.y; y <= max_pos.y; y++) {
      for(int z = min_pos.z; z <= max_pos.z; z++) {
        mb_to_update.push_back(Vector3<int>(x, y, z));
      }
    }
  }
  update_mapblocks(mb_to_update, map, sender);
}
void PlayerState::update_nearby_known_mapblocks(int mb_radius, Map& map, WsServer& sender) {
  Vector3<int> abs_pos((int)pos.x, (int)pos.y, (int)pos.z);
  
  Vector3<int> mb_pos(
      (abs_pos.x < 0 && abs_pos.x % MAPBLOCK_SIZE_X != 0) ? (abs_pos.x / MAPBLOCK_SIZE_X - 1) : (abs_pos.x / MAPBLOCK_SIZE_X),
      (abs_pos.y < 0 && abs_pos.y % MAPBLOCK_SIZE_Y != 0) ? (abs_pos.y / MAPBLOCK_SIZE_Y - 1) : (abs_pos.y / MAPBLOCK_SIZE_Y),
      (abs_pos.z < 0 && abs_pos.z % MAPBLOCK_SIZE_Z != 0) ? (abs_pos.z / MAPBLOCK_SIZE_Z - 1) : (abs_pos.z / MAPBLOCK_SIZE_Z));
  Vector3<int> min_pos(mb_pos.x - mb_radius, mb_pos.y - mb_radius, mb_pos.z - mb_radius);
  Vector3<int> max_pos(mb_pos.x + mb_radius, mb_pos.y + mb_radius, mb_pos.z + mb_radius);
  
  std::vector<Vector3<int>> mb_to_update;
  for(int x = min_pos.x; x <= max_pos.x; x++) {
    for(int y = min_pos.y; y <= max_pos.y; y++) {
      for(int z = min_pos.z; z <= max_pos.z; z++) {
        Vector3<int> where(x, y, z);
        auto search = known_mapblocks.find(where);
        if(search != known_mapblocks.end()) {
          mb_to_update.push_back(where);
        }
      }
    }
  }
  update_mapblocks(mb_to_update, map, sender);
}
