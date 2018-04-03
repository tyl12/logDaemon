all:
	g++ -std=c++11 -o logDaemon logDaemon.cpp -lpthread
	bash ./a.sh | ./logDaemon
