/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   setup.c                                            :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/03/19 13:01:22 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/19 14:14:06 by tblaudez      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

#include <stdlib.h> // exit
#include <stdio.h> // printf
#include <arpa/inet.h> // inet_ntop

/* Create socket and set socket options */
void setup_socket(void)
{
	// Create RAW socket
	if ((g_ping.sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		printf("ft_ping: can't create raw socket");
		exit(EXIT_FAILURE);
	}

	// Set socket time-to-live (default 64)
	if (setsockopt(g_ping.sockfd, IPPROTO_IP, IP_TTL, &g_ping.ttl, sizeof(g_ping.ttl)) < 0) {
		printf("ft_ping: can't set time-to-live socket option");
		exit(EXIT_FAILURE);
	}

	// Set socket receive timeout
	struct timeval timeout = { .tv_sec = g_ping.timeout };
	if (setsockopt(g_ping.sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		printf("ft_ping: can't set timeout socket option");
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
	if (getaddrinfo(g_ping.host_name, NULL, &hints, &res)) {
		fprintf(stderr, "ft_ping: %s: Name or service not known\n", g_ping.host_name);
		exit(EXIT_FAILURE);
	}

	// Copy result into global structure
	g_ping.addrlen = res->ai_addrlen;
	ft_memcpy(&g_ping.addr, res->ai_addr, res->ai_addrlen);

	inet_ntop(res->ai_family, &((struct sockaddr_in*)res->ai_addr)->sin_addr, g_ping.host_ip, sizeof(g_ping.host_ip));

	// Free result
	freeaddrinfo(res);
}
