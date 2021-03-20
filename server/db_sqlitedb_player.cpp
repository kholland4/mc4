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

#include "database.h"
#include "log.h"

#include <iostream>

void SQLiteDB::store_pw_info(PlayerAuthInfo& info) {
  const char *sql = "INSERT INTO player_auth (type, login_name, auth_id, data) VALUES (?, ?, ?, ?);";
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::store_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_text(statement, 1, info.type.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::store_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_text(statement, 2, info.login_name.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::store_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_text(statement, 3, info.auth_id.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::store_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_text(statement, 4, info.data.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::store_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  
  if(sqlite3_step(statement) != SQLITE_DONE) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::store_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  sqlite3_finalize(statement);
  
  //FIXME: thread safety
  info.db_unique_id = sqlite3_last_insert_rowid(db);
  info.has_db_unique_id = true;
}

PlayerAuthInfo SQLiteDB::fetch_pw_info(std::string login_name) {
  const char *sql = "SELECT type, auth_id, data, rowid FROM player_auth WHERE login_name=? LIMIT 1;";
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::fetch_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return PlayerAuthInfo();
  }
  if(sqlite3_bind_text(statement, 1, login_name.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::fetch_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return PlayerAuthInfo();
  }
  
  int step_result = sqlite3_step(statement);
  if(step_result == SQLITE_ROW) {
    //Have row.
    PlayerAuthInfo result;
    
    const unsigned char *type_raw = sqlite3_column_text(statement, 0);
    const unsigned char *auth_id_raw = sqlite3_column_text(statement, 1);
    const unsigned char *data_raw = sqlite3_column_text(statement, 2);
    int64_t row_id = sqlite3_column_int64(statement, 3);
    
    result.login_name = login_name;
    result.type = std::string(reinterpret_cast<const char*>(type_raw));
    result.auth_id = std::string(reinterpret_cast<const char*>(auth_id_raw));
    result.data = std::string(reinterpret_cast<const char*>(data_raw));
    result.db_unique_id = row_id;
    result.has_db_unique_id = true;
    result.is_nil = false;
    
    sqlite3_finalize(statement);
    return result;
  } else if(step_result == SQLITE_DONE) {
    //no more rows
    sqlite3_finalize(statement);
    return PlayerAuthInfo();
  } else {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::fetch_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return PlayerAuthInfo();
  }
}
void SQLiteDB::update_pw_info(std::string old_login_name, PlayerAuthInfo info) {
  if(!info.has_db_unique_id) {
    log(LogSource::SQLITEDB, LogLevel::ERR, "unable to update auth info for " + old_login_name + ": no db_unique_id");
    return;
  }
  
  const char *sql = "UPDATE player_auth SET type=?, login_name=?, auth_id=?, data=? WHERE rowid=? LIMIT 1;";
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::update_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_text(statement, 1, info.type.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::update_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_text(statement, 2, info.login_name.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::update_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_text(statement, 3, info.auth_id.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::update_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_text(statement, 4, info.data.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::update_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 5, info.db_unique_id)) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::update_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  
  if(sqlite3_step(statement) != SQLITE_DONE) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::update_pw_info: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  sqlite3_finalize(statement);
}
void SQLiteDB::delete_pw_info(std::string login_name) {
  //TODO
}



void SQLiteDB::store_player_data(PlayerData data) {
  const char *sql = "INSERT INTO player_data (auth_id, data) VALUES (?, ?);";
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::store_player_data: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_text(statement, 1, data.auth_id.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::store_player_data: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  std::string json = data.to_json();
  if(sqlite3_bind_text(statement, 2, json.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::store_player_data: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  
  if(sqlite3_step(statement) != SQLITE_DONE) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::store_player_data: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  sqlite3_finalize(statement);
}
PlayerData SQLiteDB::fetch_player_data(std::string auth_id) {
  const char *sql = "SELECT data FROM player_data WHERE auth_id=? LIMIT 1;";
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::fetch_player_data: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return PlayerData();
  }
  if(sqlite3_bind_text(statement, 1, auth_id.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::fetch_player_data: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return PlayerData();
  }
  
  int step_result = sqlite3_step(statement);
  if(step_result == SQLITE_ROW) {
    //Have row.
    const unsigned char *data_json_raw = sqlite3_column_text(statement, 0);
    std::string data_json(reinterpret_cast<const char*>(data_json_raw));
    
    PlayerData result(data_json, auth_id);
    
    sqlite3_finalize(statement);
    return result;
  } else if(step_result == SQLITE_DONE) {
    //no more rows
    sqlite3_finalize(statement);
    return PlayerData();
  } else {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::fetch_player_data: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return PlayerData();
  }
}
void SQLiteDB::update_player_data(PlayerData data) {
  const char *sql = "UPDATE player_data SET data=? WHERE auth_id=? LIMIT 1;";
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::update_player_data: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  std::string json = data.to_json();
  if(sqlite3_bind_text(statement, 1, json.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::update_player_data: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_text(statement, 2, data.auth_id.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::update_player_data: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  
  if(sqlite3_step(statement) != SQLITE_DONE) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::update_player_data: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  sqlite3_finalize(statement);
}
void SQLiteDB::delete_player_data(std::string auth_id) {
  //TODO
}
