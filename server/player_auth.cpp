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

#include "player_util.h"
#include "json.h"
#include "log.h"
#include "config.h"

#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <openssl/evp.h>
#include <openssl/kdf.h>

constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

std::string hash_password(std::string password, std::string salt) {
  EVP_PKEY_CTX *kdf = EVP_PKEY_CTX_new_id(EVP_PKEY_SCRYPT, NULL);
  if(EVP_PKEY_derive_init(kdf) <= 0) {
    log(LogSource::AUTH, LogLevel::ERR, "error in EVP_PKEY_derive_init");
    return "";
  }
  if(EVP_PKEY_CTX_set1_pbe_pass(kdf, password.c_str(), password.length()) <= 0) {
    log(LogSource::AUTH, LogLevel::ERR, "error in EVP_PKEY_CTX_set1_pbe_pass");
    return "";
  }
  if(EVP_PKEY_CTX_set1_scrypt_salt(kdf, salt.c_str(), salt.length()) <= 0) {
    log(LogSource::AUTH, LogLevel::ERR, "error in EVP_PKEY_CTX_set1_scrypt_salt");
    return "";
  }
  if(EVP_PKEY_CTX_set_scrypt_N(kdf, 1024) <= 0) {
    log(LogSource::AUTH, LogLevel::ERR, "error in EVP_PKEY_CTX_set_scrypt_N");
    return "";
  }
  if(EVP_PKEY_CTX_set_scrypt_r(kdf, 8) <= 0) {
    log(LogSource::AUTH, LogLevel::ERR, "error in EVP_PKEY_CTX_set_scrypt_r");
    return "";
  }
  if(EVP_PKEY_CTX_set_scrypt_p(kdf, 16) <= 0) {
    log(LogSource::AUTH, LogLevel::ERR, "error in EVP_PKEY_CTX_set_scrypt_p");
    return "";
  }
  
  unsigned char hash_out[64];
  size_t hash_out_len = sizeof(hash_out);
  if(EVP_PKEY_derive(kdf, hash_out, &hash_out_len) <= 0) {
    log(LogSource::AUTH, LogLevel::ERR, "error in EVP_PKEY_derive");
    return "";
  }
  
  EVP_PKEY_CTX_free(kdf);
  
  std::string password_hashed(128, ' ');
  for(size_t i = 0; i < 64; i++) {
    password_hashed[i * 2] = hexmap[(hash_out[i] >> 4) & 15];
    password_hashed[i * 2 + 1] = hexmap[hash_out[i] & 15];
  }
  
  return password_hashed;
}


bool PlayerAuthenticator::step(std::string message, WsServer& server, connection_hdl& hdl, Database& db) {
  try {
    std::stringstream ss;
    ss << message;
    
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);
    
    std::string type = pt.get<std::string>("type");
    
    if(type == "auth_list") {
      //TODO
      
      return false;
    } else {
      std::string backend = pt.get<std::string>("backend");
      if(!has_backend || backend != auth_backend_name) {
        if(has_backend) {
          has_backend = false;
          auth_backend_name = "";
          delete auth_backend;
        }
        
        if(backend == "password-plain") {
          auth_backend = new PlayerPasswordAuthenticator();
          auth_backend_name = "password-plain";
          has_backend = true;
        }
      }
      
      if(has_backend) {
        return auth_backend->step(message, server, hdl, db);
      }
      
      server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"bad_auth_mode\"}", websocketpp::frame::opcode::text);
      return false;
    }
  } catch(boost::property_tree::ptree_error const& e) {
    log(LogSource::AUTH, LogLevel::ERR, "JSON parse error: " + std::string(e.what()) + " message=" + message);
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
        if(!get_config<bool>("auth.allow_register")) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"registration_disabled\"}", websocketpp::frame::opcode::text);
          return false;
        }
        
        std::string login_name = pt.get<std::string>("login_name");
        std::string client_salt = pt.get<std::string>("client_salt");
        std::string password = pt.get<std::string>("password");
        
        if(!validate_player_name(login_name)) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"register_login_name_invalid\"}", websocketpp::frame::opcode::text);
          return false;
        }
        if(db.player_data_name_used(login_name)) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"register_login_name_exists\"}", websocketpp::frame::opcode::text);
          return false;
        }
        //TODO what about names used by currently online players?
        
        std::vector<PlayerAuthInfo> existing = db.fetch_pw_info(login_name, "password-plain");
        if(existing.size() > 0) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"register_login_name_exists\"}", websocketpp::frame::opcode::text);
          return false;
        }
        
        //Salts only need to be unique, not necessarily cryptographically random.
        std::string salt = boost::uuids::to_string(boost::uuids::random_generator()());
        std::string password_hashed = hash_password(password, salt);
        if(password_hashed == "") {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"internal_error\"}", websocketpp::frame::opcode::text);
          return false;
        }
        
        std::string auth_id = boost::uuids::to_string(boost::uuids::random_generator()());
        
        PlayerAuthInfo new_info;
        new_info.type = "password-plain";
        new_info.login_name = login_name;
        new_info.auth_id = auth_id;
        new_info.data = "{\"password_hashed\":\"" + json_escape(password_hashed) + "\",\"salt\":\"" + json_escape(salt) + "\",\"client_salt\":\"" + json_escape(client_salt) + "\"}";
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
        
        log(LogSource::AUTH, LogLevel::INFO, "registered new account for " + new_info.login_name + " (backend=password-plain)");
        
        auth_info = new_info;
        auth_success = true;
        return true;
      }
      
      if(step == "login") {
        std::string login_name = pt.get<std::string>("login_name");
        std::string password = pt.get<std::string>("password");
        
        std::vector<PlayerAuthInfo> search = db.fetch_pw_info(login_name, "password-plain");
        if(search.size() == 0) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"login_name_not_found\"}", websocketpp::frame::opcode::text);
          return false;
        }
        
        for(auto& it : search) {
          std::string pw_json = it.data;
          std::stringstream pw_ss;
          pw_ss << pw_json;
          boost::property_tree::ptree pw_pt;
          boost::property_tree::read_json(pw_ss, pw_pt);
          
          std::string salt = pw_pt.get<std::string>("salt");
          std::string password_hashed = pw_pt.get<std::string>("password_hashed");
          
          std::string input_pw_hashed = hash_password(password, salt);
          if(input_pw_hashed == "") {
            server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"internal_error\"}", websocketpp::frame::opcode::text);
            return false;
          }
          
          if(input_pw_hashed == password_hashed) {
            server.send(hdl, "{\"type\":\"auth_step\",\"message\":\"auth_ok\"}", websocketpp::frame::opcode::text);
            log(LogSource::AUTH, LogLevel::INFO, "login success for " + it.login_name + " (backend=password-plain)");
            auth_info = it;
            auth_success = true;
            return true;
          }
        }
        
        log(LogSource::AUTH, LogLevel::INFO, "login fail for " + login_name + " (backend=password-plain)");
        server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"invalid_password\"}", websocketpp::frame::opcode::text);
        return false;
      }
      
      if(step == "update_pw") {
        if(!auth_success) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"not_auth\"}", websocketpp::frame::opcode::text);
          return false;
        }
        
        std::string login_name = pt.get<std::string>("login_name");
        std::string client_salt = pt.get<std::string>("client_salt");
        std::string password = pt.get<std::string>("password");
        
        if(!validate_player_name(login_name)) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"update_login_name_invalid\"}", websocketpp::frame::opcode::text);
          return false;
        }
        //no need to check db or online players, login name is not a default nickname
        
        std::vector<PlayerAuthInfo> existing = db.fetch_pw_info(login_name, "password-plain");
        if(existing.size() > 0) {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"update_login_name_exists\"}", websocketpp::frame::opcode::text);
          return true;
        }
        
        //Salts only need to be unique, not necessarily cryptographically random.
        std::string salt = boost::uuids::to_string(boost::uuids::random_generator()());
        std::string password_hashed = hash_password(password, salt);
        if(password_hashed == "") {
          server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"internal_error\"}", websocketpp::frame::opcode::text);
          return false;
        }
        
        log(LogSource::AUTH, LogLevel::INFO, "credentials updated for " + auth_info.login_name + ", new name is " + login_name + " (backend=password-plain)");
        
        auth_info.login_name = login_name;
        auth_info.data = "{\"password_hashed\":\"" + json_escape(password_hashed) + "\",\"salt\":\"" + json_escape(salt) + "\",\"client_salt\":\"" + json_escape(client_salt) + "\"}";
        
        db.update_pw_info(auth_info);
        
        server.send(hdl, "{\"type\":\"auth_step\",\"message\":\"update_ok\"}", websocketpp::frame::opcode::text);
        return true;
      }
      
      server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"invalid_step\"}", websocketpp::frame::opcode::text);
      return false;
    } else {
      server.send(hdl, "{\"type\":\"auth_err\",\"reason\":\"unknown_command\"}", websocketpp::frame::opcode::text);
      return false;
    }
  } catch(boost::property_tree::ptree_error const& e) {
    log(LogSource::AUTH, LogLevel::ERR, "JSON parse error: " + std::string(e.what()) + " message=" + message);
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
