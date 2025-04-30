/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strdup.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/16 08:54:06 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:31:50 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

char	*ft_strdup(const char *s1)
{
	int		src_i;
	int		dest_i;
	char	*dest;

	src_i = 0;
	dest_i = 0;
	while (s1[src_i] != '\0')
		src_i++;
	dest = (char *)malloc(sizeof(char) * (src_i + 1));
	if (dest == NULL)
		return (NULL);
	while (s1[dest_i] != '\0')
	{
		dest[dest_i] = s1[dest_i];
		dest_i++;
	}
	dest[dest_i] = '\0';
	return (dest);
}
