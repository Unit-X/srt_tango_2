GCC = gcc 
CXX = g++
CXXFLAG = -Wall -g -std=gnu++11


AMF0_OBJS = amf0.o simple_buffer.o amf0_test.o

all: amf0_test

amf0_test: $(AMF0_OBJS)
	$(CXX) -o amf0_test $(CXXFLAG) $(AMF0_OBJS)

amf0.o: amf0.h
	$(CXX) -c $(CXXFLAG) amf0.cpp -o amf0.o

simple_buffer.o: simple_buffer.h 
	$(CXX) -c $(CXXFLAG) simple_buffer.cpp -o simple_buffer.o

amf0_test.o: amf0.h simple_buffer.h 
	$(CXX) -c $(CXXFLAG) test.cpp -o amf0_test.o

clean :
	rm amf0_test $(AMF0_OBJS)
