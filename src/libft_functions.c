/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   libft_functions.c                                  :+:    :+:            */
/*                                                     +:+                    */
/*   By: tblaudez <tblaudez@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2021/03/19 13:10:59 by tblaudez      #+#    #+#                 */
/*   Updated: 2021/03/19 13:11:34 by tblaudez      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include <stdbool.h>
#include <sys/types.h>

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