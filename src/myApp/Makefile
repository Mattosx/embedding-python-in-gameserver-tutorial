PLATFORM=$(shell uname)
CC = g++

DEBUG_FLAG = -O1
CXXFLAGS  = -c $(DEBUG_FLAG) -Wall -Werror=declaration-after-statement -std=c99 -pedantic -fPIC
LIBS = -lpython3.6m -lpthread -ldl -lutil -luv
LDFLAGS = $(DEBUG_FLAG) -Wall -L../libs/lib $(LIBS) -Wl,-rpath,../src/libs/lib

BIN = ../../bin/myapp
TEST_OBJS = main.o\
python_server.o\
entity.o\
format.o

all : $(BIN)

$(BIN): $(TEST_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) 

$(TEST_OBJS) : %.o : %.cpp
	$(CC) -o $@ $(CXXFLAGS) $< -I../libs/include/python3.6m -I../libs/include

clean : 
	rm -f $(TEST_OBJS) $(BIN)

.PHONY : clean

