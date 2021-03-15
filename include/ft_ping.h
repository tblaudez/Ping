/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_ping.h                                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/02/09 14:35:43 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/12 15:53:45 by anonymous     ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PING_H
# define FT_PING_H

# include <netinet/in.h>
# include <netdb.h>

# define M 1000000
# define DEFAULT_DATALEN 56


extern struct s_ping
{
	struct addrinfo		hostinfo;
	char				host_ip[INET_ADDRSTRLEN];
	int					datalen;
	unsigned char		flags;
	int					sockfd, ttl;
	unsigned int		npackets, ntransmitted, nreceived, nlimit, nerror;
} 						g_ping;

# endif