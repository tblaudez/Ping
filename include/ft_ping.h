/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ft_ping.h                                          :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/02/09 14:35:43 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/10 14:37:46 by anonymous     ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PING_H
# define FT_PING_H

# include <netinet/in.h>
# include <linux/ip.h>
# include <linux/icmp.h>
# include <sys/time.h>
# include <sys/socket.h>

# define PACKET_SIZE 84


struct s_send
{
	struct icmphdr		icmp;
	char				data[PACKET_SIZE];
};

struct s_receive
{
	char				recv_buf[PACKET_SIZE];
	struct msghdr		msg;
	struct iovec		iov;
	struct icmphdr 		recv_icmp;
	union {
		char			buff[CMSG_SPACE(sizeof(int))];
		struct cmsghdr	align;
	}					control;
};

struct s_ping
{
	struct s_send		send;
	struct s_receive	receive;
	struct sockaddr_in	host;
	char				host_ip[INET_ADDRSTRLEN];
	unsigned char		flags;
	int					ttl, interval, deadline, timeout, sockfd;
	unsigned int		ntransmitted;
	unsigned int		nreceived;
};

extern struct s_ping	g_ping;

# endif