/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_memcpy.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/26 21:49:26 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:31:16 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

void	*ft_memcpy(void *dst, const void *src, size_t n)
{
	size_t			i;
	unsigned char	*d;
	unsigned char	*s;

	d = (unsigned char *)dst;
	s = (unsigned char *)src;
	i = 0;
	if (!d && !s)
		return (NULL);
	while (i < n)
	{
		d[i] = s[i];
		i++;
	}
	return (d);
}

/*
#include <stdio.h>

void	check_memcpy(void *dst, void *src, size_t size)
{
	void	*expected;

	expected = malloc(size);
	memcpy(expected, src, size);
	ft_memcpy(dst, src, size);
	printf("dst:%s, src:%s, size:%zu\n", dst, src, size);
	// ASSERT_EQ_MEM(dst, expected, size);
	if (expected)
		free(expected);
}

*/