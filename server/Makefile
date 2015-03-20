CC = g++
all :	filesynd 
filesynd : server.o list_server.o static_tree.o MD5.o header.h
	$(CC) -o filesynd server.o list_server.o static_tree.o MD5.o -pthread
	
server.o : server.cpp header.h
	$(CC) -c server.cpp
	
list_server.o : list_server.cpp header.h
	$(CC) -c list_server.cpp 

static_tree.o : static_tree.cpp header.h
	$(CC) -c static_tree.cpp	

MD5.o : MD5.cpp header.h
	$(CC) -c MD5.cpp

clean :	
	rm -f filesynd server.o list_server.o static_tree.o MD5.o

	
