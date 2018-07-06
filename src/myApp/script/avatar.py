# -*- coding: utf-8 -*-
import random
import Tutorial

class Avatar(Tutorial.Entity):

    # 在createEntity创建申请对象空间后, 直接无参数调用了__init__
    def __init__(self):
        super(Avatar, self).__init__()
        print('Avatar.__init__', self.id, self.name)
        self.ipAddr = '127.0.0.1'
        self.port = 0

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