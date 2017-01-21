CC:=g++
CFLAGS1=-Isrc/util
CFLAGS1+=-c
CFLAGS2:=-Isrc/kernel 
CFLAGS2+=-Isrc/util
CFLAGS2+=-lpthread
CFLAGS2+=-c
TARGET1:=bin/pagerank
TARGET2:=bin/shortpath
TARGET3:=bin/katz
TARGET4:=bin/adsorption
DEPEND:=obj/static-initializers.o
DEPEND+=obj/stringpiece.o
DEPEND+=obj/common.o
DEPEND1:=obj/pagerank.o
DEPEND2:=obj/shortestpath.o
DEPEND3:=obj/katz.o
DEPEND4:=obj/adsorption.o

$(TARGET1):$(DEPEND) $(DEPEND1)
	$(CC) -o $@  $^ -lpthread 

obj/%.o:src/util/%.cc
	$(CC) -o $@ $(CFLAGS1) $^

$(DEPEND1):src/examples/pagerank.cpp
	$(CC) -o $@ $(CFLAGS2) $^ 

$(TARGET2):$(DEPEND) $(DEPEND2)
	 $(CC) -o $@  $^ -lpthread 

$(DEPEND2):src/examples/shortestpath.cpp
	$(CC) -o $@ $(CFLAGS2) $^ 

$(TARGET3):$(DEPEND) $(DEPEND3)
	 $(CC) -o $@  $^ -lpthread 

$(DEPEND3):src/examples/katz.cpp
	$(CC) -o $@ $(CFLAGS2) $^

$(TARGET4):$(DEPEND) $(DEPEND4)
	 $(CC) -o $@  $^ -lpthread 

$(DEPEND4):src/examples/adsorption.cpp
	$(CC) -o $@ $(CFLAGS2) $^

all:bin/pagerank  bin/shortpath  bin/katz  bin/adsorption

.PHONY:clean

clean:
	rm -rf obj/*  bin/*
			  
