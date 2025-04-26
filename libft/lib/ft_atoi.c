/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_atoi.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/26 22:22:25 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:30:09 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

int	ft_isspace(char c)
{
	return (c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r'
		|| c == ' ');
}

static bool	is_cut(unsigned long current, unsigned long next,
		unsigned long base)
{
	unsigned long	cutoff;
	unsigned long	cutlim;

	cutoff = LONG_MAX / base;
	cutlim = LONG_MAX % base;
	if (current > cutoff || (current == cutoff && next > cutlim))
		return (true);
	else
		return (false);
}

// void	convert_isneg(const char **str, bool *isneg)
// {
// 	*isneg = false;
// 	while (ft_isspace(**str))
// 		(*str)++;
// 	if (**str == '-')
// 		*isneg = true;
// 	if (**str == '-' || **str == '+')
// 		(*str)++;
// }

static bool	is_check_flow(const char **str, int base, unsigned long *ul_v,
		bool *isneg)
{
	*isneg = false;
	*ul_v = 0;
	while (ft_isspace(**str))
		(*str)++;
	if (**str == '-')
		*isneg = true;
	if (**str == '-' || **str == '+')
		(*str)++;
	while (ft_isdigit(**str))
	{
		if (is_cut(*ul_v, (unsigned long)**str - '0', base))
			return (true);
		*ul_v = (*ul_v) * (unsigned long)base + (unsigned long)(**str - '0');
		(*str)++;
	}
	return (false);
}

long long	ft_strtol(const char *nptr, char **endptr, int base)
{
	unsigned long	ul_v;
	bool			error;
	bool			isneg;

	error = is_check_flow(&nptr, base, &ul_v, &isneg);
	if (endptr)
		*endptr = (char *)nptr;
	if (error)
	{
		errno = ERANGE;
		if (isneg == true)
			ul_v = (unsigned long)LONG_MIN;
		else
			ul_v = (unsigned long)LONG_MAX;
	}
	else if (isneg)
		ul_v = -ul_v;
	return ((long)ul_v);
}

int	ft_atoi(const char *str)
{
	return ((int)ft_strtol(str, NULL, 10));
}

// #include <stdio.h>

// int	main(void)
// {
// 	/* 27. LONG_MAX + 1 */
// printf("%d %d\n", atoi("9223372036854775808"),
//		ft_atoi("9223372036854775808"));
// /* 28. LONG_MIN - 1 */ printf("%d %d\n", atoi("-9223372036854775809"),
// 		ft_atoi("-9223372036854775809"));
// /* 29. ULONG_MAX + 1 */ printf("%d %d\n", atoi("18446744073709551616"),
// 		ft_atoi("18446744073709551616"));
// /* 30. SIZE_MAX + 1 */ printf("%d %d\n", atoi("18446744073709551616"),
// 		ft_atoi("18446744073709551616"));
// /* 31. INT_MAX - 1 */ printf("%d %d\n", atoi("2147483646"),
// 		ft_atoi("2147483646"));
// /* 32. INT_MIN + 1 */ printf("%d %d\n", atoi("-2147483647"),
// 		ft_atoi("-2147483647"));
// /* 33. UINT_MAX - 1 */ printf("%d %d\n", atoi("4294967294"),
// 		ft_atoi("4294967294"));
// /* 34. LONG_MAX - 1 */ printf("%d %d\n", atoi("9223372036854775806"),
// 		ft_atoi("9223372036854775806"));
// /* 35. LONG_MIN + 1 */ printf("%d %d\n", atoi("-9223372036854775807"),
// 		ft_atoi("-9223372036854775807"));
// /* 36. ULONG_MAX - 1 */ printf("%d %d\n", atoi("18446744073709551614"),
// 		ft_atoi("18446744073709551614"));
// /* 37. SIZE_MAX - 1 */ printf("%d %d\n", atoi("18446744073709551614"),
// 		ft_atoi("18446744073709551614"));
// }
