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

#include <stdbool.h> // bool

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