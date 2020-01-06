#!/usr/bin/python3
import asyncio, websockets, time, json, math

MAPBLOCK_SIZE = (16, 16, 16)

class Mapblock:
    def __init__(self, data):
        self.pos = {"x": data["pos"]["x"], "y": data["pos"]["y"], "z": data["pos"]["z"]}
        self.updateNum = data["updateNum"]
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
    
    res["updateNum"] = 0
    res["lightNeedsUpdate"] = 1
    
    res["IDtoIS"] = ["default:air", "default:dirt", "default:grass", "default:stone"]
    res["IStoID"] = {"default:air": 0, "default:dirt": 1, "default:grass": 2, "default:stone": 3}
    
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
    
    current.data[local_pos[0]][local_pos[1]][local_pos[2]] = current.getNodeID(data["itemstring"]) #TODO - rotation
    
    if data["itemstring"] != "default:air":
        current.props["sunlit"] = False
    
    cache[mb_pos] = current

users = set()
to_send = []

class Message:
    def __init__(self, data, ignore=None):
        self.data = data
        self.dest = dict.fromkeys(users, False)
        if ignore != None:
            for websocket in ignore:
                self.dest[websocket] = True

async def producer(websocket):
    while True:
        for item in to_send:
            if websocket not in item.dest:
                item.dest[websocket] = False
            if item.dest[websocket]:
                continue
            item.dest[websocket] = True
            data = item.data
            
            done = True
            for d in item.dest:
                if item.dest[d] == False:
                    done = False
                    break
            
            if done:
                to_send.remove(item)
            
            return data
        
        await asyncio.sleep(0.1)

async def consumer(websocket, message):
    #print("> " + message)
    
    data = json.loads(message)
    
    s = data["type"] + " "
    if data["type"] == "req_mapblock":
        s += str(data["pos"])
    elif data["type"] == "set_mapblock":
        s += str(data["data"]["pos"])
    elif data["type"] == "set_node":
        s += str(data["pos"]) + " " + str(data["data"])
    print("> " + s)
    
    if data["type"] == "req_mapblock":
        await websocket.send(json.dumps({
            "type": "req_mapblock",
            "data": getMapblock(xyzToTuple(data["pos"])).__dict__
        }))
    elif data["type"] == "set_mapblock":
        setMapblock(Mapblock(data["data"]))
        to_send.append(Message(json.dumps({
            "type": "req_mapblock",
            "data": getMapblock(xyzToTuple(data["data"]["pos"])).__dict__
        }), [websocket]))
    elif data["type"] == "set_node":
        setNode(xyzToTuple(data["pos"]), data["data"])
        getMapblock(mbPos(xyzToTuple(data["pos"]))).updateNum += 1
        
        to_send.append(Message(json.dumps({
            "type": "req_mapblock",
            "data": getMapblock(mbPos(xyzToTuple(data["pos"]))).__dict__
        }), [websocket]))

async def consumer_handler(websocket, path):
    async for message in websocket:
        await consumer(websocket, message)

async def producer_handler(websocket, path):
    while True:
        message = await producer(websocket)
        await websocket.send(message)

async def connect(websocket):
    users.add(websocket)

async def disconnect(websocket):
    users.remove(websocket)

async def server(websocket, path):
    await connect(websocket)
    consumer_task = asyncio.ensure_future(consumer_handler(websocket, path))
    producer_task = asyncio.ensure_future(producer_handler(websocket, path))
    done, pending = await asyncio.wait([consumer_task, producer_task], return_when=asyncio.FIRST_COMPLETED)
    for task in pending:
        task.cancel()
    await disconnect(websocket)


start_server = websockets.serve(server, port=8080)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
