cpython

cd src/lib/python
./configure --with-cxx-main=g++ --enable-shared --prefix=/home/mattos/tutorial/src/libs
make
make install

libuv

sh autogen.sh
./configure --prefix=/home/mattos/tutorial/src/libs
make
make install


myApp

cd src
make

cd bin
./myapp