# 
# Keshav Verma, 2024csb1123
# Paras, 2024csb1139
#

PIN_ROOT ?= /home/keshavfire/pin-external-4.2-99776-g21d818fa2-gcc-linux

TARGET = memtrace
TOOL = memtrace.cpp

CXX = g++
CXXFLAGS = -Wall -O2 -fPIC -std=c++11
PIN_CXXFLAGS = -I$(PIN_ROOT)/source/include/pin \
               -I$(PIN_ROOT)/source/include/pin/gen \
               -I$(PIN_ROOT)/extras/components/include

PIN_LDFLAGS = -shared -Wl,--hash-style=sysv

all:
	$(CXX) $(CXXFLAGS) $(PIN_CXXFLAGS) $(PIN_LDFLAGS) \
	-o $(TARGET).so $(TOOL)

clean:
	rm -f *.so mem.trace