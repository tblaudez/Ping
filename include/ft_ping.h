/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_ping.h                                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/02/09 14:35:43 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/15 15:24:14 by tblaudez      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PING_H
# define FT_PING_H

# include <netinet/in.h>
# include <netdb.h>
# include <linux/ip.h>
# include <linux/icmp.h>


# define DEFAULT_DATALEN 56
# define IPHDR sizeof(struct iphdr)
# define ICMPHDR sizeof(struct icmphdr)

# define RTT 0x01


extern struct s_ping
{
	struct addrinfo		hostinfo;
	char				host_ip[INET_ADDRSTRLEN];
	int					datalen;
	unsigned char		flags;
	int					sockfd, ttl, interval;
	unsigned int		npackets, ntransmitted, nreceived, nlimit, nerror;
} 						g_ping;


int	ft_atoi(const char *str);

# endif