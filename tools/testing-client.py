#!/usr/bin/python3

"""
    mc4 testing client
    Copyright (C) 2021 kholland4

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""

NUM_WEBSOCKETS = 400

import asyncio, websockets, json, time

async def hello():
    uri = "ws://localhost:8080"
    ws = []
    for i in range(NUM_WEBSOCKETS):
        ws.append(await websockets.connect(uri))
    for i in range(len(ws)):
        await ws[i].send(json.dumps({
            "type": "auth_guest"
        }))
        await ws[i].recv()
        
        await ws[i].send(json.dumps({
            "type": "set_player_pos",
            "pos": { "x": i, "y": 10, "z": 0, "w": 0, "world": 0, "universe": 0 },
            "vel": { "x": 0, "y": 0, "z": 0, "w": 0, "world": 0, "universe": 0 },
            "rot": { "x": 0, "y": 0, "z": 0, "w": 0 }
        }))
        
        time.sleep(0.1)
    
    time.sleep(1)
    
    for n in range(2000):
        for i in range(len(ws)):
            await ws[i].send(json.dumps({
                "type": "set_node",
                "pos": { "x": i, "y": 5, "z": 0, "w": 0, "world": 0, "universe": 0 },
                "data": { "itemstring": "default:stone", "rot": 0 }
            }))
            
            #time.sleep(0.5 / float(len(ws)))
        
        time.sleep(0.1)
        
        for i in range(len(ws)):
            await ws[i].send(json.dumps({
                "type": "set_node",
                "pos": { "x": i, "y": 5, "z": 0, "w": 0, "world": 0, "universe": 0 },
                "data": { "itemstring": "air", "rot": 0 }
            }))
            
            #time.sleep(0.5 / float(len(ws)))
        
        time.sleep(0.1)
    
    exit()

asyncio.get_event_loop().run_until_complete(hello())
