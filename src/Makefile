
RM=rm -f

VERSION:=$(shell date "+%F_%T")
COMILE_OPT=
MACROS=-D__BUILD_VER__="\"V1.0 $(VERSION)\""


ifdef debug
CC=g++ -g -W -Wall $(MACROS) $(COMILE_OPT)
else
#-s :strip symbols 
CC=g++ -s -O3 -W -Wall $(MACROS) $(COMILE_OPT)
endif


prog:=bin_test
inc_dir=-I../include -I.
lib_dir=-L../libs
libs=-lcrypto -ldl -lpthread -lrt

test_sock_bin:=bin_sock
test_sock_srcs=misc.cpp $(wildcard sock/*.cpp)
test_sock_objs=$(test_sock_srcs:.cpp=.o)

prog_all:=$(prog) $(test_sock_bin)

srcs=$(wildcard *.cpp) $(wildcard payloads/*.cpp) $(wildcard rtp/*.cpp) $(wildcard sock/*.cpp) \
	$(wildcard encoding/*.cpp) 
objs=$(srcs:.cpp=.o)

default:$(prog)
.PHONY:default

$(prog):$(objs)
	$(CC) -o $@ $^ $(lib_dir) $(libs)

$(objs):%.o:%.cpp
	$(CC) -c -o $@ $< $(inc_dir)	

pre_test:export COMILE_OPT+=-D__TEST__
pre_test:$(test_sock_bin)
.PHONY:pre_test

test_sock:pre_test
.PHONY:test_sock

$(test_sock_bin):$(test_sock_objs)
	$(CC) -o $@ $^ $(lib_dir) $(libs)
		
clean:
	-@$(RM)  $(objs) $(prog_all)
.PHONY:clean


