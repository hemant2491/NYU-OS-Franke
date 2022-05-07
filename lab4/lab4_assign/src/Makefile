CFLAGS=-g # -g = debug, -O2 for optimized code CXXFLAGS for g++
# CC=gcc-9.2.0
# To-do: how to load 9.2 in Makefile
CC=g++

sched: iosched.cpp
	$(CC) $(CFLAGS) -o iosched iosched.cpp

clean:
	rm -f sched *~
