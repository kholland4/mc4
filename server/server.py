#!/usr/bin/python3
import asyncio, websockets, time, json

class MapBlock:
    def __init__(self, data):
        self.pos = {"x": data["pos"]["x"], "y": data["pos"]["y"], "z": data["pos"]["z"]}
        self.updateNum = data["updateNum"]
        self.lightNeedsUpdate = data["lightNeedsUpdate"]
        self.IDtoIS = data["IDtoIS"]
        self.IStoID = data["IStoID"]
        self.data = data["data"]

def genMapblock(px, py, pz):
    res = {}
    res["type"] = "req_mapblock"
    
    res["pos"] = {"x": px, "y": py, "z": pz}
    
    res["updateNum"] = 0
    res["lightNeedsUpdate"] = False
    
    res["IDtoIS"] = ["default:air", "default:dirt", "default:grass", "default:stone"]
    res["IStoID"] = {"default:air": 0, "default:dirt": 1, "default:grass": 2, "default:stone": 3}
    
    data = []
    for x in range(16):
        s1 = []
        for y in range(16):
            s2 = []
            for z in range(16):
                if py == -1:
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
    
    return MapBlock(res)

cache = {}

def getMapblock(px, py, pz):
    index = (px, py, pz)
    if index in cache:
        return cache[index]
    else:
        cache[index] = genMapblock(px, py, pz)
    
    return cache[index]

def setMapblock(mapBlock):
    index = (mapBlock.pos["x"], mapBlock.pos["y"], mapBlock.pos["z"])
    cache[index] = mapBlock

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
    print("> " + s)
    
    if data["type"] == "req_mapblock":
        await websocket.send(json.dumps({
            "type": "req_mapblock",
            "data": getMapblock(data["pos"]["x"], data["pos"]["y"], data["pos"]["z"]).__dict__
        }))
    elif data["type"] == "set_mapblock":
        setMapblock(MapBlock(data["data"]))
        to_send.append(Message(json.dumps({
            "type": "req_mapblock",
            "data": getMapblock(data["data"]["pos"]["x"], data["data"]["pos"]["y"], data["data"]["pos"]["z"]).__dict__
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
