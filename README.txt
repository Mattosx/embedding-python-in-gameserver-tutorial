cd src/lib/python
./configure --with-cxx-main=g++ --enable-shared --prefix=$PRE_PATH/src/libs
make
make install