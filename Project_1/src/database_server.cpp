/**********************************************************************
 *
 * CS542 Database Management Systems
 *
 * Written by: Tyler Carroll, James Silvia, Tom Strott
 * In completion of: CS542 Project 1
 *
 * database_server.cpp
 *
 **********************************************************************/

#include "database_server.hpp"

#include "memory_manager.hpp"
#include "helpers.hpp"

//The backlog for listening
#define BACKLOG 5

//The buffer length for receiving commands
#define BUFFER_LEN 1024

//Set to false to disable
bool verbose = true;
//Verbose printing mode
void printv(char *format, ...) {

    if(verbose == false)
        return;

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}


// Handles a client connection
bool handleclient(const int socket) {
    char buffer[BUFFER_LEN + 1] = {0}; //Allow for null.
    int len;
    string cmd, key, data, to_send;

    while (true) {

        //Clear receive buffer
        memset(buffer, 0, sizeof(buffer));
        //Clear strings
        cmd.clear();
        key.clear();
        data.clear();
        to_send.clear();
        
        //Shall we get some data?
        len = read(socket, buffer, BUFFER_LEN);

        //Some sort of error
        if (len < 0) {
            cerr << "Unable to read from client socket!" << endl;
            break;
        }

        //Disconnected
        if (len == 0) {
            cerr << "The client disconnected!" << endl;
            break;
        }

        printv("Received: %s\n", buffer);
        //Stringstream of received data
        stringstream strstr(buffer);
        if (!(strstr >> cmd))
            continue;

        /* Put request received */
        if (cmd == "put") {
            // Let's get the info we need
            strstr >> key;
            while (strstr >> data) {
                // Get all data in
                (to_send.empty()) ?
                    to_send = data :
                    to_send = to_send + " " + data;
            }
            // TODO: Add FD for comms
            table.put(atoi(key.c_str()), to_send, 0);
            to_send.clear();
            //Put the entry
            to_send = "We got your PUT request brah";
        } 
        /* Get request received */
        else if (cmd == "get") {
            // Let's get the info we need
            strstr >> key;
            // TODO: Add FD for comms
            table.get(atoi(key.c_str()), 0);
            //Get the entry
            to_send = "We got your GET request brah";
        } 
        /* Remove request received */
        else if (cmd == "remove") {
            // Let's get the info we need
            strstr >> key;
            // TODO: Add FD for comms
            table.remove(atoi(key.c_str()), 0);
           //Remove the entry
           to_send = "We got your REMOVE request brah";
        }
        /* Smoothly close socket, will slam all clients */
        else if (cmd == "close") {
            // Server close?
            strstr >> data;
            // Yes, lets exit.
            if (data == "server") {
                cout << "Received force close, EXIT" << endl;
                close(socket);
                exit(1);
            }
        }
        /* Print queue - verbose only */
        else if (cmd == "print" && verbose) {
            table.print_queue();
            to_send = "We got your PRINT request brah";
        }
        /* Junk catch all */
        else {
            continue;
        }

        //Send the data
        len = write(socket, to_send.c_str(), to_send.length());
        if (len != to_send.length())
            error("Unable to send the data!");
    }

    return true;
}


//functino to handle new client as a new thread
void *handleclient_thread(void *newsock ){

    int *sock = (int*)(newsock);
    
    handleclient(*sock);

    close(*sock);
}


//Main function!
int main(int argc, char *argv[])
{
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    pthread_t thread;

    //Create memory manager for database
    Memory_manager memory_manager("database");
    int current_size = memory_manager.load_memory_map();
    memory_manager.map_to_memory(current_size);

    //Create the sock handle
    printv("Creating socket\n");
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        error("Unable to create sock!");
    
    //Clear the structures just incase
    memset(&cli_addr,  0, sizeof(cli_addr));
    memset(&serv_addr, 0, sizeof(serv_addr));
    
    //Fill in the server structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    
    //Bind the socket to a local address
    printv("Binding socket\n");
    if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("Unable to bind sock!");

    //Begin listening on that port
    if (listen(sock, BACKLOG) < 0)
        error("Unable to listen on sock!");

    bool soldierOn = true;
    while (soldierOn)
    {
        //Accept a new connection!
        printv("Listening for connections...\n");
        int newsock = accept(sock, (struct sockaddr *)&cli_addr, &clilen);
        if (newsock < 0)
            error("Unable to accept connection!");

        //Start a new thread to handle clients
        printv("Connection made! Starting new thread...\n");
        if(pthread_create(&thread, NULL, handleclient_thread, (void*)&newsock) < 0)
            error("Unable to create new thread!\n");

    }

    //All done.
    memory_manager.unmap_from_memory();
    return 0;
}
