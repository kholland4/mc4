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
#include <algorithm>

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
  std::ostringstream out;
  
  out << "{\"type\":\"req_mapblock\",\"data\":{"
      << "\"pos\":{\"x\":" << pos.x << ",\"y\":" << pos.y << ",\"z\":" << pos.z << ",\"w\":" << pos.w << ",\"world\":" << pos.world << ",\"universe\":" << pos.universe << "},"
      << "\"updateNum\":" << update_num << ","
      << "\"lightUpdateNum\":" << light_update_num << ","
      << "\"lightNeedsUpdate\":" << light_needs_update << ","
      << "\"props\":{\"sunlit\":" << sunlit << "},";
  
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



MapblockCompressed::MapblockCompressed(Mapblock *from)
    : pos(from->pos), update_num(from->update_num), light_update_num(from->light_update_num), light_needs_update(from->light_needs_update),
      IDtoIS(from->IDtoIS), sunlit(from->sunlit), is_nil(from->is_nil), dirty(from->dirty), dont_write_to_db(from->dont_write_to_db)
{
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
        uint32_t val = from->data[x][y][z] & 0b00000000011111111111111111111111; //exclude lighting information
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
        
        uint16_t light_val = (from->data[x][y][z] >> 23) & 255;
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
  
  data_c_len = out_cursor;
  data_c.reserve(data_c_len);
  std::copy(data_compressed, data_compressed + data_c_len, std::back_inserter(data_c));
  
  light_data_c_len = light_cursor;
  light_data_c.reserve(light_data_c_len);
  std::copy(light_data_compressed, light_data_compressed + light_data_c_len, std::back_inserter(light_data_c));
}

Mapblock* MapblockCompressed::decompress() {
  Mapblock *mb = new Mapblock(pos);
  mb->update_num = update_num;
  mb->light_update_num = light_update_num;
  mb->light_needs_update = light_needs_update;
  mb->IDtoIS = IDtoIS;
  for(size_t i = 0; i < IDtoIS.size(); i++) {
    mb->IStoID[IDtoIS[i]] = i;
  }
  mb->sunlit = sunlit;
  mb->is_nil = is_nil;
  mb->dirty = dirty;
  mb->dont_write_to_db = dont_write_to_db;
  
  //data (nodes)
  int y = 0;
  int x = 0;
  int z = 0;
  bool full = false;
  for(size_t i = 0; i < data_c_len; i++) {
    uint32_t run_val = data_c[i] & 0b00000000011111111111111111111111;
    size_t run_length = ((data_c[i] >> 23) & 511) + 1; //Run lengths are offset by 1 to allow storing [1, 512] instead of [0, 511]
    
    for(size_t n = 0; n < run_length; n++) {
      if(full) {
        log(LogSource::MAP, LogLevel::ALERT, "Error in MapblockCompressed::decompress: decompressed mapblock is too long");
        mb->dont_write_to_db = true;
        return mb;
      }
      
      mb->data[x][y][z] = run_val;
      z++;
      if(z >= MAPBLOCK_SIZE_Z) {
        z = 0;
        x++;
        if(x >= MAPBLOCK_SIZE_X) {
          x = 0;
          y++;
          if(y >= MAPBLOCK_SIZE_Y) {
            y = 0;
            full = true;
          }
        }
      }
    }
  }
  if(!full) {
    log(LogSource::MAP, LogLevel::ALERT, "Error in MapblockCompressed::decompress: decompressed mapblock is too short");
    mb->dont_write_to_db = true;
    return mb;
  }
  
  
  //light
  y = 0;
  x = 0;
  z = 0;
  full = false;
  for(size_t i = 0; i < light_data_c_len; i++) {
    uint32_t run_val = light_data_c[i] & 255;
    size_t run_length = ((light_data_c[i] >> 8) & 255) + 1;
    
    for(size_t n = 0; n < run_length; n++) {
      if(full) {
        log(LogSource::MAP, LogLevel::ALERT, "Error in MapblockCompressed::decompress: decompressed mapblock light is too long");
        mb->dont_write_to_db = true;
        return mb;
      }
      
      mb->data[x][y][z] |= run_val << 23;
      z++;
      if(z >= MAPBLOCK_SIZE_Z) {
        z = 0;
        x++;
        if(x >= MAPBLOCK_SIZE_X) {
          x = 0;
          y++;
          if(y >= MAPBLOCK_SIZE_Y) {
            y = 0;
            full = true;
          }
        }
      }
    }
  }
  if(!full) {
    log(LogSource::MAP, LogLevel::ALERT, "Error in MapblockCompressed::decompress: decompressed mapblock light is too short");
    mb->dont_write_to_db = true;
    return mb;
  }
  
  
  return mb;
}
