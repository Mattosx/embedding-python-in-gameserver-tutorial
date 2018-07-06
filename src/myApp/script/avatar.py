# -*- coding: utf-8 -*-
import random
import Tutorial

class Avatar(Tutorial.Entity):

    # 在createEntity创建申请对象空间后, 直接无参数调用了__init__
    def __init__(self):
        super(Avatar, self).__init__()
        self.ipAddr = '127.0.0.1'
        self.port = 0

    def setAddrPort(self, addr, port):
        self.ipAddr = addr
        self.port = port

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

class MailBox(object):
    def __init__(self, ipAddr, port, id):
        super(MailBox, self).__init__()
        self.ipAddr = ipAddr
        self.port = port
        self.id = id

    def __getattr__(self, name):
        return remoteMethod(name, self.ipAddr, self.port, self.id)

class remoteMethod(object):
    def __init__(self, name, ipAddr, port, id):
        super(remoteMethod, self).__init__()
        self.name = name
        self.ipAddr = ipAddr
        self.port = port
        self.id = id

    def __call__(self, *args):
        realAddrKey = (self.ipAddr, self.port)
        s = Tutorial.socketFd
        msg = cPickle.dumps((self.name, self.id, args), -1)
        if len(msg) > 1472:
            print('msg more than mtu, please split it')
            return
        s.sendto(msg, realAddrKey)

def _unpickle_(info):
    return MailBox(*info)