/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_itoa.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/16 08:18:58 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:30:37 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

int	ft_longlen(long n)
{
	long	i;

	i = 0;
	if (n < 0)
	{
		n *= -1;
		i++;
	}
	while (n > 9)
	{
		n /= 10;
		i++;
	}
	i++;
	return (i);
}

void	ft_put(long n, char *str, int *i)
{
	if (n > 9)
	{
		ft_put(n / 10, str, i);
		ft_put(n % 10, str, i);
	}
	else
		str[(*i)++] = n + '0';
}

char	*ft_itoa(int n)
{
	long	num;
	char	*ans;
	int		i;

	num = n;
	ans = malloc(sizeof(char) * ft_longlen(num) + 1);
	if (!ans)
		return (NULL);
	i = 0;
	if (num < 0)
	{
		ans[i++] = '-';
		num *= -1;
	}
	ft_put(num, ans, &i);
	ans[i] = '\0';
	return (ans);
}
