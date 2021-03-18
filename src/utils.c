/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   utils.c                                            :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/03/15 14:03:05 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/18 10:40:15 by tblaudez      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h" // g_ping
#include <sys/types.h> // u_int16_t
#include <stdbool.h> // bool
#include <stdio.h> // fprintf
#include <stdlib.h> // exit
#include <unistd.h> // close
#include <signal.h> // SIGINT

static inline bool ft_isspace(const char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r');
}

static inline bool ft_isdigit(const char c)
{
	return (c >= '0' && c <= '9');
}

int ft_atoi(const char *str)
{
	int sign = 1, total = 0;

	while(ft_isspace(*str)) {
		str++;
	}
	
	if (*str == '-') {
		sign = -1;
		str++;
	}
	else if (*str == '+')
		str++;
		
	while (ft_isdigit(*str)) {
		total = (total * 10) + (*str - '0');
		str++;
	}
	
	return (total * sign);
}

int	ft_strlen(const char *s)
{
	if (!s || !*s)
		return (0);
	return (1 + ft_strlen(s + 1));
}

void *ft_memcpy(void *dst, const void *src, size_t n)
{
	void		*tmp;
	char		*dst_dup;
	const char	*src_dup;
	size_t		i;

	i = 0;
	dst_dup = dst;
	src_dup = src;
	tmp = dst;
	while (i < n)
	{
		dst_dup[i] = src_dup[i];
		i++;
	}
	return (tmp);
}

/* Simple checksum function */
u_int16_t in_cksum(void *addr, int size)
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

/* Close socket file descriptor */
void close_socket(void)
{
	if (g_ping.sockfd != -1)
		close(g_ping.sockfd);
}

/* Print program usage and exit */
void usage(void)
{
	fprintf(stderr, "usage: ft_ping\t[-vh] [-c count] [-i interval]\n\t\t[-t ttl] [-w deadline] [-W timeout]\n\t\t[-s packetsize] destination\n");
	exit(EXIT_FAILURE);
}

/* Handles program's signals */
void signal_handler(int signo)
{
	switch (signo)
	{
		case SIGINT:
			finish();
			break;
		
		default:
			break;
	}
}

/* Subtract two struct timeval and copy result in the first argument */
void tvsub(struct timeval *out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) < 0)
	{
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

/* Update ping's RTT stats */
void update_round_trip_time(long triptime)
{
	g_ping.tsum += triptime;
	g_ping.tsum2 += (long long)triptime * triptime;

	if (triptime < g_ping.tmin)
		g_ping.tmin = triptime;
	if (triptime > g_ping.tmax)
		g_ping.tmax = triptime;
}

void pr_icmph(uint8_t type, uint8_t code, uint32_t info, struct icmphdr *icp)
{
	switch(type) {
	case ICMP_ECHOREPLY:
		printf("Echo Reply\n");
		break;
	case ICMP_DEST_UNREACH:
		switch(code) {
		case ICMP_NET_UNREACH:
			printf("Destination Net Unreachable\n");
			break;
		case ICMP_HOST_UNREACH:
			printf("Destination Host Unreachable\n");
			break;
		case ICMP_PROT_UNREACH:
			printf("Destination Protocol Unreachable\n");
			break;
		case ICMP_PORT_UNREACH:
			printf("Destination Port Unreachable\n");
			break;
		case ICMP_FRAG_NEEDED:
			printf("Frag needed and DF set (mtu = %u)\n", info);
			break;
		case ICMP_SR_FAILED:
			printf("Source Route Failed\n");
			break;
		case ICMP_PKT_FILTERED:
			printf("Packet filtered\n");
			break;
		default:
			printf("Dest Unreachable, Bad Code: %d\n", code);
			break;
		}
		break;
	case ICMP_SOURCE_QUENCH:
		printf("Source Quench\n");
		break;
	case ICMP_REDIRECT:
		switch(code) {
		case ICMP_REDIR_NET:
			printf("Redirect Network");
			break;
		case ICMP_REDIR_HOST:
			printf("Redirect Host");
			break;
		case ICMP_REDIR_NETTOS:
			printf("Redirect Type of Service and Network");
			break;
		case ICMP_REDIR_HOSTTOS:
			printf("Redirect Type of Service and Host");
			break;
		default:
			printf("Redirect, Bad Code: %d", code);
			break;
		}
		break;
	case ICMP_ECHO:
		printf("Echo Request\n");
		break;
	case ICMP_TIME_EXCEEDED:
		switch(code) {
		case ICMP_EXC_TTL:
			printf("Time to live exceeded\n");
			break;
		case ICMP_EXC_FRAGTIME:
			printf("Frag reassembly time exceeded\n");
			break;
		default:
			printf("Time exceeded, Bad Code: %d\n", code);
			break;
		}
		break;
	case ICMP_PARAMETERPROB:
		printf("Parameter problem: pointer = %u\n", icp ? (ntohl(icp->un.gateway)>>24) : info);
		break;
	case ICMP_TIMESTAMP:
		printf("Timestamp\n");
		break;
	case ICMP_TIMESTAMPREPLY:
		printf("Timestamp Reply\n");
		break;
	case ICMP_INFO_REQUEST:
		printf("Information Request\n");
		break;
	case ICMP_INFO_REPLY:
		printf("Information Reply\n");
		break;
	default:
		printf("Bad ICMP type: %d\n", type);
	}
}