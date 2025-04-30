/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf_x.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/27 15:31:10 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:29:37 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

int	ft_put_x(unsigned int uinbr, const char format)
{
	int	x_len;

	x_len = 0;
	if (uinbr >= 16)
	{
		x_len += ft_put_x(uinbr / 16, format);
		ft_put_x(uinbr % 16, format);
	}
	else if (uinbr <= 9)
		ft_printf_char(uinbr + '0');
	else if (format == 'x')
		ft_printf_char(uinbr - 10 + 'a');
	else if (format == 'X')
		ft_printf_char(uinbr - 10 + 'A');
	x_len++;
	return (x_len);
}

int	ft_printf_x(unsigned int uinbr, const char format)
{
	if (uinbr)
		return (ft_put_x(uinbr, format));
	else
		return (write(1, "0", 1));
}
