/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   utils.c                                            :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/03/15 14:03:05 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/15 14:12:07 by tblaudez      ########   odam.nl         */
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
	fprintf(stderr, "usage: ft_ping [-vh] destination\n");
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