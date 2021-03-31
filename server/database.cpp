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

#include <stdexcept>

void Database::lock_mapblock_shared(MapPos<int> pos) {
  std::shared_lock<std::shared_mutex> table_lock_shared(mapblock_locks_lock);
  
  auto search = mapblock_locks.find(pos);
  if(search != mapblock_locks.end()) {
    std::shared_lock<std::shared_mutex> entry_lock(*search->second.first);
    table_lock_shared.unlock();
    search->second.second->lock_shared();
  } else {
    table_lock_shared.unlock();
    std::unique_lock<std::shared_mutex> table_lock_unique(mapblock_locks_lock);
    
    auto new_search = mapblock_locks.find(pos);
    if(new_search != mapblock_locks.end()) {
      std::shared_lock<std::shared_mutex> entry_lock(*new_search->second.first);
      table_lock_unique.unlock();
      new_search->second.second->lock_shared();
    } else {
      std::shared_mutex *m1 = new std::shared_mutex();
      std::shared_mutex *m2 = new std::shared_mutex();
      auto [it, success] = mapblock_locks.insert(std::make_pair(pos, std::make_pair(m1, m2)));
      std::shared_lock<std::shared_mutex> entry_lock(*it->second.first);
      table_lock_unique.unlock();
      it->second.second->lock_shared();
    }
  }
}

void Database::unlock_mapblock_shared(MapPos<int> pos) {
  std::shared_lock<std::shared_mutex> table_lock_shared(mapblock_locks_lock);
  
  auto search = mapblock_locks.find(pos);
  if(search != mapblock_locks.end()) {
    search->second.second->unlock_shared();
    
    table_lock_shared.unlock();
    std::unique_lock<std::shared_mutex> table_lock_unique(mapblock_locks_lock);
    
    auto new_search = mapblock_locks.find(pos);
    if(new_search != mapblock_locks.end()) {
      bool is_free = new_search->second.first->try_lock();
      if(is_free) {
        bool is_free2 = new_search->second.second->try_lock();
        if(is_free2) {
          std::shared_mutex *m1 = new_search->second.first;
          std::shared_mutex *m2 = new_search->second.second;
          mapblock_locks.erase(new_search);
          m1->unlock();
          m2->unlock();
          delete m1;
          delete m2;
        } else {
          new_search->second.first->unlock();
        }
      }
    }
  } else {
    throw std::out_of_range("can't unlock mapblock that was not previously locked");
  }
}

void Database::lock_mapblock_unique(MapPos<int> pos) {
  std::shared_lock<std::shared_mutex> table_lock_shared(mapblock_locks_lock);
  
  auto search = mapblock_locks.find(pos);
  if(search != mapblock_locks.end()) {
    std::shared_lock<std::shared_mutex> entry_lock(*search->second.first);
    table_lock_shared.unlock();
    search->second.second->lock();
  } else {
    table_lock_shared.unlock();
    std::unique_lock<std::shared_mutex> table_lock_unique(mapblock_locks_lock);
    
    auto new_search = mapblock_locks.find(pos);
    if(new_search != mapblock_locks.end()) {
      std::shared_lock<std::shared_mutex> entry_lock(*new_search->second.first);
      table_lock_unique.unlock();
      new_search->second.second->lock();
    } else {
      std::shared_mutex *m1 = new std::shared_mutex();
      std::shared_mutex *m2 = new std::shared_mutex();
      auto [it, success] = mapblock_locks.insert(std::make_pair(pos, std::make_pair(m1, m2)));
      std::shared_lock<std::shared_mutex> entry_lock(*it->second.first);
      table_lock_unique.unlock();
      it->second.second->lock();
    }
  }
}

void Database::unlock_mapblock_unique(MapPos<int> pos) {
  std::shared_lock<std::shared_mutex> table_lock_shared(mapblock_locks_lock);
  
  auto search = mapblock_locks.find(pos);
  if(search != mapblock_locks.end()) {
    search->second.second->unlock();
    
    table_lock_shared.unlock();
    std::unique_lock<std::shared_mutex> table_lock_unique(mapblock_locks_lock);
    
    auto new_search = mapblock_locks.find(pos);
    if(new_search != mapblock_locks.end()) {
      bool is_free = new_search->second.first->try_lock();
      if(is_free) {
        bool is_free2 = new_search->second.second->try_lock();
        if(is_free2) {
          std::shared_mutex *m1 = new_search->second.first;
          std::shared_mutex *m2 = new_search->second.second;
          mapblock_locks.erase(new_search);
          m1->unlock();
          m2->unlock();
          delete m1;
          delete m2;
        } else {
          new_search->second.first->unlock();
        }
      }
    }
  } else {
    throw std::out_of_range("can't unlock mapblock that was not previously locked");
  }
}
