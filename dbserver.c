#include <fcntl.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "msg.h"
#include <pthread.h>
#include <sys/mman.h>

void usage(char *progname);

//prototype for our listen function that will find a socket for listening

int Listen(char *portnum, int *sock_family);

//prototype for handling client

void* handleClient(void *arg);

int main(int argc, char** argv)
{
	
	// Expect the port number as a command line argument.
  
	if (argc != 2) 
	{
    
		usage(argv[0]);
  
	}

	 int sock_family;
	 int listen_fd = Listen(argv[1], &sock_family);
 	
	 if (listen_fd <= 0) 
 	 {
  
		 // We failed to bind/listen to a socket.  Quit with failure.
   	 	printf("Couldn't bind to any addresses.\n");
    
		return EXIT_FAILURE;
  
	}
	
	int rc;
	pthread_t p1;

	int fd = open("file.txt", O_CREAT | O_RDWR, 0666);
	close(fd); //close file after creating 

	 // Loop forever, accepting a connection from a client and doing
 	// an echo trick to it.
	
	while (1) 
	{
    
		struct sockaddr_storage caddr;
   		socklen_t caddr_len = sizeof(caddr);
   	 	int client_fd = accept(listen_fd,(struct sockaddr *)(&caddr), &caddr_len);
  		

		if (client_fd < 0) 
		{
      		
			if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
       				 continue;
    			
			printf("Failure on accept:%s \n ", strerror(errno));
    		  	break;
    
		}
		
		rc = pthread_create(&p1, NULL, handleClient, &client_fd);

		if(rc == -1 )
		{
		
			puts("thread failed");
		
		}

	}

		
	return 0;

}

//handles client

void* handleClient(void *arg)
{
	
	int fd = *((int*)arg); //cast 
	struct msg mess;	
	int file = open("file.txt", O_RDWR); //open the file 
	struct record rec = {"", 0, ""};
	size_t lastOffset = 0;	

	//if there is stuff to read from socket

	if(read(fd, &mess, sizeof(struct msg)) > 0 )
	{

		if(mess.type == PUT) //if put request
		{
			
			//while no matching id and we are still within the files bounds
			

			while(read(file, &rec, sizeof(struct record)) > 0)
			{

				//look for deleted cell

				if((strcmp(rec.name, "") == 0 && lockf(file, F_TEST, -sizeof(struct record)) != -1))
				{
			
					break;

				}
				
				lastOffset = lseek(file, 0, SEEK_CUR); //get the last offset
				
			}
			
			lseek(file, lastOffset,SEEK_SET); //seek to the last offset
			lockf(file, F_LOCK, sizeof(struct record));

			//write to file 

                    	if(write(file, &mess.rd, sizeof(struct record))  <= 0)
                        {

                           	puts("couldnt write to database");
                       	        mess.type = FAIL;

                     	}

                        if(lockf(file, F_ULOCK, -sizeof(struct record)) == -1)
                        {

       	                        puts("couldnt unlock");
              	                mess.type = FAIL;

                       	}

                        if(mess.type != FAIL)
                        {

          	                mess.type = SUCCESS;

       	                }

                      	if(write(fd, &mess ,sizeof(struct msg) <= 0))
                        {

            	                puts("dang couldnt communicate with socket L"); //debug

                        }			
			

		}
		else
		if(mess.type == GET || mess.type == DEL) //if a get request
		{
			
			lastOffset = lseek(file, 0, SEEK_CUR); //get cur 
			int found = 0;

			//while no matching id and we are still within the files bounds
                        
			while(read(file, &rec, sizeof(struct record)) > 0)
                        {

                                if(rec.id == mess.rd.id)
                                {
					
					found = 1;
                                        break;

                                }

                                  lastOffset = lseek(file, 0, SEEK_CUR); //get the last offset

                        }

			if(rec.id == mess.rd.id || found == 1) //if found check if delete or get request
			{
				
				if(mess.type == GET) //if it was a get request
				{
					
					if(strcmp(rec.name, "") != 0) //if wasnt deleted
					{
					
						strcpy(mess.rd.name,rec.name); //copy name into mess.rd.name
						mess.rd.id = rec.id;
						mess.type = SUCCESS;
						write(fd,&mess, sizeof(struct msg));
					
					}
					else
					{
						
						mess.type = FAIL;
						write(fd,&mess, sizeof(struct msg));
					
					}

				}
				else //otherwise it was a delete request
				{
					
					if(strcmp(rec.name, "") != 0)
					{
					
						strcpy(rec.name, ""); //mark as deleted
						lseek(file, lastOffset, SEEK_SET);
					
						if(write(file, &rec, sizeof(struct record)) == -1)
						{
						
							mess.type = FAIL;
	
						}
						else
						{
					
							mess.type = SUCCESS;
					
						}
					}
					else
					{
					
						mess.type = FAIL;
					
					}

					write(fd,&mess, sizeof(struct msg));

				}

			}
			else //other send fail message
			{
			
				//send failure message

				mess.type = FAIL;
				write(fd,&mess, sizeof(struct msg));

			}
		
		}
		
	
	}
	
	close(fd);
	close(file);
	return NULL;
	
}

//does listening 

int Listen(char *portnum, int *sock_family) 
{

  	// Populate the "hints" addrinfo structure for getaddrinfo().
 	 // ("man addrinfo")
	 
  	struct addrinfo hints;
 	memset(&hints, 0, sizeof(struct addrinfo));
  	hints.ai_family = AF_INET;       // IPv6 (also handles IPv4 clients)
  	hints.ai_socktype = SOCK_STREAM;  // stream
	hints.ai_flags = AI_PASSIVE;      // use wildcard "in6addr_any" address
  	hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
 	hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
 	hints.ai_canonname = NULL;
  	hints.ai_addr = NULL;
	hints.ai_next = NULL;

  	// Use argv[1] as the string representation of our portnumber to
  	// pass in to getaddrinfo().  getaddrinfo() returns a list of
 	 // address structures via the output parameter "result".
  	
  	struct addrinfo *result;
  	int res = getaddrinfo(NULL, portnum, &hints, &result);

  	// Did addrinfo() fail?
  
	if (res != 0) 
	{
	
		printf( "getaddrinfo failed: %s", gai_strerror(res));
   		 return -1;
  
	}

	  // Loop through the returned address structures until we are able
  	// to create a socket and bind to one.  The address structures are
 	 // linked in a list through the "ai_next" field of result.
  
	int listen_fd = -1;
	struct addrinfo *rp;
	
  	for (rp = result; rp != NULL; rp = rp->ai_next) 
	{
 
	     	listen_fd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
    		
		if (listen_fd == -1) 
		{
      		
			// Creating this socket failed.  So, loop to the next returned
     			 // result and try again.
      
			printf("socket() failed:%s \n ", strerror(errno));
     			listen_fd = -1;
     			continue;
    		
		}

   		 // Configure the socket; we're setting a socket "option."  In
   		 // particular, we set "SO_REUSEADDR", which tells the TCP stack
   		 // so make the port we bind to available again as soon as we
    		// exit, rather than waiting for a few tens of seconds to recycle it.
    	
		int optval = 1;
    		setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    		// Try binding the socket to the address and port number returned
    		// by getaddrinfo().
	
		if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) 
		{
      
     			 // Return to the caller the address family.
      
			*sock_family = rp->ai_family;
      			break;
    
		}

    		
		// The bind failed.  Close the socket, then loop back around and
   		// try the next address/port returned by getaddrinfo().
    		
		close(listen_fd);
	    	listen_fd = -1;
	
  	}

	// Free the structure returned by getaddrinfo().
	freeaddrinfo(result);

	// If we failed to bind, return failure.
  	
  	if (listen_fd == -1)
    		return listen_fd;

	 // Success. Tell the OS that we want this to be a listening socket.
 	
	if (listen(listen_fd, SOMAXCONN) != 0) 
 	{
 
		printf("Failed to mark socket as listening:%s \n ", strerror(errno));
    		close(listen_fd);
   		 return -1;
  
	}

 	 // Return to the client the listening file descriptor.
  
	return listen_fd;

}

//usage

void usage(char *progname) 
{

      	printf("usage: %s port \n", progname);
  	exit(EXIT_FAILURE);

}

