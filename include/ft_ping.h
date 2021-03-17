/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_ping.h                                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/02/09 14:35:43 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/17 18:24:17 by tblaudez      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PING_H
# define FT_PING_H

# include <linux/ip.h> // iphdr
# include <linux/icmp.h> // icmphdr
# include <netinet/in.h> // sockaddr, socklen_t
# include <sys/types.h> // uint16_t
# include <netdb.h> // NI_MAXHOST


# define DEFAULT_DATALEN 56
# define IPHDR sizeof(struct iphdr)
# define ICMPHDR sizeof(struct icmphdr)

# define RTT 0x01
# define TIMEOUT 0x02
# define DEADLINE 0x04

struct s_host {
	struct sockaddr		addr;
	socklen_t			addrlen;
	char				name[NI_MAXHOST];
	char				ip[INET_ADDRSTRLEN];
};

extern struct s_ping
{
	struct s_host		host;
	char				host_arg[NI_MAXHOST];
	int					datalen;
	unsigned char		flags;
	int					sockfd, ttl, interval, count;
	long				deadline, timeout;
	long				npacket, ntransmitted, nreceived;
	long				tmin, tmax, tsum;
	long long			tsum2;
} 						g_ping;

// utils.c
int			ft_atoi(const char *str);
int			ft_strlen(const char *s);
void		*ft_memcpy(void *dst, const void *src, size_t n);
void		update_round_trip_time(long triptime);
void		tvsub(struct timeval *out, struct timeval *in);
uint16_t	in_cksum(void *addr, int size);
void		close_socket(void);
void		usage(void);
void		signal_handler(int signo);

// ping.c
void		finish();


# endif