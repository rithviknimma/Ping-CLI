#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h> 
#include <stdbool.h>
#include <netinet/in.h> 
#include <arpa/inet.h> 

#include <netinet/ip_icmp.h> 
#include <fcntl.h>
#include <time.h> 

#define PORT_NO 0 

bool PINGING;

// signal handler
void sig_handler(int sig_no){
	switch(sig_no){
		case SIGINT:
			// wait to get the last packet
			// print statistics like packet loss and round-trip
			PINGING = false;
			printf("\n--- statistics ---\n");
			exit(0);
			break;
	}
}

// char* getDNS(struct sockaddr_in* addr, char* hostname){
// 	struct hostend *host_struct;
// 	char* ip = (char *) malloc (NI_MAXHOST * sizeof(char));

// 	// host = gethostbyname(hostname);

// 	// if(host == NULL) // IP not found
// 	// 	return NULL;

// 	if ((host_struct = gethostbyname(hostname)) == NULL) 
//     { 
//         // No ip found for hostname 
//         return NULL; 
//     } 

//     return NULL;

// }
// Performs a DNS lookup  

char *getDNS(char *hostname, struct sockaddr_in *addr) { 
    struct hostent* host; 
    char *ip = (char *) malloc (NI_MAXHOST * sizeof(char)); 
  
  	host = gethostbyname(hostname);

    if (host == NULL) // IP not found
        return NULL; 

    strcpy(ip, inet_ntoa(*(struct in_addr *) host->h_addr)); 
  
  	(*addr).sin_port = htons (PORT_NO);
  	(*addr).sin_addr.s_addr  = *(long*)host->h_addr; 
    (*addr).sin_family = host->h_addrtype; 
  
    return ip; 
} 

int main(int argc, char** argv){
	char* ip;
	int socketNum;
	struct sockaddr_in* addr;

	PINGING = true;
	signal(SIGINT, &sig_handler); 

 // if(argc!=2) 
 //    { 
 //        printf("\nFormat %s <address>\n", argv[0]); 
 //        return 0; 
 //    }

    ip = getDNS(argv[argc-1], addr);
    
	while(1){

	}
}


