/*
Info: Network based card0 changer. Will listen on a socket specified by user for a card0 information.
Compiler: MinGW g++ 5.3.1 (Ubuntu 16.04)
Flags: -std=c++11 -static-libgcc -static-libstdc++ -lws2_32
Run ex: nCard0.exe -p1="G:\card0.txt" -p2="H:\card0.txt" -c1="G:\check.txt" -c2="H:\check.txt" -p="4500"

TODO:
Dump debug message to log file
*/

#include <iostream>
#include <fstream>
#include <string>
#include "argh.h"
#include <winsock2.h>
#include <stdio.h>
#include <regex>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

void exampleMessage();
void debugOutput(std::string);
bool checkExist(std::string);
bool overwriteCard(std::string, std::string);

int main (int argc, char* argv[])
{
    std::string card_p1, card_p2, check_p1, check_p2; // card = card0.txt ; check = check.txt
    int port = 0; // port number to listen

    argh::parser cmdl; // command line parser
    cmdl.parse(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);
    std::cout << std::endl; // space out for formatting

    if(cmdl["-h"]) // help
    {
        exampleMessage();
    }

    for(auto& param: cmdl.params())
    {
        if(!param.first.compare("p1")) // card0 p1
        {
            card_p1 = param.second;
        }
        if(!param.first.compare("p2")) // card0 p2
        {
            card_p2 = param.second;
        }
        if(!param.first.compare("c1")) // checker p1
        {
            check_p1 = param.second;
        }
        if(!param.first.compare("c2")) // checker p2
        {
            check_p2 = param.second;
        }
        if(!param.first.compare("p")) // port
        {
            try 
            {
                port = stoi(param.second); // convert to int
            }
            catch (std::invalid_argument) // failed convert
            {
                exampleMessage();
            }
        }
    }

    if(card_p1 == "" || port < 1 || port > 65535) // failed validation
        exampleMessage();

    debugOutput("Card P1:\t" + card_p1 + "\n");
    debugOutput("Card P2:\t" + card_p2 + "\n");
    debugOutput("Check P1:\t" + check_p1 + "\n");
    debugOutput("Check P2:\t" + check_p2 + "\n");
    debugOutput("Port\t\t" + std::to_string(port) + "\n");

    // begin socket work - copy and pasted below (http://www.binarytides.com/code-tcp-socket-server-winsock/)
    WSADATA wsa;
    SOCKET master , new_socket , client_socket[5] , s;
    struct sockaddr_in server, address;
    int max_clients = 5 , activity, addrlen, i, valread;
     
    //size of our receive buffer, this is string length.
    int MAXRECV = 18;
    //set of socket descriptors
    fd_set readfds;
    //1 extra for null character, string termination
    char *buffer;
    buffer =  (char*) malloc((MAXRECV + 1) * sizeof(char));
 
    for(i = 0 ; i < 5;i++)
    {
        client_socket[i] = 0;
    }
 
    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        exit(EXIT_FAILURE);
    }
     
    printf("Initialised.\n");
     
    //Create a socket
    if((master = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }
 
    printf("Socket created.\n");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );
     
    //Bind
    if( bind(master ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }
     
    puts("Bind done");
 
    //Listen to incoming connections
    listen(master , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
     
    addrlen = sizeof(struct sockaddr_in);
     
    while(TRUE)
    {
        //clear the socket fd set
        FD_ZERO(&readfds);
  
        //add master socket to fd set
        FD_SET(master, &readfds);
         
        //add child sockets to fd set
        for (  i = 0 ; i < max_clients ; i++) 
        {
            s = client_socket[i];
            if(s > 0)
            {
                FD_SET( s , &readfds);
            }
        }
         
        //wait for an activity on any of the sockets, timeout is NULL , so wait indefinitely
        activity = select( 0 , &readfds , NULL , NULL , NULL);
    
        if ( activity == SOCKET_ERROR ) 
        {
            printf("select call failed with error code : %d" , WSAGetLastError());
            exit(EXIT_FAILURE);
        }
          
        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master , &readfds)) 
        {
            if ((new_socket = accept(master , (struct sockaddr *)&address, (int *)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
          
            //inform user of socket number - used in send and receive commands
            printf("New connection - socket fd: %d , ip: %s , port: %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
           
            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++) 
            {
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets at index %d \n" , i);
                    break;
                }
            }
        }
          
        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++) 
        {
            s = client_socket[i];
            //if client presend in read sockets             
            if (FD_ISSET( s , &readfds)) 
            {
                //get details of the client
                getpeername(s , (struct sockaddr*)&address , (int*)&addrlen);
 
                //Check if it was for closing , and also read the incoming message
                //recv does not place a null terminator at the end of the string (whilst printf %s assumes there is one).
                valread = recv( s , buffer, MAXRECV, 0);
                 
                if( valread == SOCKET_ERROR)
                {
                    int error_code = WSAGetLastError();
                    if(error_code == WSAECONNRESET)
                    {
                        //Somebody disconnected , get his details and print
                        printf("Host disconnected unexpectedly , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                      
                        //Close the socket and mark as 0 in list for reuse
                        closesocket( s );
                        client_socket[i] = 0;
                    }
                    else
                    {
                        printf("recv failed with error code : %d" , error_code);
                    }
                }
                if ( valread == 0)
                {
                    //Somebody disconnected , get his details and print
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                      
                    //Close the socket and mark as 0 in list for reuse
                    closesocket( s );
                    client_socket[i] = 0;
                }
                  
                //Echo back the message that came in
                else
                {
                    //add null character, if you want to use with printf/puts or other string handling functions
                    buffer[valread] = '\0';
                    
                    // the 'buffer' is the string to use to edit our card0.txt file
                    std::regex cardRegex("[1,2]:[e,E]004[(0-9)(a-f)(A-F)]{12}"); // 16 digit hexadecimal starting with e004; ex: 1:e004abcdef123456
                    if(std::regex_match(buffer, cardRegex)) 
                    {
                        
                        printf("\nRecieved by: %s:%d contains: %s \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port), buffer);
                        debugOutput("Data recieved is a valid game card\n");

                        // validate if buffer string is for p1 or p2
                        if(buffer[0] == '1') //p1
                        {
                            debugOutput("Looking for P1 check file in: " + check_p1 + "... ");
                            if(check_p1 != "" && checkExist(check_p1))
                            {
                                debugOutput("Found!\n");        
                                overwriteCard(buffer, card_p1);
                            }
                            else if(check_p1 == "") // forced overwrite without check file
                            {
                                debugOutput("No check file found!\nOverwriting card regardless\n");
                            }
                            else // refuse overwrite
                            {
                                debugOutput("No check file found!\nSkipping overwrite of card0\n"); 
                            }
                        }
                        else //p2
                        {
                            debugOutput("Looking for P2 check file in: " + check_p2 + "... ");
                            if(check_p2 != "" && checkExist(check_p2))
                            {
                                debugOutput("Found!\n");

                                overwriteCard(buffer, card_p2);
                            }
                            else if(check_p2 == "") // forced overwrite without check file
                            {
                                debugOutput("No check file found!\nOverwriting card regardless\n");
                                overwriteCard(buffer, card_p2);
                            }
                            else // refuse overwrite
                            {
                                debugOutput("Not found!\nSkipping overwrite of card0\n");
                            }
                        }
                    }
                    else // invalid data
                    {
                        std::string bufferS(buffer); // fix this ghetto way to send the data
                        debugOutput("Invalid data recieved: " + bufferS);
                    }
                    //send( s , "recieved" , valread , 0 );
                    
                    // close socket after recieving information, one send per request
                    closesocket( s );
                    client_socket[i] = 0;
                }
            }
        }
    }
     
    closesocket(s);
    WSACleanup();

    return 0;
}

void exampleMessage() // help menu
{
    std::cout << "Availble options:" << std::endl;
    std::cout << "  -p1\tLocation of player 1's card0.txt file" << std::endl;
    std::cout << "  -p2\tLocation of player 2's card0.txt file" << std::endl;
    std::cout << "  -c1\tLocation of player 1's check file" << std::endl;
    std::cout << "  -c2\tLocation of player 2's check file" << std::endl;
    std::cout << "  -p\tPort number to listen on (1-65535)" << std::endl;
    std::cout << std::endl << "Ex (1P): nCard0.exe -p1=\"G:\\card0.txt\" -c1=\"G:\\check.txt\" -p=\"4500\"" << std::endl;
    std::cout << "Ex (2P): nCard0.exe -p1=\"G:\\card0.txt\" -p2=\"H:\\card0.txt\" -c1=\"G:\\check.txt\" -c2=\"H:\\check.txt\" -p=\"4500\"" << std::endl;
    exit(0);
}

void debugOutput(std::string text) // debugging text output
{
    std::cout << text;
}

bool checkExist (std::string fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

bool overwriteCard(std::string buffer, std::string location)
{
    // split card up from data
    std::string card (buffer);
    card = card.substr(2, 16); // remove player number and colon (1:) to produce card number only

    // check if card exists
    debugOutput("Card data: " + card + "\n");

    // try to write card file
    try
    {
        std::ofstream cardFile(location);
        if(cardFile.is_open())
        {
            cardFile << card;
            cardFile.close();
            debugOutput("Successfully written card number to: " + location + "\n");
        }
        else
        {
            debugOutput("Unable to write to card in: " + location + "\n");
        }
    }
    catch (std::ofstream::failure &e)
    {
        std::string errorMsg(e.what());
        debugOutput("An error occured when trying to write file: " + errorMsg);
    }
    
}