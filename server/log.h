#ifndef __LOG_H__
#define __LOG_H__

#include <string>

enum class LogSource {
  SERVER,
  SQLITEDB,
  LOADER
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
