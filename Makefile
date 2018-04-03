all:
	g++ -std=c++11 -o logDaemon logDaemon.cpp -lpthread
	#cat 0001-update.patch | ./a.out
