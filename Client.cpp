// ---------------------------- Client.cpp ---------------------------
// Sean Eaton CSS503
// Robert Dimpsey
// Created: June 4th, 2020 
// Last Modified: June 5th, 2020
// -------------------------------------------------------------------
// Client file for transfering various messages via sockets over a TCP 
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
#include <cstring>
#include <chrono>
using namespace std;

int main(int argc, char *argv[]) {
    // validate arguments                                                                             
    if (argc < 7) {
        cerr << "usage: client server_name port repetition nbufs bufsize type" << endl;
        return -1;
    }

    //get type and repititions
    int repetitions = atoi(argv[3]);
    int type = atoi(argv[6]);

    // initiate buffer
    int nbufs = atoi(argv[4]);
    int bufsize = atoi(argv[5]);
    char data_buf[nbufs][bufsize];

    // get address info
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    struct addrinfo *results;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(argv[1], argv[2], &hints, &results) != 0) {
        cerr << "address get failed" << endl;
        return -1;
    }

    // create TCP socket and store descriptor
    int sock_fd;
    if ((sock_fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol)) == 0) { 
        cerr << "socket failed" << endl;
        return -1;
    }

    // connect socket to server address
    if (connect(sock_fd, results->ai_addr, results->ai_addrlen) < 0) {
        cerr << "connect failed" << endl;
        return -1;
    }

    // communicate number of repititions to server
    write(sock_fd, &repetitions, sizeof(int));

    // set up timer variables and start clock
    chrono::time_point<chrono::system_clock> start, throughput_end;
    start = chrono::system_clock::now(); 


    switch(type) { // determine and perform transfer type
        case 1 : // multiple transfer
            for (int i = 0; i < repetitions; i++) {
                for (int j = 0; j < nbufs; j++) {
                    write(sock_fd, data_buf[j], bufsize);  
                }
            }
            break;
        case 2 : // single writev transfer
            for (int i = 0; i < repetitions; i++) {
                struct iovec vector[nbufs];
                for (int j = 0; j < nbufs; j++) 
                {
                    vector[j].iov_base = data_buf[j];
                    vector[j].iov_len = bufsize;
                }
                writev(sock_fd, vector, nbufs);   
            }    
            break;
        case 3: // single transfer
            for (int i = 0; i < repetitions; i++) {
                write(sock_fd, data_buf, nbufs * bufsize);
            } 
            break;
    }
    
    // get total roundtrip time taken for all transfers, for throughput calculations
    throughput_end = chrono::system_clock::now(); 
    chrono::duration<double> throughput_elapsed_seconds = throughput_end - start; 

    // get roundtrip and reads
    int reads;
    read(sock_fd, &reads, sizeof(int));
 
    // output all data gathered
    cout << "------ results for test type " << type << " with " << nbufs << " buffers at size " << bufsize << ", " << repetitions << " repetitions ------" << endl;
    cout << "roundtrip time: " << throughput_elapsed_seconds.count() << "s" << endl;
    double throughput = (((((double)nbufs * (double)bufsize) * (double)repetitions) * (double)8) / throughput_elapsed_seconds.count()) / (double)1000000000;
    cout << "throughput: " << throughput << "Gbps" << endl;
    cout << "server made " << reads << " reads" << endl << endl;

    return 0;
}