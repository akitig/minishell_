/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strjoin.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/16 06:31:16 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:31:57 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

char	*ft_strjoin(char const *s1, char const *s2)
{
	size_t	len;
	char	*ans;

	if (s1 == NULL || s2 == NULL)
		return (NULL);
	len = ft_strlen(s1) + ft_strlen(s2) + 1;
	ans = (char *)malloc(sizeof(char) * len);
	if (ans == NULL)
	{
		errno = ENOMEM;
		return (NULL);
	}
	ft_strlcpy(ans, s1, len);
	ft_strlcat(ans, s2, len);
	return (ans);
}
