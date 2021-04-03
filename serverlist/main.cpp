/*
    mc4 server list
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

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include <curl/curl.h>

#define BOOST_ERROR_CODE_HEADER_ONLY

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "json.h"

class ServerlistEntry {
  public:
    std::string address;
    std::string desc;
    
    bool online = false;
    std::string version;
    int players = 0;
    int player_limit = 0;
    
    std::string as_json() const {
      std::ostringstream out;
      
      out << "{\"address\":\"" << json_escape(address) << "\","
          << "\"desc\":\"" << json_escape(desc) << "\","
          << "\"online\":" << std::boolalpha << online << ",";
      
      std::string version_safe = version;
      if(version_safe.size() > 256) {
        version_safe = version_safe.substr(0, 256);
      }
      
      out << "\"version\":\"" << json_escape(version_safe) << "\","
          << "\"players\":" << players << ","
          << "\"player_limit\":" << player_limit;
      out << "}";
      
      return out.str();
    };
};

std::vector<ServerlistEntry> serverlist;

size_t write_function(void *ptr, size_t size, size_t nmemb, std::string *data) {
  data->append((char*)ptr, size * nmemb);
  return size * nmemb;
}

int main(int argc, char *argv[]) {
  // Get filenames to read JSON from
  if(argc < 2) {
    std::cerr << "usage: " << argv[0] << " [list.json]" << std::endl;
    exit(1);
  }
  std::vector<std::string> files_to_ingest;
  for(int i = 1; i < argc; i++) {
    files_to_ingest.push_back(std::string(argv[i]));
  }
  
  // Read JSON files and extract server information
  for(const std::string& filename : files_to_ingest) {
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(filename, pt);
    
    for(auto& item : pt) {
      ServerlistEntry s;
      
      for(auto& key : item.second) {
        if(key.first == "address") {
          s.address = key.second.data();
        } else if(key.first == "desc") {
          s.desc = key.second.data();
        } else {
          std::cerr << "unrecognized server property '" + key.first + "'" << std::endl;
          exit(1);
        }
      }
      
      serverlist.push_back(s);
    }
  }
  
  // Query each server on the list for information
  curl_global_init(CURL_GLOBAL_DEFAULT);
  for(ServerlistEntry& s : serverlist) {
    // Convert websocket URLs to HTTP/HTTPS URLs
    std::string url = s.address;
    if(url.rfind("wss://", 0) == 0) {
      url = "https://" + url.substr(6);
    } else if(url.rfind("ws://", 0) == 0) {
      url = "http://" + url.substr(5);
    } else {
      std::cerr << "Unrecognized server URL: " << url << std::endl;
      exit(1);
    }
    
    auto curl = curl_easy_init();
    if(!curl) {
      std::cerr << "cURL init failed" << std::endl;
      exit(1);
    }
    
    std::cerr << "Querying " << url << " ..." << std::endl;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 15L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1000L);
    
    std::string response_string;
    std::string header_string;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);
    
    int curl_code = curl_easy_perform(curl);
    if(curl_code == CURLE_OK) {
      long http_code;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
      
      std::cerr << "got HTTP " << http_code << std::endl;
      
      if(http_code == 200) {
        std::cerr << response_string << std::endl;
        
        std::stringstream ss;
        ss << response_string;
        
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(ss, pt);
        
        s.version = pt.get<std::string>("version");
        s.players = pt.get<int>("players");
        s.player_limit = pt.get<int>("player_limit");
        
        s.online = true;
      }
    } else {
      std::cerr << "cURL error code " << curl_code << std::endl;
    }
    
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
  
  // Output JSON server list to std::cout
  std::cout << "[";
  bool first = true;
  for(const ServerlistEntry& s : serverlist) {
    if(!first) { std::cout << ","; }
    first = false;
    
    std::cout << s.as_json();
  }
  std::cout << "]" << std::endl;
}
