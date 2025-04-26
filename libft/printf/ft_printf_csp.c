/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf_csp.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/27 14:26:00 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:29:30 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

int	ft_printf_char(char c)
{
	ft_putchar_fd(c, 1);
	return (1);
}

int	ft_printf_str(char *str)
{
	if (!str)
	{
		ft_putstr_fd("(null)", 1);
		return (6);
	}
	else
	{
		ft_putstr_fd(str, 1);
		return (ft_strlen(str));
	}
}

int	ft_putptr(unsigned long long ptr)
{
	int	ptrlen;

	ptrlen = 0;
	if (ptr >= 16)
	{
		ptrlen += ft_putptr(ptr / 16);
		ft_putptr(ptr % 16);
	}
	else
	{
		if (ptr <= 9)
			ft_putchar_fd((ptr + '0'), 1);
		else
			ft_putchar_fd((ptr - 10 + 'a'), 1);
	}
	ptrlen++;
	return (ptrlen);
}

int	ft_printf_ptr(unsigned long long ptr)
{
	int	ptrlen;

	ptrlen = 0;
	ptrlen += write(1, "0x", 2);
	if (ptr == 0)
		ptrlen += write(1, "0", 1);
	else
		ptrlen += ft_putptr(ptr);
	return (ptrlen);
}
