/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ping.c                                             :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/02/09 14:35:29 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/18 11:50:52 by tblaudez      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h" // g_ping

#include <netdb.h> // addrinfo, getnameinfo
#include <unistd.h> // getpid, getuid, getopt
#include <stdio.h> // printf
#include <stdlib.h> // exit, atexit
#include <errno.h> // errno
#include <sys/time.h> // gettimeofday
#include <stdbool.h> // bool
#include <limits.h> // LONG_MAX
#include <signal.h> // signal, SIGINT
#include <math.h> // sqrtl
#include <arpa/inet.h> // inet_ntop

/* Global ping variable */
struct s_ping g_ping = {
	.ttl = 64,
	.datalen = ICMPHDR + DEFAULT_DATALEN,
	.sockfd = -1,
	.interval = 1,
	.timeout = 1,
	.count = 0,
	.flags = RTT,
	.tmin = LONG_MAX
};

/* Create socket and set socket options */
void setup_socket(void)
{
	// Create RAW socket
	if ((g_ping.sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Set socket time-to-live (default 64)
	if (setsockopt(g_ping.sockfd, IPPROTO_IP, IP_TTL, &g_ping.ttl, sizeof(g_ping.ttl)) < 0) {
		perror("ft_ping: can't set time-to-live socket option");
		exit(EXIT_FAILURE);
	}

	// Set socket receive timeout
	struct timeval timeout = { .tv_sec = g_ping.timeout };
	if (setsockopt(g_ping.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		perror("ft_ping: can't set timeout socket option");
		exit(EXIT_FAILURE);
	}
}

/* Find host and IP address */
void setup_host()
{
	struct addrinfo	*res = NULL, hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_RAW,
		.ai_protocol = IPPROTO_ICMP
	};
	
	// Get host by name
	if (getaddrinfo(g_ping.host_arg, NULL, &hints, &res)) {
		fprintf(stderr, "ft_ping: %s: Name or service not known\n", g_ping.host_arg);
		exit(EXIT_FAILURE);
	}

	// Copy result into global structure
	g_ping.host.addrlen = res->ai_addrlen;
	ft_memcpy(&g_ping.host.addr, res->ai_addr, res->ai_addrlen);

	getnameinfo(res->ai_addr, res->ai_addrlen, g_ping.host.name, sizeof(g_ping.host.name), NULL, 0, 0);
	getnameinfo(res->ai_addr, res->ai_addrlen, g_ping.host.ip, sizeof(g_ping.host.ip), NULL, 0, NI_NUMERICHOST);
	
	// Free result
	freeaddrinfo(res);
}

/* Print ping statistics and exit */
void finish()
{
	putchar('\n');
	printf("--- %s ping statistics ---\n", g_ping.host_arg);
	printf("%ld packet transmitted, ", g_ping.ntransmitted);
	printf("%ld received", g_ping.nreceived);
	if (g_ping.ntransmitted > 0) {
		int loss = g_ping.nreceived == 0 ? 100 : ((g_ping.ntransmitted - g_ping.nreceived) * 100) / g_ping.ntransmitted;
		printf(", %d%% packet loss", loss);
	}
	putchar('\n');

	if (g_ping.flags & RTT && g_ping.nreceived > 0) {
		g_ping.tsum /= g_ping.nreceived;
		g_ping.tsum2 /= g_ping.nreceived;
		long tmdev = sqrtl(g_ping.tsum2 - g_ping.tsum * g_ping.tsum);

		printf("rtt min/avg/max/mdev = %ld.%03ld/%lu.%03ld/%ld.%03ld/%ld.%03ld ms\n",
			g_ping.tmin / 1000, g_ping.tmin % 1000,
			(g_ping.tsum / 1000), (g_ping.tsum % 1000),
			g_ping.tmax / 1000, g_ping.tmax % 1000,
			tmdev / 1000, tmdev % 1000);
	}

	exit(EXIT_SUCCESS);
}

/* Send ICMP ECHO_REQUEST to host */
void send_echo_request()
{
	ssize_t			size;
	char			buf[g_ping.datalen];
	char			*ptr = buf + ICMPHDR;
	struct icmphdr	*icmp = (struct icmphdr*) buf;


	// IP header is generated by kernel
	// First 8 bytes are ICMP header
	icmp->type = ICMP_ECHO;
	icmp->code = 0;
	icmp->checksum = 0;
	icmp->un.echo.id = getpid();
	icmp->un.echo.sequence = htons(g_ping.ntransmitted + 1);
	
	// If data is large enough, write timestamp
	if (g_ping.flags & RTT) {
		gettimeofday((struct timeval*) ptr, NULL);
		ptr += sizeof(struct timeval);
	}

	// Rest of data is set to arbitrary values
	for (char j = 0x10; ptr != (buf + g_ping.datalen); ptr++, j++) {
		*ptr = j;
	}

	icmp->checksum = in_cksum(icmp, g_ping.datalen);
	
	// Send ECHO_REQUEST to host
	size = sendto(g_ping.sockfd, icmp, g_ping.datalen, 0, &g_ping.host.addr, g_ping.host.addrlen);
	
	if (size == -1) {
		fprintf(stderr, "ft_ping: error sending icmp_seq=%hu\n", ntohs(icmp->un.echo.sequence));
		return;
	}
	
	g_ping.ntransmitted++;
}

/* Wait for ICMP ECHO_REPLY from host */
bool receive_echo_reply()
{
	char recvbuf[128];
	struct iovec iov = {
		.iov_base = recvbuf,
		.iov_len = sizeof(recvbuf)
	};
	struct s_host target = {.addrlen = sizeof(target.addr)};
	struct msghdr msg = {
		.msg_name = (void*)&target.addr,
		.msg_namelen = target.addrlen,
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};
	struct iphdr *ip = (struct iphdr *) recvbuf;
	struct icmphdr *icmp = (struct icmphdr *) (recvbuf + IPHDR);
	int msg_flags = 0;
	ssize_t size;
	
	for (;;) {
		// Wait for message in socket
		size = recvmsg(g_ping.sockfd, &msg, msg_flags);
		msg_flags = MSG_DONTWAIT;
		
		if (size < 0 && (errno == EAGAIN || errno == EINTR)) {
			if (g_ping.flags & VERBOSE)
				fprintf(stderr, "Timeout for icmp_seq=%ld\n", g_ping.npacket);
			return false; // Packet has timeout-ed, send new one immediatly
		}
		
		getnameinfo(&target.addr, target.addrlen, target.name, sizeof(target.name), NULL, 0, 0);
		getnameinfo(&target.addr, target.addrlen, target.ip, sizeof(target.ip), NULL, 0, NI_NUMERICHOST);
		
		if (icmp->type == ICMP_ECHOREPLY)
			break;
		else if (g_ping.flags & VERBOSE) {
			printf("From %s (%s) ", target.name, target.ip);
			pr_icmph(icmp->type, icmp->code, icmp->un.gateway, icmp);
		}
	}
	
	printf("%lu bytes from %s (%s): icmp_seq=%u ttl=%d", size - IPHDR, target.name, target.ip, ntohs(icmp->un.echo.sequence), ip->ttl);
	// If RTT is set, get send and receive timestamps	
	if (g_ping.flags & RTT) {
		struct timeval tv_end;
		gettimeofday(&tv_end, NULL);
		tvsub(&tv_end, (struct timeval *) (recvbuf + IPHDR + ICMPHDR));
		long triptime = tv_end.tv_sec * 1000000 + tv_end.tv_usec;
		update_round_trip_time(triptime);
		printf(" time=%ld.%03ld ms", triptime / 1000, triptime % 1000);
	}
	
	putchar('\n');
	g_ping.nreceived++;
	return true;
}

/* Handle time to wait between two pings and ping deadline */
static void wait_for_next(struct timeval *program_start)
{
	struct timeval wait_start, now, now2;
	
	gettimeofday(&wait_start, NULL);
	while (true) {
		gettimeofday(&now, NULL);
		ft_memcpy(&now2, &now, sizeof(now));
		
		if (g_ping.flags & DEADLINE) {
			tvsub(&now, program_start);
			if ((now.tv_sec + now.tv_usec / 1000000) >= g_ping.deadline)
				finish();
		}

		tvsub(&now2, &wait_start);
		if (now2.tv_sec + now2.tv_usec / 1000000 >= g_ping.interval)
			return;
	}
}

/* Main loop of the program */
void ping_loop()
{
	struct timeval program_start;

	gettimeofday(&program_start, NULL);
	printf("PING %s (%s) %ld(%ld) data bytes\n", g_ping.host_arg, g_ping.host.ip, g_ping.datalen - ICMPHDR, g_ping.datalen + IPHDR);
	for (g_ping.npacket = 1; g_ping.count == 0 || g_ping.npacket <= g_ping.count; g_ping.npacket++)
	{
		send_echo_request();
		if (receive_echo_reply())
			wait_for_next(&program_start);
	}
	finish();
}

/* Parse program arguments and set correct values. Return hostname */
void parse_arguments(int argc, char* argv[])
{
	int	ch;

	while ((ch = getopt(argc, argv, "c:s:i:t:W:w:vh")) != -1) {
		switch (ch) {
			
			// Count limit
			case 'c':
				if ((g_ping.count = ft_atoi(optarg)) <= 0) {
					fprintf(stderr, "ft_ping: bad number of packets to transmit.\n");
					exit(EXIT_FAILURE);
				}
				break;
			
			// Packet size
			case 's':
			{
				int packet_size = ft_atoi(optarg);
				if (packet_size < 0) {
					fprintf(stderr, "ft_ping: illegal negative packet size -1.\n");
					exit(EXIT_FAILURE);
				}
				// If packet size can't hold a timeval, disable RTT
				if ((size_t)packet_size < sizeof(struct timeval))
					g_ping.flags &= ~RTT;
				g_ping.datalen = ICMPHDR + packet_size;
				break;
			}
			
			// Interval
			case 'i':
				if ((g_ping.interval = ft_atoi(optarg)) < 0) {
					fprintf(stderr, "ft_ping: bad timing interval.\n");
					exit(EXIT_FAILURE);
				}
				break;
			
			case 't':
			{
				int ttl = ft_atoi(optarg);
				if (ttl <= 0 || ttl > 255) {
					fprintf(stderr, "ft_ping: ttl %d out of range.\n", ttl);
					exit(EXIT_FAILURE);
				}
				g_ping.ttl = ttl;
				break;
			}

			case 'W':
				g_ping.flags |= TIMEOUT;
				if ((g_ping.timeout = ft_atoi(optarg)) < 0) {
					fprintf(stderr, "ft_ping: bad timeout value");
					exit(EXIT_FAILURE);
				}
				break;
			
			case 'w':
				g_ping.flags |= DEADLINE;
				if ((g_ping.deadline = ft_atoi(optarg)) < 0) {
					fprintf(stderr, "ft_ping: bad deadline value");
					exit(EXIT_FAILURE);
				}
				break;
			
			case 'v':
				g_ping.flags |= VERBOSE;
				break;

			case 'h':
			default:
				usage();
		}
	}
	

	if ((argc -= optind) == 0) {
		usage();
	}

	argv += optind;
	ft_memcpy(g_ping.host_arg, *argv, ft_strlen(*argv));
}

int main(int argc, char* argv[])
{
	// If user is not root, exit
	if (getuid() != 0) {
		fprintf(stderr, "ft_ping: Operation Not Permitted\n");
		return 1;
	}
	
	// Setup closing socket at exit
	atexit(&close_socket);
	// Setup signal handler
	signal(SIGINT, &signal_handler);
	// Parse arguments and get hostname
	parse_arguments(argc, argv);
	// Create socket and set socket options
	setup_socket();
	// Find host and IP
	setup_host();
	// Main loop
	ping_loop();

	return 0;
}

