/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_ping.c                                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/02/09 14:35:29 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/15 13:11:44 by anonymous     ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h" // g_ping
#include <unistd.h> // getpid, close
#include <stdio.h> // printf
#include <stdlib.h> // exit
#include <errno.h> // errno
#include <linux/icmp.h> //icmphdr
#include <sys/time.h> // gettimeofday
#include <string.h> // memset
#include <arpa/inet.h> // inet_ntop

/* Global ping variable */
struct s_ping g_ping = {
	.ttl = 113,
	.datalen = sizeof(struct icmphdr) + DEFAULT_DATALEN,
	.sockfd = -1
};

/* Simple checksum function */
unsigned short in_cksum(void *addr, int size)
{
	register uint32_t	sum;
	uint16_t			*buff;

	buff = (uint16_t *)addr;
	for (sum = 0; size > 1; size -= 2)
		sum += *buff++;
	if (size == 1)
		sum += *(uint8_t*)buff;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	return (~sum);
}

/* Close socket file descriptor and exit program */
void exit_clean(int const status)
{
	if (g_ping.sockfd != -1)
		close(g_ping.sockfd);
	exit(status);
}

/* Print program usage and exit */
void usage(void)
{
	fprintf(stderr, "usage: ft_ping [-vh] destination\n");
	exit_clean(EXIT_FAILURE);
}

/* Return the difference (in milliseconds) between two struct timeval */
static inline double get_time_difference(struct timeval *start, struct timeval *end)
{
	return ((double)(end->tv_sec - start->tv_sec) * 1000) + ((double)(end->tv_usec - start->tv_usec) / 1000);
}

/* Create socket and set socket options */
void setup_socket(void)
{
	// Create RAW socket
	if ((g_ping.sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		perror("socket");
		exit_clean(EXIT_FAILURE);
	}

	// Set socket time-to-live (default 113)
	if (setsockopt(g_ping.sockfd, IPPROTO_IP, IP_TTL, &g_ping.ttl, sizeof(g_ping.ttl)) < 0) {
		perror("ft_ping: can't set time-to-live socket option");
		exit_clean(EXIT_FAILURE);
	}

	// Set socket receive timeout (default 5 seconds)
	struct timeval timeout = { .tv_sec = 5 };
	if (setsockopt(g_ping.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		perror("ft_ping: can't set timeout socket option");
		exit_clean(EXIT_FAILURE);
	}
}

/* Find host and IP address */
void setup_host(const char *hostname)
{
	struct addrinfo	*res, hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_RAW,
		.ai_protocol = IPPROTO_ICMP,
		.ai_flags = AI_CANONNAME
	};
	
	// Get host by name
	if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
		fprintf(stderr, "ft_ping: getaddrinfo: could not resolve \"%s\"\n", hostname);
		exit_clean(EXIT_FAILURE);
	}

	// Copy result into global structure
	memcpy(&g_ping.hostinfo, res, sizeof(struct addrinfo));
	
	// Get IP address and copy it into global structure
	inet_ntop(res->ai_family, &((struct sockaddr_in*)res->ai_addr)->sin_addr, g_ping.host_ip, INET_ADDRSTRLEN);
	
	// Free result
	freeaddrinfo(res);
}

/* Send ICMP ECHO_REQUEST to host */
void send_echo_request()
{
	ssize_t			size;
	char			buf[g_ping.datalen];
	char			*ptr = buf + sizeof(struct icmphdr);
	struct icmphdr	*icmp = (struct icmphdr*) buf;


	// IP header is generated by kernel
	// First 8 bytes are ICMP header
	icmp->type = ICMP_ECHO;
	icmp->un.echo.id = getpid();
	icmp->un.echo.sequence = htons(g_ping.ntransmitted + 1);
	icmp->checksum = in_cksum(&icmp, sizeof(struct icmphdr));
	
	// If data is large enough, write timestamp
	// TODO: Find a prettier way to handle this
	if (g_ping.datalen - sizeof(struct icmphdr) >= sizeof(struct timeval)) {
		gettimeofday((struct timeval*) ptr, NULL);
		ptr += sizeof(struct timeval);
	}

	// Rest of data is set to arbitrary values
	for (char j = 0x10; ptr != (buf + g_ping.datalen); ptr++, j++) {
		*ptr = j;
	}
	
	// Send ECHO_REQUEST to host
	size = sendto(g_ping.sockfd, buf, g_ping.datalen, 0, g_ping.hostinfo.ai_addr, g_ping.hostinfo.ai_addrlen);
	
	if (size == -1) {
		fprintf(stderr, "ft_ping: error sending icmp_seq=%hu\n", g_ping.npackets);
		g_ping.nerror++;
	} else
		g_ping.ntransmitted++;
}

/* Wait for ICMP ECHO_REPLY from host */
void receive_echo_reply()
{
	ssize_t			size;
	char			recvbuf[1500] = {0};
	char			controlbuf[1500] = {0};
	struct msghdr	msg;
	struct iovec	iov[1];
	struct timeval	tv_start, tv_end;

	iov[0].iov_base = recvbuf;
	iov[0].iov_len = sizeof(recvbuf);
	msg.msg_name = g_ping.hostinfo.ai_addr;
	msg.msg_namelen = g_ping.hostinfo.ai_addrlen;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = controlbuf;
	msg.msg_controllen = sizeof(recvbuf);
	msg.msg_flags = 0;
	
	// Wait for message in socket
	size = recvmsg(g_ping.sockfd, &msg, 0);
	
	// TODO: Do this only if timeval was copied packet before sending
	memcpy(&tv_start, recvbuf + 28, sizeof(struct timeval));
	gettimeofday(&tv_end, NULL);
	
	// If timeout
	if (size == -1 && errno == EAGAIN) {
		fprintf(stderr, "ft_ping: request timed out for icmp_seq=%hu\n", g_ping.npackets);
		g_ping.nerror++;
	}
	else {
		// TODO: print round-trip time only of timeval was copied before sending
		printf("%lu bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n", size, g_ping.host_ip, g_ping.npackets, g_ping.ttl, get_time_difference(&tv_start, &tv_end));
		g_ping.nreceived++;
	}
}

/* Main loop of the program */
void ping_loop(char *const hostname)
{
	printf("PING %s (%s) %d(%d) data bytes\n", hostname, g_ping.host_ip, g_ping.datalen - 8, g_ping.datalen);
	// While limit is not reached (0 means no limit)
	while (g_ping.nlimit == 0 || (g_ping.npackets < g_ping.nlimit))
	{
		g_ping.npackets++;
		send_echo_request();
		receive_echo_reply();
		// TODO: replace sleep()
		sleep(1);
	}
	// TODO: signal handling / print stats when done
}

int main(int argc, char *argv[])
{
	// TODO: do real parsing
	char			*hostname = argv[1];

	if (argc < 2) {
		usage();
	}

	// If user is not root, exit
	if (getuid() != 0) {
		fprintf(stderr, "ft_ping: Operation Not Permitted\n");
		return 1;
	}
	
	// Create socket and set socket options
	setup_socket();
	
	// Find host and IP
	setup_host(hostname);

	// Main loop
	ping_loop(hostname);

	return 0;
}

