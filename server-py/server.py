#!/usr/bin/python3

"""
    mc4, a web voxel building game
    Copyright (C) 2019 kholland4

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

import asyncio, websockets, time, json, math, random

MAPBLOCK_SIZE = (16, 16, 16)

class Entity:
    def __init__(self, ident):
        self.id = ident
        self.pos = {"x": 0, "y": 0, "z": 0}
        self.vel = {"x": 0, "y": 0, "z": 0}
        self.rot = {"x": 0, "y": 0, "z": 0, "w": 0}
    def serialize(self):
        return {
            "id": self.id,
            "pos": self.pos,
            "vel": self.vel,
            "rot": self.rot
        }

class UserState:
    def __init__(self, websocket):
        self.websocket = websocket
        self.name = "user" + str(random.randint(100, 999))
        self.entity = Entity(self.name)

class Mapblock:
    def __init__(self, data):
        self.pos = {"x": data["pos"]["x"], "y": data["pos"]["y"], "z": data["pos"]["z"]}
        self.updateNum = data["updateNum"]
        self.lightUpdateNum = data["lightUpdateNum"]
        self.lightNeedsUpdate = data["lightNeedsUpdate"]
        self.IDtoIS = data["IDtoIS"]
        self.IStoID = data["IStoID"]
        self.props = data["props"]
        self.data = data["data"]
    
    def getNodeID(self, itemstring):
        if itemstring not in self.IStoID:
            self.IStoID[itemstring] = len(self.IDtoIS)
            self.IDtoIS.append(itemstring)
        return self.IStoID[itemstring]
    
    def getItemstring(self, nodeID):
        return self.IDtoIS[nodeID]

def genMapblock(pos):
    res = {}
    res["type"] = "req_mapblock"
    
    res["pos"] = {"x": pos[0], "y": pos[1], "z": pos[2]}
    
    res["updateNum"] = 1
    res["lightUpdateNum"] = 0
    res["lightNeedsUpdate"] = 1
    
    res["IDtoIS"] = ["air", "default:dirt", "default:grass", "default:stone"]
    res["IStoID"] = {"air": 0, "default:dirt": 1, "default:grass": 2, "default:stone": 3}
    
    res["props"] = {"sunlit": pos[1] >= 1}
    
    data = []
    for x in range(MAPBLOCK_SIZE[0]):
        s1 = []
        for y in range(MAPBLOCK_SIZE[1]):
            s2 = []
            for z in range(MAPBLOCK_SIZE[2]):
                if pos[1] == -1:
                    if y == 15:
                        s2.append(2)
                    elif y >= 13:
                        s2.append(1)
                    else:
                        s2.append(3)
                else:
                    s2.append(0)
            s1.append(s2)
        data.append(s1)
    
    res["data"] = data
    
    return Mapblock(res)

cache = {}

def getMapblock(pos):
    if pos in cache:
        return cache[pos]
    else:
        cache[pos] = genMapblock(pos)
    
    return cache[pos]

def setMapblock(mapblock):
    index = (mapblock.pos["x"], mapblock.pos["y"], mapblock.pos["z"])
    cache[index] = mapblock
    
    mapblock.updateNum += 1
    mapblock.lightNeedsUpdate = 2
    
    to_send.append(Message(json.dumps({
        "type": "req_mapblock",
        "data": mapblock.__dict__
    }))) #, [websocket]))

def xyzToTuple(data):
    return (data["x"], data["y"], data["z"])
def mbPos(pos):
    return (math.floor(pos[0] / MAPBLOCK_SIZE[0]), math.floor(pos[1] / MAPBLOCK_SIZE[1]), math.floor(pos[2] / MAPBLOCK_SIZE[2]))
def localPos(pos):
    return (pos[0] % MAPBLOCK_SIZE[0], pos[1] % MAPBLOCK_SIZE[1], pos[2] % MAPBLOCK_SIZE[2])
def setNode(pos, data):
    mb_pos = mbPos(pos)
    local_pos = localPos(pos)
    
    current = getMapblock(mb_pos)
    
    current.data[local_pos[0]][local_pos[1]][local_pos[2]] = (current.getNodeID(data["itemstring"]) & 32767) + ((data["rot"] & 255) << 15)
    
    if data["itemstring"] != "air":
        current.props["sunlit"] = False
    
    #cache[mb_pos] = current
    setMapblock(current)

users = set()
to_send = []

class Message:
    def __init__(self, data, ignore=None):
        self.data = data
        self.dest = dict.fromkeys(users, False)
        if ignore != None:
            for userdata in ignore:
                self.dest[userdata] = True

async def producer(userdata):
    while True:
        for item in to_send:
            if userdata not in item.dest:
                item.dest[userdata] = False
            if item.dest[userdata]:
                continue
            item.dest[userdata] = True
            data = item.data
            
            done = True
            for d in item.dest:
                if item.dest[d] == False:
                    done = False
                    break
            
            if done:
                to_send.remove(item)
            
            return data
        
        actions = []
        for user in users:
            if user.name != userdata.name:
                actions.append({"type": "update", "data": user.entity.serialize()})
        await userdata.websocket.send(json.dumps({
            "type": "update_entities",
            "actions": actions
        }))
        
        await asyncio.sleep(0.2)

async def consumer(userdata, message):
    #print("> " + message)
    
    data = json.loads(message)
    
    s = data["type"] + " "
    if data["type"] == "req_mapblock":
        s += str(data["pos"])
    elif data["type"] == "set_mapblock":
        s += str(data["data"]["pos"])
    elif data["type"] == "set_node":
        s += str(data["pos"]) + " " + str(data["data"])
    elif data["type"] == "send_chat":
        s += "[#" + data["channel"] + "] <" + userdata.name + "> " + data["message"]
    if data["type"] != "set_player_pos":
        print("> " + s)
    
    if data["type"] == "req_mapblock":
        await userdata.websocket.send(json.dumps({
            "type": "req_mapblock",
            "data": getMapblock(xyzToTuple(data["pos"])).__dict__
        }))
    elif data["type"] == "set_mapblock":
        setMapblock(Mapblock(data["data"]))
    elif data["type"] == "set_node":
        setNode(xyzToTuple(data["pos"]), data["data"])
    elif data["type"] == "set_player_pos":
        for c in ["x", "y", "z"]:
            userdata.entity.pos[c] = data["pos"][c]
        for c in ["x", "y", "z"]:
            userdata.entity.vel[c] = data["vel"][c]
        for c in ["x", "y", "z", "w"]:
            userdata.entity.rot[c] = data["rot"][c]
    elif data["type"] == "send_chat":
        for user in users:
            #if user.name != userdata.name:
            await user.websocket.send(json.dumps({
                "type": "send_chat",
                "from": userdata.name,
                "channel": data["channel"],
                "message": data["message"]
            }))

async def consumer_handler(userdata, path):
    async for message in userdata.websocket:
        await consumer(userdata, message)

async def producer_handler(userdata, path):
    while True:
        message = await producer(userdata)
        await userdata.websocket.send(message)

async def connect(userdata):
    users.add(userdata)
    entity_data = userdata.entity.serialize()
    actions = []
    for user in users:
        if user.name != userdata.name:
            await user.websocket.send(json.dumps({
                "type": "update_entities",
                "actions": [
                    {"type": "create", "data": entity_data}
                ]
            }))
            
            actions.append({"type": "create", "data": user.entity.serialize()})
    await userdata.websocket.send(json.dumps({
        "type": "update_entities",
        "actions": actions
    }))

async def disconnect(userdata):
    users.remove(userdata)
    entity_data = userdata.entity.serialize()
    for user in users:
        if user.name != userdata.name:
            await user.websocket.send(json.dumps({
                "type": "update_entities",
                "actions": [
                    {"type": "delete", "data": entity_data}
                ]
            }))

async def server(websocket, path):
    userdata = UserState(websocket)
    await connect(userdata)
    consumer_task = asyncio.ensure_future(consumer_handler(userdata, path))
    producer_task = asyncio.ensure_future(producer_handler(userdata, path))
    done, pending = await asyncio.wait([consumer_task, producer_task], return_when=asyncio.FIRST_COMPLETED)
    for task in pending:
        task.cancel()
    await disconnect(userdata)

start_server = websockets.serve(server, port=8080)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
