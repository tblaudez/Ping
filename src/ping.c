/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ping.c                                             :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/02/09 14:35:29 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/22 10:38:46 by tblaudez      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h" // g_ping

#include <unistd.h> // getpid, getuid, getopt
#include <stdio.h> // printf
#include <stdlib.h> // exit, atexit
#include <sys/time.h> // gettimeofday
#include <limits.h> // LONG_MAX
#include <signal.h> // signal, SIGINT
#include <math.h> // sqrtl

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

/* Print program usage and exit */
static void usage(void)
{
	fprintf(stderr, "usage: ft_ping\t[-vh] [-c count] [-i interval]\n\t\t[-t ttl] [-w deadline] [-W timeout]\n\t\t[-s packetsize] destination\n");
	exit(EXIT_FAILURE);
}

/* Print ping statistics and exit */
void finish()
{
	putchar('\n');
	printf("--- %s ping statistics ---\n", g_ping.host_name);
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

/* Handle time to wait between two pings and ping deadline */
static void wait_for_next()
{
	struct timeval wait_start, now;
	
	gettimeofday(&wait_start, NULL);
	while (true) {
		gettimeofday(&now, NULL);
		tvsub(&now, &wait_start);
		if (now.tv_sec + now.tv_usec / 1000000 >= g_ping.interval)
			return;
	}
}

/* Main loop of the program */
static void ping_loop()
{
	printf("PING %s (%s) %ld(%ld) data bytes\n", g_ping.host_name, g_ping.host_ip, g_ping.datalen - ICMPHDR, g_ping.datalen + IPHDR);
	for (g_ping.npacket = 1; g_ping.count == 0 || g_ping.npacket <= g_ping.count; g_ping.npacket++)
	{
		send_echo_request();
		if (receive_echo_reply())
			wait_for_next();
	}
	finish();
}

/* Parse program arguments and set correct values. Return hostname */
static void parse_arguments(int argc, char* argv[])
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
	ft_memcpy(g_ping.host_name, *argv, ft_strlen(*argv));
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
	signal(SIGALRM, &signal_handler);
	
	// Parse arguments and get hostname
	parse_arguments(argc, argv);
	
	// Create socket and set socket options
	setup_socket();
	
	// Find host and IP
	setup_host();

	// If deadline is set, send SIGALRM in `deadline` seconds
	if (g_ping.flags & DEADLINE)
		alarm(g_ping.deadline);
	
	// Main loop
	ping_loop();

	return 0;
}