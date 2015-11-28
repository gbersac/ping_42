#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#define PACKETSIZE  64

typedef struct sockaddr_in s_sockaddr_in;
typedef struct sockaddr s_sockaddr;
typedef struct icmphdr s_icmphdr;

// add timeval ?
typedef struct	packet
{
	s_icmphdr	hdr;
	char 		msg[PACKETSIZE - sizeof(s_icmphdr)];
}				s_packet;

int pid = -1;
struct protoent *proto = NULL;

/*--------------------------------------------------------------------*/
/*--- checksum - standard 1s complement checksum                   ---*/
/*--------------------------------------------------------------------*/
unsigned short checksum(void *b, int len)
{
	unsigned short *buf = b;
	unsigned int sum=0;
	unsigned short result;

	for ( sum = 0; len > 1; len -= 2 )
		sum += *buf++;
	if ( len == 1 )
		sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

/*--------------------------------------------------------------------*/
/*--- ping - Create message and send it.                           ---*/
/*--------------------------------------------------------------------*/
void ping(s_sockaddr_in *addr)
{
	const int val=255;
	int sd, cnt=1;
	s_packet pckt;
	s_sockaddr_in r_addr;

	sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
	if ( sd < 0 ) {
		perror("socket");
		return;
	}
	if ( setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
		perror("Set TTL option");
	if ( fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
		perror("Request nonblocking I/O");
	while (42) {
		int len = sizeof(r_addr);

		// send message
		printf("Msg #%d\n", cnt);
		bzero(&pckt, sizeof(pckt));
		pckt.hdr.type = ICMP_ECHO;
		pckt.hdr.un.echo.id = pid;
		pckt.hdr.un.echo.sequence = cnt++;
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
		if ( sendto(sd, &pckt, sizeof(pckt), 0, (s_sockaddr*)addr, sizeof(*addr)) <= 0 )
			perror("sendto");

		// receive message
		if ( recvfrom(sd, &pckt, sizeof(pckt), 0, (s_sockaddr*)&r_addr, (socklen_t *)&len) > 0 )
			printf("***Got message!***\n");
		sleep(1);
	}
}

/*--------------------------------------------------------------------*/
/*--- main - look up host and start ping processes.                ---*/
/*--------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	struct hostent *hname;
	s_sockaddr_in addr;

	if ( argc != 2 )
	{
		printf("usage: %s <addr>\n", argv[0]);
		exit(0);
	}

	pid = getpid();
	proto = getprotobyname("ICMP");
	hname = gethostbyname(argv[1]);
	bzero(&addr, sizeof(addr));
	addr.sin_family = hname->h_addrtype;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = *(long*)hname->h_addr;
	ping(&addr);
	return 0;
}
