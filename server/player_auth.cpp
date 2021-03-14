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

#include "player_auth.h"

#include <sstream>
#include <regex>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

//#include "lib/ctbignum/ctbignum.hpp"
//#include "lib/ctbignum/mod_exp.hpp"

/*
    DH Parameters: (1024 bit)
        prime:
            00:d0:31:b5:57:27:40:b0:f6:b6:9a:38:84:eb:98:
            fe:1a:ab:a5:c9:b0:9f:3b:25:f2:79:81:6e:da:71:
            2f:dc:1f:4b:01:f4:54:74:d6:7f:69:61:aa:76:0a:
            21:a2:1b:ea:ff:2e:b8:d3:3d:0b:05:bd:f0:8e:2d:
            e3:ea:f2:23:24:7e:d9:fb:4e:7a:40:59:06:b1:e7:
            57:a4:c0:ff:5c:0d:83:e6:60:b3:c7:82:f3:21:3d:
            60:e2:1a:b8:d8:df:29:51:20:72:bb:43:15:06:17:
            8b:af:39:c5:bb:e7:66:2d:fb:d4:94:15:ee:df:e0:
            25:80:6a:48:e0:ad:99:9e:a3
        generator: 2 (0x2)
-----BEGIN DH PARAMETERS-----
MIGHAoGBANAxtVcnQLD2tpo4hOuY/hqrpcmwnzsl8nmBbtpxL9wfSwH0VHTWf2lh
qnYKIaIb6v8uuNM9CwW98I4t4+ryIyR+2ftOekBZBrHnV6TA/1wNg+Zgs8eC8yE9
YOIauNjfKVEgcrtDFQYXi685xbvnZi371JQV7t/gJYBqSOCtmZ6jAgEC
-----END DH PARAMETERS-----
*/

//'N' in srp
//using GF1024 = decltype(cbn::Zq(146198920325700438841009225581592483916990556834762974961884344477099509713950442942736459626001468444398384711851720189354446011771742470546010913184050612491768178355190281957995078048762050765279860273669354370726575968618577949364684625891111055473909172902453230789469007622019404840370187910048156065443_Z));
//#define srp_g 2

void init_player_data(PlayerData &data) {
  data.pos.set(0, 20, 0, 0, 0, 0);
  data.vel.set(0, 0, 0, 0, 0, 0);
  data.rot.set(0, 0, 0, 0);
}

bool validate_player_name(std::string name) {
  std::regex nick_allow("^[a-zA-Z0-9\\-_]{1,40}$");
  if(!std::regex_match(name, nick_allow)) {
    //not ok
    return false;
  }
  return true;
}


bool PlayerAuthenticator::step(std::string message, WsServer& server, connection_hdl& hdl, Database& db) {
  if(has_backend) {
    return auth_backend->step(message, server, hdl, db);
  }
  
  try {
    std::stringstream ss;
    ss << message;
    
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);
    
    std::string type = pt.get<std::string>("type");
    
    if(type == "auth_mode") {
      std::string mode = pt.get<std::string>("mode");
      if(mode == "password-srp") {
        //delete auth_backend;
        auth_backend = new PlayerPasswordAuthenticator();
        has_backend = true;
        
        server.send(hdl, "{\"type\":\"auth_step\",\"message\":\"auth_mode_ok\"}", websocketpp::frame::opcode::text);
        return false;
      }
      
      server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"bad_auth_mode\"}", websocketpp::frame::opcode::text);
      return false;
    } else {
      server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"no_auth_mode\"}", websocketpp::frame::opcode::text);
      return false;
    }
  } catch(std::exception const& e) {
    std::cerr << message << std::endl;
    std::cerr << e.what() << std::endl;
    
    return false;
  }
}

std::string PlayerAuthenticator::result() {
  if(has_backend) {
    return auth_backend->result();
  }
  return "";
}


bool PlayerPasswordAuthenticator::step(std::string message, WsServer& server, connection_hdl& hdl, Database& db) {
  try {
    std::stringstream ss;
    ss << message;
    
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);
    
    std::string type = pt.get<std::string>("type");
    
    if(type == "auth_step") {
      std::string step = pt.get<std::string>("step");
      
      if(step == "register") {
        std::string login_name = pt.get<std::string>("login_name");
        std::string salt = pt.get<std::string>("salt");
        std::string verifier = pt.get<std::string>("verifier");
        
        if(!validate_player_name(login_name)) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"register_login_name_invalid\"}", websocketpp::frame::opcode::text);
          return false;
        }
        
        PlayerPasswordAuthInfo existing = db.fetch_pw_info(login_name);
        if(!existing.is_nil) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"register_login_name_exists\"}", websocketpp::frame::opcode::text);
          return false;
        }
        
        std::string auth_id = boost::uuids::to_string(boost::uuids::random_generator()());
        
        PlayerPasswordAuthInfo new_info;
        new_info.login_name = login_name;
        new_info.salt = salt;
        new_info.verifier = verifier;
        new_info.auth_id = auth_id;
        new_info.is_nil = false;
        db.store_pw_info(new_info);
        
        PlayerData new_data;
        new_data.name = login_name;
        new_data.auth_id = auth_id;
        new_data.is_nil = false;
        init_player_data(new_data);
        db.store_player_data(new_data);
        
        server.send(hdl, "{\"type\":\"auth_step\",\"message\":\"register_ok\"}", websocketpp::frame::opcode::text);
        server.send(hdl, "{\"type\":\"auth_step\",\"message\":\"auth_ok\"}", websocketpp::frame::opcode::text);
        
        auth_info = new_info;
        auth_success = true;
        return true;
      }
      
      if(step == "login") {
        std::string login_name = pt.get<std::string>("login_name");
        //std::string A_str = pt.get<std::string>("A");
        
        //TODO validate input parameters
        
        PlayerPasswordAuthInfo search = db.fetch_pw_info(login_name);
        if(search.is_nil) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"login_name_not_found\"}", websocketpp::frame::opcode::text);
          return false;
        }
        
        //TODO
        //unsigned long A_l = stol(A_str);
        //cbn::big_int<1, unsigned long> A {A_l};
        
        //TODO use real SRP algorithm
        std::string verifier = pt.get<std::string>("verifier");
        if(verifier == search.verifier) {
          server.send(hdl, "{\"type\":\"auth_step\",\"message\":\"auth_ok\"}", websocketpp::frame::opcode::text);
          auth_info = search;
          auth_success = true;
          return true;
        }
        
        server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"invalid_verifier\"}", websocketpp::frame::opcode::text);
        return false;
      }
      
      if(step == "update_pw") {
        if(!auth_success) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"not_auth\"}", websocketpp::frame::opcode::text);
          return false;
        }
        
        std::string login_name = pt.get<std::string>("login_name");
        std::string salt = pt.get<std::string>("salt");
        std::string verifier = pt.get<std::string>("verifier");
        
        if(!validate_player_name(login_name)) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"update_login_name_invalid\"}", websocketpp::frame::opcode::text);
          return false;
        }
        
        PlayerPasswordAuthInfo search = db.fetch_pw_info(auth_info.login_name);
        if(search.is_nil) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"login_name_not_found\"}", websocketpp::frame::opcode::text);
          return true;
        }
        
        PlayerPasswordAuthInfo existing = db.fetch_pw_info(login_name);
        if(!existing.is_nil) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"update_login_name_exists\"}", websocketpp::frame::opcode::text);
          return true;
        }
        
        search.login_name = login_name;
        search.salt = salt;
        search.verifier = verifier;
        
        db.update_pw_info(auth_info.login_name, search);
        
        auth_info = search;
        
        server.send(hdl, "{\"type\":\"auth_step\",\"message\":\"update_ok\"}", websocketpp::frame::opcode::text);
        return true;
      }
      
      server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"invalid_step\"}", websocketpp::frame::opcode::text);
      return false;
    } else {
      server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"unknown_command\"}", websocketpp::frame::opcode::text);
      return false;
    }
  } catch(std::exception const& e) {
    std::cerr << message << std::endl;
    std::cerr << e.what() << std::endl;
    
    return false;
  }
}

std::string PlayerPasswordAuthenticator::result() {
  if(auth_success) {
    return auth_info.auth_id;
  } else {
    return "";
  }
}
