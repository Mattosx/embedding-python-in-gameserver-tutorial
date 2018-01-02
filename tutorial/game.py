# -*- coding: utf-8 -*-
import noddy4

import socket
import cPickle

class MailBox(object):
    def __init__(self, ipAddr, port):
        super(MailBox, self).__init__()
        self.ipAddr = ipAddr
        self.port = port

    def __getattr__(self, name):
        return remoteMethod(name, self.ipAddr, self.port)


def makeMailBox(args):
    return MailBox(*args)

class remoteMethod(object):
    def __init__(self, name, ipAddr, port):
        super(remoteMethod, self).__init__()
        self.name = name
        self.ipAddr = ipAddr
        self.port = port

    def __call__(self, *args):
        realAddrKey = (self.ipAddr, self.port)
        global socketPool
        if realAddrKey not in socketPool:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.bind(realAddrKey)
            socketPool[realAddrKey] = s

        s = socketPool[realAddr]
        msg = cPickle.dumps((self.port, self.name, args), -1)
        s.sendto(msg, realAddrKey)

class Avatar(noddy4.Noddy):
    def __init__(self, first, last, number, ipAddr, port):
        super(Avatar, self).__init__(first, last, number)
        self.ipAddr = ipAddr
        self.port = port

    def onUpdate(self, dt):
        print(self.name(), 'onUpdate', dt)

    def getMailBox(self):
        v = self.__reduce_ex__()
        return v[0](v[1])

    def __reduce_ex__(self):
        return (makeMailBox, (self.ipAddr, self.port))

socketPool = {}

entities = []
portOffset = 10000

def onMyAppRun(isBootstrap):

    print('onMyAppRun', isBootstrap)

    global portOffset
    for i in range(10):
        player = Avatar('mark' + str(i), 'down', 32, '127.0.0.1', portOffset + i)
        global entities
        box = player.getMailBox()
        print(box)
        entities.append(box)

    print(len(entities))

def getEntityViaPort(port):
    global portOffset
    port -= portOffset
    global entities
    return entities[port]

def onTimerUpdate(dt):
    global entities
    for box in entities:
        box.onUpdate(dt)

    global socketPool
    for realAddrKey, socket in socketPool.iteritems():
        data, addr = socket.recvfrom(1024)
        port, funcName, msg = data
        entity = getEntityViaPort(port)
        getattr(entity, funcName)(*msg)