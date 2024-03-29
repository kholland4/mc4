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

#ifndef __LOG_H__
#define __LOG_H__

#include <string>

enum class LogSource {
  SERVER,
  SQLITEDB,
  MEMORYDB,
  LOADER,
  MAP,
  MAPGEN,
  AUTH,
  PLAYER,
  CONFIG,
  INIT,
  VECTOR,
  INVENTORY
};

//Based on https://en.wikipedia.org/wiki/Syslog#Severity_level
enum class LogLevel {
  EMERG,
  ALERT,
  ERR,
  WARNING,
  NOTICE,
  INFO,
  EXTRA,
  DEBUG
};

void log(LogSource src, LogLevel lvl, std::string message);

#endif
