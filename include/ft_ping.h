/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_ping.h                                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/02/09 14:35:43 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/19 14:08:56 by tblaudez      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PING_H
# define FT_PING_H

# include <linux/ip.h> // iphdr
# include <linux/icmp.h> // icmphdr
# include <netinet/in.h> // sockaddr, socklen_t
# include <netdb.h> // NI_MAXHOST
# include <stdbool.h> // bool

# define DEFAULT_DATALEN 56
# define IPHDR sizeof(struct iphdr)
# define ICMPHDR sizeof(struct icmphdr)

# define VERBOSE 0x01
# define RTT 0x02
# define TIMEOUT 0x04
# define DEADLINE 0x08

extern struct s_ping
{
	struct sockaddr		addr;
	socklen_t			addrlen;
	char				host_name[NI_MAXHOST];
	char				host_ip[INET_ADDRSTRLEN];
	int					datalen;
	unsigned char		flags;
	int					sockfd, ttl, interval, count;
	long				deadline, timeout;
	long				npacket, ntransmitted, nreceived;
	long				tmin, tmax, tsum;
	long long			tsum2;
} 						g_ping;

// libft_functions.c
int			ft_atoi(const char *str);
int			ft_strlen(const char *s);
void		*ft_memcpy(void *dst, const void *src, size_t n);

// ping.c
void		finish();

// receive.c
bool		receive_echo_reply();

// send.c
void		send_echo_request();

// setup.c
void		setup_socket(void);
void		setup_host();

// utils.c
void		update_round_trip_time(long triptime);
void		tvsub(struct timeval *out, struct timeval *in);
uint16_t	in_cksum(void *addr, int size);
void		close_socket(void);
void		signal_handler(int signo);
void		pr_icmph(uint8_t type, uint8_t code, uint32_t info, const struct icmphdr *icp);

# endif