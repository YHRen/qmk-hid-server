CXX=g++
all: main.cpp
	$(CXX) -std=c++17 -O3 main.cpp -o main -pthread

