#ifndef __PLAYER_H__
#define __PLAYER_H__

#define BOOST_ERROR_CODE_HEADER_ONLY

#include "vector.h"
#include "mapblock.h"
#include "map.h"
#include "json.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

using WsServer = websocketpp::server<websocketpp::config::asio>;
using websocketpp::connection_hdl;



class PlayerState {
  public:
    PlayerState(connection_hdl hdl);
    
    bool operator<(const PlayerState& other) const {
      return m_tag < other.m_tag;
    }
    bool operator==(const PlayerState& other) const {
      return m_tag == other.m_tag;
    }
    
    std::string get_tag() const {
      return boost::uuids::to_string(m_tag);
    }
    
    std::string get_name() const {
      return m_name;
    }
    
    void set_name(std::string new_name) {
      m_name = new_name;
    }
    
    std::string pos_as_json();
    std::string entity_data_as_json();
    
    bool needs_mapblock_update(MapblockUpdateInfo info);
    void send_mapblock(Mapblock *mb, WsServer& sender);
    
    void prepare_mapblocks(std::vector<Vector3<int>> mapblock_list, Map& map);
    void prepare_nearby_mapblocks(int mb_radius, int mb_radius_outer, Map& map);
    
    void update_mapblocks(std::vector<Vector3<int>> mapblock_list, Map& map, WsServer& sender);
    void update_nearby_mapblocks(int mb_radius, Map& map, WsServer& sender);
    void update_nearby_known_mapblocks(int mb_radius, Map& map, WsServer& sender);
    
    void send(std::string msg, WsServer& sender) {
      sender.send(m_connection_hdl, msg, websocketpp::frame::opcode::text);
    }
    
    bool auth;
    
    //Stores the player's physical position.
    //Once 'auth' is set to true, these will contain valid data.
    Vector3<double> pos;
    Vector3<double> vel;
    Quaternion rot;
    
    std::map<Vector3<int>, MapblockUpdateInfo> known_mapblocks;
    std::set<std::string> known_player_tags;
  private:
    connection_hdl m_connection_hdl;
    boost::uuids::uuid m_tag;
    std::string m_name;
};

#endif