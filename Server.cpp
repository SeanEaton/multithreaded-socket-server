// --------------------------- Server.cpp ----------------------------
// Sean Eaton CSS503
// Robert Dimpsey
// Created: June 4th, 2020 
// Last Modified: June 5th, 2020
// -------------------------------------------------------------------
// Server file for recieving various messages via sockets over a TCP 
// connection
// -------------------------------------------------------------------
#include <unistd.h> 
#include <stdio.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netdb.h>
#include <stdlib.h>
#include <iostream> 
#include <pthread.h>    
using namespace std;

#define BUFSIZE 1500    


void *thread_func(void *arg) { 
    // store this thread's socket descriptor
    int my_sock = *((int *)arg);
    free(arg); 

    // get number of repetitions from client
    int repetitions;
    read(my_sock, &repetitions, sizeof(int));

    // start reading tests
    char read_buf[BUFSIZE] = {0};
    int total_bytes_read = 0;
    int bytes_read = 0;
    int reads = 0;
    for (int i = 0; i < repetitions; i++) {
        while (total_bytes_read < BUFSIZE) {
            bytes_read = read(my_sock, read_buf, BUFSIZE - total_bytes_read);
            total_bytes_read += bytes_read;
            reads++;
        }
        total_bytes_read = 0;
        bytes_read = 0;
    }

    // report reads back to client and close socket
    write(my_sock, &reads, sizeof(int));
    cout << "terminating connection at socket " << my_sock << endl;
    close(my_sock);
    pthread_exit(0);
} 

int main(int argc, char *argv[]) {
    // validate arguments                                                                             
    if (argc < 2) {
        cerr << "usage: server port" << endl;
        return -1;
    }

    if (atoi(argv[1]) > 65535) {
        cerr << "port number exceeds limit (65535)";
        return -1;
    }
    int sock_fd;

    // create TCP socket, store descriptor
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
        cerr << "socket failed" << endl;
        return -1;
    }

    // set socket option
    const int opt = 1;
 	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int)) < 0) {
        cerr << "socket option set failed" << endl;
        return -1;
    }

    // bind to specified port on this machine
    struct sockaddr_in addr; 
    int addrlen = sizeof(addr); 
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = INADDR_ANY; 
    addr.sin_port = htons( atoi(argv[1]) );
    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        cerr << "socket bind failed" << endl;
        return -1;
    }

    // set listening limit
    if (listen(sock_fd, 5) < 0) {
        cerr << "listen failed" << endl;
        return -1;
    }

    // begin waiting for connections
    struct sockaddr_in client_addr; 
    int client_addrlen = sizeof(client_addr); 
    int i;
    while (1) {
        int *new_sock = new int;
        *new_sock = accept(sock_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addrlen);
        if (new_sock > 0) { // create new thread for this socket connection
            pthread_t tid;
            pthread_attr_t tattr;
            pthread_attr_init(&tattr);
            pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
            cout << "connection made at socket " << *new_sock << endl;
            pthread_create(&tid, &tattr, thread_func, (void *)new_sock);
        }
    }
    
    return 0;
}