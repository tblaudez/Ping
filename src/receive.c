/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   receive.c                                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/03/19 13:02:20 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/19 14:24:22 by tblaudez      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

#include <stdbool.h> // bool
#include <errno.h> // errno
#include <stdio.h> // fprintf, printf
#include <sys/time.h> // gettimeofday
#include <unistd.h> // getpid, getuid, getopt


/* Wait for ICMP ECHO_REPLY from host */
bool receive_echo_reply()
{
	char recvbuf[128];
	struct iovec iov = {
		.iov_base = recvbuf,
		.iov_len = sizeof(recvbuf)
	};
	struct sockaddr target;
	struct msghdr msg = {
		.msg_name = (void*)&target,
		.msg_namelen = sizeof(target),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};
	const struct iphdr *ip = (struct iphdr *) recvbuf;
	const struct icmphdr *icmp = (struct icmphdr *) (recvbuf + IPHDR);
	int msg_flags = 0;
	ssize_t size;
	
	for (;;) {
		// Wait for message in socket
		size = recvmsg(g_ping.sockfd, &msg, msg_flags);
		msg_flags = MSG_DONTWAIT;
		
		if (size < 0) {
			if (errno == EAGAIN && g_ping.flags & VERBOSE)
				fprintf(stderr, "Timeout for icmp_seq=%ld\n", g_ping.npacket);
			return false; // Packet has timeout-ed, send new one immediatly
		}
		
		// If packet is not ours
		if (icmp->un.echo.id != getpid()) {
			msg_flags = 0;
			continue;
		}
		// If packet is not a ping reply
		else if (icmp->type != ICMP_ECHOREPLY) {
			if (g_ping.flags & VERBOSE) {
				printf("ICMP: ");
				pr_icmph(icmp->type, icmp->code, icmp->un.gateway, icmp);
			}
			continue;
		} 
		else break;
	}

	getnameinfo(&target, sizeof(target), g_ping.host_name, sizeof(g_ping.host_name), NULL, 0, 0);
	getnameinfo(&target, sizeof(target), g_ping.host_ip, sizeof(g_ping.host_ip), NULL, 0, NI_NUMERICHOST);
	
	printf("%lu bytes from %s (%s): icmp_seq=%u ttl=%d", size - IPHDR, g_ping.host_name, g_ping.host_ip, ntohs(icmp->un.echo.sequence), ip->ttl);
	
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
