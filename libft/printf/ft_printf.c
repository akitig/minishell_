/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/27 05:46:09 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:29:43 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

int	ft_printf_format(va_list args, const char format)
{
	int	printf_len;

	printf_len = 0;
	if (format == 'c')
		printf_len += ft_printf_char(va_arg(args, int));
	else if (format == 's')
		printf_len += ft_printf_str(va_arg(args, char *));
	else if (format == 'p')
		printf_len += ft_printf_ptr(va_arg(args, unsigned long long));
	else if (format == 'd' || format == 'i')
		printf_len += ft_printf_int(va_arg(args, int));
	else if (format == 'u')
		printf_len += ft_printf_uint(va_arg(args, unsigned int));
	else if (format == 'x' || format == 'X')
		printf_len += ft_printf_x(va_arg(args, int), format);
	else if (format == '%')
		printf_len += ft_printf_char('%');
	return (printf_len);
}

int	ft_printf(const char *str, ...)
{
	int		i;
	int		ret;
	va_list	args;

	i = 0;
	ret = 0;
	va_start(args, str);
	while (str[i] != 0)
	{
		if (str[i] == '%')
		{
			ret += ft_printf_format(args, str[i + 1]);
			i++;
		}
		else
			ret += ft_printf_char(str[i]);
		i++;
	}
	va_end(args);
	return (ret);
}
