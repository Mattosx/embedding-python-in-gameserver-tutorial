# -*- coding: utf-8 -*-
import Tutorial

import socket
import _pickle as cPickle
import random

import avatar

class MailBox(object):
    def __init__(self, ipAddr, port):
        super(MailBox, self).__init__()
        self.ipAddr = ipAddr
        self.port = port

    def __getattr__(self, name):
        return remoteMethod(name, self.ipAddr, self.port)

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

        s = socketPool[realAddrKey]

        msg = cPickle.dumps((self.port, self.name, args), -1)
        if len(msg) > 1472:
            print('msg more than mtu')
            return

        s.sendto(msg, realAddrKey)

def _unpickle_(info):
    return MailBox(*info)

socketPool = {}
entities = []
portOffset = 10000

def onMyAppRun(isBootstrap):

    print('onMyAppRun', isBootstrap)

    global portOffset
    for i in range(2):
        player = Tutorial.createEntity(avatar.Avatar)
        # player = Avatar('mark' + str(i), 'down', 32, '127.0.0.1', portOffset + i)
        global entities
        entities.append(player)
        print(player, type(player), player.id, player.name)
        realAddrKey = (player.ipAddr, player.port)
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.bind(realAddrKey)
        socketPool[realAddrKey] = s

def getEntityViaPort(port):
    global portOffset
    port -= portOffset
    global entities
    return entities[port]

def callEntityMethod(entity, methodName, args):
    func = getattr(entity, methodName)
    func(*args)

def onTimerUpdate(dt):
    global entities
    for box in entities:
        box.onUpdate(dt)

    global socketPool
    for realAddrKey in list(socketPool.keys()):
        socket = socketPool[realAddrKey]
        data, addr = socket.recvfrom(1472)
        datas = cPickle.loads(data)
        port, funcName, msg = datas
        entity = getEntityViaPort(port)
        callEntityMethod(entity, funcName, msg)
