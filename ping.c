#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <float.h>
#include <signal.h>

#include <sys/types.h>
#include <unistd.h> 
#include <sys/socket.h>

#include <netdb.h>  // hostent struct defined here
#include <stdbool.h>
#include <netinet/in.h> // sockaddr_in and in_addr structs defined here
#include <arpa/inet.h> 

#include <netinet/ip_icmp.h> 
#include <fcntl.h>
#include <time.h> 

#define PORT_NO 0 
#define PACKETSIZE 64 // 64 bytes default
#define SLEEPTIME 1 // in seconds

bool PINGING;

/****** For reference: ************/
// struct icmphdr
// {
//   u_int8_t type;		/* message type */
//   u_int8_t code;		/* type sub-code */
//   u_int16_t checksum;
//   union
//   {
//     struct
//     {
//       u_int16_t	id;
//       u_int16_t	sequence;
//     } echo;			/* echo datagram */
//     u_int32_t	gateway;	/* gateway address */
//     struct
//     {
//       u_int16_t	__unused;
//       u_int16_t	mtu;
//     } frag;			/* path mtu discovery */
//   } un;
// };

// packet info
struct pckt{
	struct icmphdr header;
	char message[PACKETSIZE - sizeof(struct icmphdr)];
};

// signal handler
void sig_handler(int sig_no){
	PINGING = false;
}

// Performs a DNS lookup  
char* get_DNS(char *hostname, struct sockaddr_in *addr) { 
    struct hostent* host; 
  
  	host = gethostbyname(hostname);

    if (host == NULL) // IP not found
        return NULL; 

    // get address structure
  	(*addr).sin_port = htons (PORT_NO);
  	(*addr).sin_addr.s_addr  = *(long*)host->h_addr; 
    (*addr).sin_family = host->h_addrtype; 
  
    return inet_ntoa(*(struct in_addr *) host->h_addr); // returns IPv4 dotted decimal
} 

// Performs a reverse DNS lookup
bool get_reverseDNS(char *ip, char* reverseDNS){
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; // IPv4
	addr.sin_addr.s_addr = inet_addr(ip); // convert to int value of internet address

	char buf[NI_MAXHOST];

	if(getnameinfo((struct sockaddr *)&addr, sizeof(addr), buf, sizeof(buf), NULL, 0, NI_NAMEREQD) < 0){
		return false;
	}
	else{
		strcpy(reverseDNS, buf);
		return true;
	}

}

// calculates checksum value for each packet for error detection
uint16_t get_checkSum(void *packet, int packetLength){
	uint16_t *buf = packet; 
	uint32_t checkSum = 0;
	uint16_t result;
	int i;
	for(i = 0; i < packetLength; i += 2){ 
		checkSum += *buf++;
	}
	if(packetLength - i == 1)
		checkSum += *(uint8_t *) buf;
	checkSum = (checkSum >> 16) + (checkSum & 0xFFFF);
	checkSum += (checkSum >> 16);
	result = ~checkSum;
	return result;
}

// prints stats after pinging ends
void print_statistics(char * hostname, int sPackets, int rPackets, double totaltime, double mintime, double avgtime, double maxtime){
	double packetLoss = 0;	

	if(rPackets < sPackets){
		packetLoss = ((sPackets - rPackets)/sPackets) * 100.0;
	}

	printf("\n--- %s ping statistics ---\n", hostname);
	printf("%d packets transmitted, %d received, %f percent packet loss, total time: %f ms\n", sPackets, rPackets, packetLoss, totaltime);
	printf("rtt min: %f ms, avg: %f ms, max: %f ms\n", mintime, avgtime, maxtime);
}

// prints each echo reply
void print_echo_reply(char *reverseDNS, char *ip, int seq, int ttl, double rtt){
	if(reverseDNS == NULL){
		printf("%d bytes from %s (%s): icmp_seq = %d ttl = %d rtt = %f ms\n", PACKETSIZE, ip, ip, seq, ttl, rtt);
	}
	else
		printf("%d bytes from %s (%s): icmp_seq = %d ttl = %d rtt = %f ms\n", PACKETSIZE, reverseDNS, ip, seq, ttl, rtt);
}

// handles sending and recieving ICMP messages
void echo_handler(char *ip, int socketNum, struct sockaddr_in* addr, char* hostname){
	struct pckt packet;
	bool recieve;
	struct sockaddr_in recieve_addr; 
	int recieveLength;

	int sentPackets = 0; 
	int recievedPackets = 0; 

	char reverseDNS[NI_MAXHOST];
	bool rDNSavailable = get_reverseDNS(ip, reverseDNS); // get reverse DNS

	// time
	double rtt_time;
	double rtt_min = DBL_MAX;
	double rtt_max = 0;
	double rtt_avg;
	struct timespec start, end, rtt_start, rtt_end;

	clock_gettime(CLOCK_MONOTONIC, &start);

	// set socket options
	int ttl = 64; // time to live
	setsockopt(socketNum, SOL_IP, IP_TTL, &ttl, sizeof(ttl));
	struct timeval timeout; 
    timeout.tv_sec = 1; 
    timeout.tv_usec = 0; 
	setsockopt(socketNum, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);


	// keep sending echo messages
	while(PINGING){
		/* update packet */
		memset(&packet, 0, sizeof(packet)); // reset packet
		packet.header.type = ICMP_ECHO;
		packet.header.un.echo.id = getpid();
		packet.header.un.echo.sequence = sentPackets++;
		for(int i = 0; i < PACKETSIZE - sizeof(struct icmphdr); i++){ // empty message
			packet.message[i] = '0';
		}
		packet.header.checksum = get_checkSum(&packet, sizeof(packet)); // update checksum

		
		clock_gettime(CLOCK_MONOTONIC, &rtt_start); // start rtt time
		recieve = true;
		if(sendto(socketNum, &packet, sizeof(packet), 0, (struct sockaddr *)addr, sizeof(*addr)) < 0){
			printf("ping: Error sending packet to %s\n", hostname);
			recieve = false;
		}
		if(recvfrom(socketNum, &packet, sizeof(packet), 0, (struct sockaddr *)&recieve_addr, &recieveLength) < 0){
			printf("ping: Error recieving packet from %s\n", hostname);
		}
		else{
			if(recieve && packet.header.type == 69 && packet.header.code == ICMP_ECHOREPLY){
				clock_gettime(CLOCK_MONOTONIC, &rtt_end); // end rtt time
				rtt_time = ((rtt_end.tv_sec - rtt_start.tv_sec) * 1000.0) + (((double) (rtt_end.tv_nsec - rtt_start.tv_nsec))/1000000.0);

				if(rDNSavailable)
					print_echo_reply(reverseDNS, ip, sentPackets, ttl, rtt_time);
				else
					print_echo_reply(NULL, ip, sentPackets, ttl, rtt_time);

				/* calculate statistics*/
				if(rtt_time < rtt_min)
					rtt_min = rtt_time;
				if(rtt_max < rtt_time)
					rtt_max = rtt_time;

				rtt_avg += rtt_time;

				recievedPackets++;
			}
			else{
				printf("ping: Error, packet recieved with type %d, and code %d\n", packet.header.type, packet.header.code);
			}
		}
		sleep(SLEEPTIME); // pause 1 second
	}

	// acquire and print stats
	clock_gettime(CLOCK_MONOTONIC, &end);
	rtt_time = ((end.tv_sec - start.tv_sec) * 1000.0) + (((double) (end.tv_nsec - start.tv_nsec))/1000000.0);
	rtt_avg = (double) rtt_avg/recievedPackets;
	print_statistics(hostname, sentPackets, recievedPackets, rtt_time, rtt_min, rtt_avg, rtt_max);
}

// driver
int main(int argc, char** argv){
	char* ip;
	int socketNum;
	struct sockaddr_in addr;

	PINGING = true;
	signal(SIGINT, &sig_handler); 

 	if(argc!=2){ 
        printf("ping: incorrect formatting\n"); 
        exit(0);
    }

	char* hostname = argv[argc-1];

    ip = get_DNS(hostname, &addr); // get DNS
    if(ip == NULL){
    	printf("ping: cannot resolve \"%s\": Unknown host\n", hostname);
    	exit(0);
    }

    socketNum = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(socketNum < 0)
    	printf("\nping: Socket file descriptor error \n");

    echo_handler(ip, socketNum, &addr, hostname);

    return 0;
}


