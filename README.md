# embedding-python-in-gameserver-tutorial
>a tutorial for embedding-python-in-gameserver

"/home/mattos/tutorial" is the clone path

## build cpython

cd tutorial/src/lib/python

./configure --with-cxx-main=g++ --enable-shared --prefix=/home/mattos/tutorial/src/libs

make

make install

## build libuv

sh autogen.sh

./configure --prefix=/home/mattos/tutorial/src/libs

make

make install

## build myApp

cd tutorial/src

make

cd tutorial/bin

./myapp