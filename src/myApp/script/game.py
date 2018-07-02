# -*- coding: utf-8 -*-
import Tutorial

import socket
import _pickle as cPickle

import random

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

class Avatar(Tutorial.Entity):
    def __init__(self, ipAddr, port):
        super(Avatar, self).__init__()
        self.ipAddr = ipAddr
        self.port = port

    def onUpdate(self, dt):
        r = random.random()
        if r < 2:#allways
            avatar = self.pickRandomAvatar()
            if avatar:
                box = avatar.getMailBox()
                box.sayHello(self, 'hello')

    def pickRandomAvatar(self):
        ret = None
        global entities
        for entity in entities:
            if entity.port == self.port:
                continue
            ret = entity

        return ret

    def sayHello(self, fBox, hello):
        print(self.port, 'recv', hello, 'From', fBox.port)
        fBox.onSayHelloSucc(self)

    def onSayHelloSucc(self, fBox):
        print(self.port, 'recv', 'onSayHelloSucc', 'From', fBox.port)

    def getMailBox(self):
        v = self.__reduce_ex__(None)
        return v[0](v[1][0])

    def __reduce_ex__(self, args):
        return (_unpickle_, ((self.ipAddr, self.port),))

socketPool = {}
entities = []
portOffset = 10000

def onMyAppRun(isBootstrap):

    print('onMyAppRun', isBootstrap)

    global portOffset
    for i in range(2):
        player = Tutorial.createEntity(Avatar)
        print(player.id)
        # player = Avatar('mark' + str(i), 'down', 32, '127.0.0.1', portOffset + i)
        global entities
        entities.append(player)

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
