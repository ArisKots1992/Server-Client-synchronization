# Server Client file synchronization

Server-Client efficient file synchronization in C++, Used thread pools, hashtrees,
pipes, TCP/UDP sockets, Makefiles, with steady stream of requests to the
internet to avoid the Stop-And-Wait delay (for respond).

Clean with ```make clean```.

###Client:

Run with 
```
./filesync <server_ip> <server_port> <thread_pool_size> <queue_size> <tree_height> <tree_fanout> <directory_name>
```

*Make File Commands :*
```
g++ -c client.cpp
g++ -c list_client.cpp
g++ -c static_tree.cpp
g++ -c MD5.cpp
g++ -o filesync client.o list_client.o static_tree.o MD5.o -pthread
```

1. **MD5.cpp** : All the MD5 functions
2. **client.cpp** : Main client program
3. **header.cpp** : Defines & includes & functions headers
4. **list_client.cpp/.h** : Everything about my Clients Lists
5. **Makefile** : Dynamic Makefile & clean.
6. **static_tree.cpp/.h** : hash tree functions,insert,out,etc..

###Server:

Run with 
```
./filesynd <port> <thread_pool_size> <queue_size> <tree_height> <tree_fanout>
```
1. *MD5.cpp* : All the MD5 functions.
2. *server.cpp* : Main Server program.
3. *header.cpp* : Defines & includes & functions headers.
4. *list_server.cpp* : Everything about my Servers Lists
5. *Makefile* : Dynamic Makefile & clean.
6. *static_tree.cpp* : hash tree functions,insert,out,etc..

*Programmed in 2013*
