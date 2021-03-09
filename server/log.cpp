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

#include "log.h"

#include <iostream>
#include <map>

std::map<LogSource, std::string> log_source_messages = {
  {LogSource::SERVER, "server"},
  {LogSource::SQLITEDB, "SQLiteDB"},
  {LogSource::LOADER, "loader"},
  {LogSource::MAP, "map"}
};

std::map<LogLevel, std::string> log_level_messages = {
  {LogLevel::EMERG, "FATAL"},
  {LogLevel::ALERT, "ALERT"},
  {LogLevel::ERR, "Error"},
  {LogLevel::WARNING, "Warning"},
  {LogLevel::NOTICE, "notice"},
  {LogLevel::INFO, "info"},
  {LogLevel::EXTRA, "verbose"},
  {LogLevel::DEBUG, "debug"}
};

void log(LogSource src, LogLevel lvl, std::string message) {
  if(lvl > LogLevel::EXTRA) { return; } //TODO: make configurable
  
  std::cerr << log_level_messages[lvl] << " [" << log_source_messages[src] << "]   " << message << std::endl;
}
