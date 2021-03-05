#include "log.h"

#include <iostream>
#include <map>

std::map<LogSource, std::string> log_source_messages = {
  {LogSource::SERVER, "server"},
  {LogSource::SQLITEDB, "SQLiteDB"}
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
  //if(lvl > LogLevel::INFO) { return; } //TODO: make configurable
  
  std::cerr << log_level_messages[lvl] << " [" << log_source_messages[src] << "]   " << message << std::endl;
}
