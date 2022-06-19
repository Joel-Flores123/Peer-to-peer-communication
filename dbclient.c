#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "msg.h"
#define _GNU_SOURCE

//to print out an error message if args are bad

void usage(char *progname);

//prototype for functoin that looks up a dns

int LookupName(char *name, unsigned short port,struct sockaddr_storage *ret_addr,size_t *ret_addrlen); 

//prototype for the function that connects a socket to a remote host

int Connect(const struct sockaddr_storage *addr,const size_t addrlen,int *ret_fd);

int main(int argc, char** argv)
{
	
	char* buffer;
	size_t buffsize = 128;

	//make sure we have the right amount of parameters

	if (argc != 3) 
	{
   	
		usage(argv[0]);
	
	}

	//get the port and error check

	unsigned short port = 0;
  
	if (sscanf(argv[2], "%hu", &port) != 1) 
	{
    
		usage(argv[0]);
 
       	}

	//prepare to get ip address
	
	struct sockaddr_storage addr;
	size_t addrlen;

	//look up the dns

 	if (!LookupName(argv[1], port, &addr, &addrlen)) 
	{
   
   	    	usage(argv[0]);
  
	}

  
	int socket_fd;
	int choice;//to get schoice
	struct msg mess; 

	while(1)
	{
	
		//feed the ip address, then length of the address and the soon to be fd into the connect function to connect to a remote host
		// Connect to the remote host.
		
		if (!Connect(&addr, addrlen, &socket_fd)) 
		{
		
			puts("connection failed");
			exit(EXIT_FAILURE);
		
		}
		
		//get user input
		
		printf("Enter your choice (1 to put, 2 to get, 3 to delete, 0 to quit): ");
		scanf("%d", &choice);
		getchar();
		
		if(choice == PUT)
		{
			
			//allocate buffer
			
			buffer = (char*) malloc(sizeof(char) * buffsize);

			//get the name
			
			printf("Enter the name: ");
			getline(&buffer, &buffsize, stdin);
			buffer[strlen(buffer) - 1] = ' ';
			//get id

			printf("Enter the id: ");
			scanf("%d", &choice);

			//put the name and the id into a message structure

			strcpy(mess.rd.name, buffer);
			mess.rd.id = choice;
			mess.type = PUT;
  
  			//send to socket

                        if(write(socket_fd, &mess, sizeof(struct msg)) == -1) //check for error
                        {

                                puts("write failed");
                                exit(EXIT_FAILURE);

                        }

                        //read from socket

                        if(read(socket_fd, &mess, sizeof(struct msg)) == -1) //check for errors
                        {

                                puts("read failed");
                                exit(EXIT_FAILURE);
                        
                        }
			
			//check if successful

                        if(mess.type == SUCCESS)
                        {

				puts("PUT success");
			
                        }
                        else
                        if(mess.type == FAIL)
                        {

                                puts("PUT failed"); //failed

                        }

			//printf("name: %s\n id: %d", mess.rd.name, mess.rd.id);
			//puts("");
			free(buffer); //free buffer no longer needed	

		}
		else
		if(choice == GET)
		{
		
			//get id

			printf("Enter id: ");
			scanf("%d", &choice);
			
			//set type and send id to server
			
			mess.type = GET;
			mess.rd.id = choice;

			//send to socket

			if(write(socket_fd, &mess, sizeof(struct msg)) == -1) //check for error
			{
			
				puts("write failed");
				exit(EXIT_FAILURE);

			}
			
			//read from socket
			
			if(read(socket_fd, &mess, sizeof(struct msg)) == -1) //check for errors
			{
				
				puts("read failed");
				exit(EXIT_FAILURE);
			
			}
			
			//check if successful

			if(mess.type == SUCCESS)
			{
			
				printf("name: %s\nid: %d\n", mess.rd.name, mess.rd.id); //print the information

			}
			else
			if(mess.type == FAIL)
			{
			
				puts("GET failed"); //failed
			
			}

		
		}
		else
		if(choice == DEL)
		{
			
			//get id

			printf("Enter id: ");
			scanf("%d", &choice);
			mess.type = DEL;
			mess.rd.id = choice;
                        
			//send to socket

                        if(write(socket_fd, &mess, sizeof(struct msg)) == -1) //check for error
                        {

                                puts("write failed");
                                exit(EXIT_FAILURE);

                        }

                        //read from socket

                        if(read(socket_fd, &mess, sizeof(struct msg)) == -1) //check for errors
                        {

                                puts("read failed");
                                exit(EXIT_FAILURE);
                        
                        }
		
			//check if successful

                        if(mess.type == SUCCESS)
                        {

				puts("DEL success");
			
                        }
                        else
                        if(mess.type == FAIL)
			{
			
				puts("DEL failed");
			
			}

		}
		else
		if(choice == 0)
		{
		
			break; //break if choice is 0
		
		}
		
		close(socket_fd); //end connection

	}


	return 0;

}


//to print out an error message if args are bad

void usage(char *progname) 
{

  	printf("usage: %s  hostname port \n", progname);
  	exit(EXIT_FAILURE);

}

//used to look up the ip address of a dns given the dns, a port number, a socket struct, and a return address, and a variable that will hold the length

int LookupName(char *name, unsigned short port,struct sockaddr_storage *ret_addr,size_t *ret_addrlen) 
{

	struct addrinfo hints, *results;
	int retval;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// Do the lookup by invoking getaddrinfo().
  
	if ((retval = getaddrinfo(name, NULL, &hints, &results)) != 0) 
  	{
  
		 printf( "getaddrinfo failed: %s", gai_strerror(retval));
   		 return 0;
  
  	}

  	// Set the port in the first result.
  	if (results->ai_family == AF_INET) 
  	{
  
		 struct sockaddr_in *v4addr = (struct sockaddr_in *) (results->ai_addr);
   		 v4addr->sin_port = htons(port);
 	} 
  	else 
 	if (results->ai_family == AF_INET6) 
	{
    
		struct sockaddr_in6 *v6addr = (struct sockaddr_in6 *)(results->ai_addr);
   		 v6addr->sin6_port = htons(port);
  
	} 
	else 
	{
    
		printf("getaddrinfo failed to provide an IPv4 or IPv6 address \n");
    		freeaddrinfo(results);
    		return 0;
  
	}

  	// Return the first result.
  	assert(results != NULL);
  	memcpy(ret_addr, results->ai_addr, results->ai_addrlen);
  	*ret_addrlen = results->ai_addrlen;

  	// Clean up.
  	freeaddrinfo(results);
  	return 1;
}

//connect the given socket

int Connect(const struct sockaddr_storage *addr,const size_t addrlen,int *ret_fd) 
{
  
	// Create the socket.
  
	int socket_fd = socket(addr->ss_family, SOCK_STREAM, 0);
  	
	if (socket_fd == -1) 
	{
    
		printf("socket() failed: %s", strerror(errno));
    		return 0;
  
	}

  	// Connect the socket to the remote host.
  
	int res = connect(socket_fd,(const struct sockaddr *)(addr),addrlen);
  
	if (res == -1) 
	{
    
		printf("connect() failed: %s", strerror(errno));
    		return 0;
  
	}

  	*ret_fd = socket_fd;
  	return 1;
}

