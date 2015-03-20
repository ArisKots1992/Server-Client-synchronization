CC = g++
all :	filesync 
filesync : client.o list_client.o static_tree.o MD5.o header.h
	$(CC) -o filesync client.o list_client.o static_tree.o MD5.o -pthread
	
client.o : client.cpp header.h
	$(CC) -c client.cpp
	
list_client.o : list_client.cpp header.h
	$(CC) -c list_client.cpp 

static_tree.o : static_tree.cpp header.h
	$(CC) -c static_tree.cpp	

MD5.o : MD5.cpp header.h
	$(CC) -c MD5.cpp

clean :	
	rm -f filesync client.o list_client.o static_tree.o MD5.o

	
