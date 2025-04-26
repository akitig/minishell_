/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_memset.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/23 13:40:35 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:31:24 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

void	*ft_memset(void *buf, int ch, size_t len)
{
	size_t			i;
	unsigned char	*ans;

	i = 0;
	ans = (unsigned char *)buf;
	while (i < len)
	{
		ans[i] = ch;
		i++;
	}
	return (ans);
}
