/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf_diu.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/27 14:45:53 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:29:33 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

int	ft_uilen(unsigned int uinbr)
{
	int	len;

	len = 0;
	while (uinbr)
	{
		len++;
		uinbr /= 10;
	}
	return (len);
}

char	*ft_uitoa(unsigned int uinbr)
{
	int		uinbr_len;
	char	*str_uinbr;

	uinbr_len = ft_uilen(uinbr);
	str_uinbr = (char *)malloc(sizeof(char) * (uinbr_len + 1));
	if (!str_uinbr)
		return (0);
	str_uinbr[uinbr_len] = '\0';
	while (uinbr != 0)
	{
		str_uinbr[uinbr_len - 1] = uinbr % 10 + '0';
		uinbr /= 10;
		uinbr_len--;
	}
	return (str_uinbr);
}

int	ft_printf_uint(unsigned int uinbr)
{
	int		uilen;
	char	*str_uinbr;

	uilen = 0;
	if (uinbr != 0)
	{
		str_uinbr = ft_uitoa(uinbr);
		uilen += ft_printf_str(str_uinbr);
		free(str_uinbr);
	}
	else
		uilen += write(1, "0", 1);
	return (uilen);
}

int	ft_printf_int(int nbr)
{
	int		nbrlen;
	char	*strnbr;

	nbrlen = 0;
	strnbr = ft_itoa(nbr);
	nbrlen = ft_printf_str(strnbr);
	free(strnbr);
	return (nbrlen);
}
