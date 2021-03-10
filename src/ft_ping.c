/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_ping.c                                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/02/09 14:35:29 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/10 18:08:37 by anonymous     ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h" // g_ping
#include <unistd.h> // getpid
#include <stdio.h> // printf
#include <stdlib.h> // exit
#include <errno.h> // errno
#include <math.h> // isfinite
#include <limits.h> // INT32MAX
#include <linux/icmp.h> //icmphdr
#include <sys/time.h> // gettimeofday
#include <string.h> // memset
#include <arpa/inet.h> // inet_ntop
#include <netdb.h> // addrinfo

struct s_ping g_ping = {0};

unsigned short in_cksum(void *addr, int size)
{
	register uint32_t sum;
	uint16_t *buff;

	buff = (uint16_t *)addr;
	for (sum = 0; size > 1; size -= 2)
		sum += *buff++;
	if (size == 1)
		sum += *(uint8_t*)buff;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	return (~sum);
}

void exit_clean(int status)
{
	if (g_ping.sockfd != -1)
		close(g_ping.sockfd);
	exit(status);
}

void usage(void)
{
	fprintf(stderr, "usage: ft_ping [-vh] destination\n");
	exit_clean(EXIT_FAILURE);
}

void setup_socket(void)
{
	g_ping.ttl = 113;

	if ((g_ping.sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		perror("socket");
		exit_clean(EXIT_FAILURE);
	}

	if (setsockopt(g_ping.sockfd, IPPROTO_IP, IP_TTL, &g_ping.ttl, sizeof(g_ping.ttl)) < 0) {
		perror("ft_ping: can't set time-to-live socket option");
		exit_clean(EXIT_FAILURE);
	}

	struct timeval timeout = { .tv_sec = 5 };
	if (setsockopt(g_ping.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		perror("ft_ping: can't set timeout socket option");
		exit_clean(EXIT_FAILURE);
	}
}

void setup_host(const char *hostname)
{
	struct addrinfo	*res, hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_RAW,
		.ai_protocol = IPPROTO_ICMP
	};
	
	if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
		fprintf(stderr, "ft_ping: getaddrinfo: could not resolve \"%s\"\n", hostname);
		exit_clean(EXIT_FAILURE);
	}

	memcpy(&g_ping.host, res->ai_addr, sizeof(struct sockaddr));
	inet_ntop(res->ai_family, &g_ping.host.sin_addr, g_ping.host_ip, INET_ADDRSTRLEN);
	freeaddrinfo(res);
}

void setup_send(struct timeval *tv_start)
{
	bzero(&g_ping.send, sizeof(struct s_send));
	
	g_ping.send.icmp.type = ICMP_ECHO;
	g_ping.send.icmp.un.echo.id = getpid();
	g_ping.send.icmp.un.echo.sequence = g_ping.ntransmitted + 1;
	g_ping.send.icmp.checksum = in_cksum(&g_ping.send.icmp, sizeof(struct icmphdr));

	// memcpy(&g_ping.send.data, tv_start, sizeof(struct timeval));
}

void setup_receive(void)
{
	bzero(&g_ping.receive, sizeof(struct s_receive));
	
	g_ping.receive.msg.msg_name = NULL;
	g_ping.receive.msg.msg_namelen = 0;
	g_ping.receive.msg.msg_iov = &g_ping.receive.iov;
	g_ping.receive.msg.msg_iov->iov_base = g_ping.receive.recv_buf;
	g_ping.receive.msg.msg_iov->iov_len = sizeof(g_ping.receive.recv_buf);
	g_ping.receive.msg.msg_iovlen = 1;
	g_ping.receive.msg.msg_control = &g_ping.receive.control;
	g_ping.receive.msg.msg_controllen = sizeof(g_ping.receive.control);
}

void send_echo_request(struct timeval *tv_start)
{
	ssize_t	size;
	
	setup_send(tv_start);
	size = sendto(g_ping.sockfd, &g_ping.send, sizeof(g_ping.send), 0, (struct sockaddr*)&g_ping.host, sizeof(g_ping.host));
	if (size == -1) {
		fprintf(stderr, "ft_ping: sendto: could not send packet\n");
		g_ping.nerror++;
	} else {
		g_ping.ntransmitted++;
	}
}

static inline double get_time_difference(struct timeval *start, struct timeval *end)
{
	return (double)((end->tv_sec * 1000 + end->tv_usec) - (start->tv_sec * 1000 + start->tv_usec)) / 1000;
}

void receive_echo_reply(struct timeval *tv_start)
{
	ssize_t size;
	struct timeval tv_end;

	setup_receive();
	size = recvmsg(g_ping.sockfd, &g_ping.receive.msg, 0);
	gettimeofday(&tv_end, NULL);
	if (size == -1 && errno == EAGAIN) {
		fprintf(stderr, "ft_ping: request timed out for icmp_seq=%hu\n", g_ping.npackets);
		g_ping.nerror++;
	} else {
		fprintf(stdout, "%lu bytes from %s: icmp_seq=%d ttl=%d time=%.2f ms\n", size, g_ping.host_ip, g_ping.npackets, g_ping.ttl, get_time_difference(tv_start, &tv_end));
		g_ping.nreceived++;
	}
}

void ping_loop(char *const hostname)
{
	struct timeval tv_start;

	printf("PING %s (%s) %lu(%lu) data bytes\n", hostname, g_ping.host_ip, sizeof(g_ping.send.data), sizeof(g_ping.send));
	while (g_ping.nlimit == 0 || (g_ping.npackets < g_ping.nlimit))
	{
		gettimeofday(&tv_start, NULL);
		g_ping.npackets++;
		send_echo_request(&tv_start);
		receive_echo_reply(&tv_start);
		sleep(1);
	}
}

int main(int argc, char *argv[])
{
	char			*hostname = argv[1]; // TODO: replace

	if (argc < 2) {
		usage();
	}
	if (getuid() != 0) {
		fprintf(stderr, "ft_ping: Operation Not Permitted\n");
		return 1;
	}

	setup_socket();
	setup_host(hostname);
	ping_loop(hostname);

	return 0;
}