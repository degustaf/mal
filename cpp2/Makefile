CXX=g++

CXXFLAGS=-O3 -Wall -Wextra -std=c++11
LDFLAGS=-O3 -L.

MAINS=$(wildcard step*.cpp)
TARGETS=$(MAINS:%.cpp=%)

$(TARGETS): %: %.o 
	$(CXX) $^ -o $@ $(LDFLAGS)
