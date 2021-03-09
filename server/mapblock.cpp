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

#include "mapblock.h"
#include "json.h"
#include "log.h"

#include <sstream>
//#include <simdjson.h>

Mapblock::Mapblock(MapPos<int> _pos)
    : pos(_pos), update_num(0), light_update_num(0), light_needs_update(1), sunlit(true), is_nil(true), dirty(false), dont_write_to_db(false), data {}
{
  IDtoIS.push_back("air");
  IStoID["air"] = 0;
}

//Data for every node in a mapblock is stored as a uint32_t array.
//The bottom 31 bits are defined as follows, with the rest as of yet unused:
//| light||  rot ||           id|
//322222222221111111111
//0987654321098765432109876543210

//|         light|
//|   sun|| other|
// 7 6 5 4 3 2 1 0

Node Mapblock::get_node_rel(MapPos<int> rel_pos) {
  //a safety check to make sure all the new code isn't doing anything silly
  if(rel_pos.w != 0 || rel_pos.world != 0 || rel_pos.universe != 0) {
    log(LogSource::MAP, LogLevel::WARNING, "cannot get data outside of this mapblock!");
    return Node();
  }
  
  uint32_t val = data[rel_pos.x][rel_pos.y][rel_pos.z];
  unsigned int id = val & 32767;
  unsigned int rot = (val >> 15) & 255;
  return Node(id_to_itemstring(id), rot);
}

void Mapblock::set_node_rel(MapPos<int> rel_pos, Node node) {
  //a safety check to make sure all the new code isn't doing anything silly
  if(rel_pos.w != 0 || rel_pos.world != 0 || rel_pos.universe != 0) {
    log(LogSource::MAP, LogLevel::WARNING, "cannot set data outside of this mapblock!");
  }
  
  unsigned int id = itemstring_to_id(node.itemstring) & 32767;
  unsigned int rot = node.rot & 255;
  uint32_t old_val = data[rel_pos.x][rel_pos.y][rel_pos.z];
  unsigned int light = (old_val >> 23) & 255;
  uint32_t val = (light << 23) | (rot << 15) | id;
  data[rel_pos.x][rel_pos.y][rel_pos.z] = val;
  
  is_nil = false;
  dirty = true;
  if(node.itemstring != "air") {
    sunlit = false;
  }
}

//Gets a node id (for data storage) given an itemstring, creating one if necessary.
unsigned int Mapblock::itemstring_to_id(std::string itemstring) {
  auto search = IStoID.find(itemstring);
  if(search != IStoID.end()) {
    return search->second;
  } else {
    unsigned int id = IDtoIS.size();
    IDtoIS.push_back(itemstring);
    IStoID[itemstring] = id;
    return id;
  }
}

//Gets the itemstring associated with a node id, empty string if not found
std::string Mapblock::id_to_itemstring(unsigned int id) {
  if(id < IDtoIS.size()) {
    return IDtoIS[id];
  }
  return std::string();
}

std::string Mapblock::as_json() {
  /*boost::property_tree::ptree pt;
  
  pt.put("data.pos.x", pos.x);
  pt.put("data.pos.y", pos.y);
  pt.put("data.pos.z", pos.z);
  
  pt.put("data.updateNum", update_num);
  pt.put("data.lightUpdateNum", light_update_num);
  pt.put("data.lightNeedsUpdate", light_needs_update);
  pt.put("data.props.sunlit", sunlit);
  
  for(auto it : IStoID) {
    pt.put("data.IStoID." + it->first, it->second); //FIXME data sanitization
  }
  
  boost::property_tree::ptree id_list;
  for(unsigned int i = 0; i < IDtoIS.size(); i++) {
    id_list.
  }
  
  std::ostringstream out;
  boost::property_tree::write_json(out, pt, false);
  return out.str();*/
  
  std::ostringstream out;
  
  out << "{\"type\":\"req_mapblock\",\"data\":{"
      << "\"pos\":{\"x\":" << pos.x << ",\"y\":" << pos.y << ",\"z\":" << pos.z << ",\"w\":" << pos.w << ",\"world\":" << pos.world << ",\"universe\":" << pos.universe << "},"
      << "\"updateNum\":" << update_num << ","
      << "\"lightUpdateNum\":" << light_update_num << ","
      << "\"lightNeedsUpdate\":" << light_needs_update << ","
      << "\"props\":{\"sunlit\":" << sunlit << "},";
  
  bool first = true;
  out << "\"IStoID\":{";
  for(auto it : IStoID) {
    if(!first) {
      out << ",";
    }
    first = false;
    
    out << "\"" << json_escape(it.first) << "\":" << it.second;
  }
  out << "},";
  
  out << "\"IDtoIS\":[";
  for(unsigned int i = 0; i < IDtoIS.size(); i++) {
    if(i > 0) {
      out << ",";
    }
    
    out << "\"" << json_escape(IDtoIS[i]) << "\"";
  }
  out << "],";
  
  out << "\"data\":[";
  for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
    if(x > 0) { out << ","; }
    out << "[";
    for(int y = 0; y < MAPBLOCK_SIZE_Y; y++) {
      if(y > 0) { out << ","; }
      out << "[";
      for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
        if(z > 0) { out << ","; }
        out << data[x][y][z];
      }
      out << "]";
    }
    out << "]";
  }
  out << "]";
  
  out << "}}";
  
  return out.str();
}
