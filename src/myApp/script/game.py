# -*- coding: utf-8 -*-
import Tutorial

import socket
import _pickle as cPickle
import random
import json
import errno, os

import avatar

def onMyAppRun(isBootstrap):
    Tutorial.entities = dict()
    f = open('serverconf.json', 'r', encoding='utf-8');
    serverconf = json.loads(f.read(), encoding="utf-8")
    serverconf = serverconf["servers"]
    serverNodeName = Tutorial.serverNodeName
    print('onMyAppRun', isBootstrap, serverNodeName)
    if serverNodeName not in serverconf:
        return
    listenAddr = serverconf[serverNodeName]['address']
    listenPort = serverconf[serverNodeName]['port']
    sfd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sfd.setblocking(False)
    sfd.bind((listenAddr, listenPort))

    Tutorial.socketFd = sfd

    player = Tutorial.createEntity(avatar.Avatar)
    if not player:
        print('Tutorial.createEntity return None')
        return
    player.setAddrPort(listenAddr, listenPort)
    Tutorial.entities[player.id] = player

def callEntityMethod(entity, methodName, args):
    func = getattr(entity, methodName)
    func(*args)

def callRemoteEntityMethod(remoteHostAddr, remotePort, entityId, methodName, methodArgs):
    s = Tutorial.socketFd
    msg = cPickle.dumps((methodName, entityId, methodArgs), -1)
    if len(msg) > 1472:
        print('msg more than mtu, please split it')
        return
    s.sendto(msg, (remoteHostAddr, remotePort))

def onDispatchEntityMsg():
    socket = Tutorial.socketFd
    try:
        datas, addr = socket.recvfrom(1472)
    except:
        return

    args = cPickle.loads(datas)
    methodName, id, msg = args
    print(args)
    entity = Tutorial.entities.get(id)
    if not entity:
        return
    callEntityMethod(entity, methodName, msg)
