/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_substr.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/23 13:51:09 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:32:26 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

char	*ft_substr(char const *s, unsigned int start, size_t len)
{
	char	*ans;
	size_t	ans_len;

	if (!s)
		return (NULL);
	else if (ft_strlen(s) <= start || len == 0)
		return (ft_strdup(""));
	else if (start < ft_strlen(s))
	{
		if (start + len <= ft_strlen(s))
			ans_len = len;
		else
			ans_len = ft_strlen(s) - start;
	}
	else
		ans_len = len;
	ans = (char *)malloc(sizeof(char) * (ans_len + 1));
	if (ans == NULL)
	{
		errno = ENOMEM;
		return (NULL);
	}
	ft_strlcpy(ans, s + start, ans_len + 1);
	return (ans);
}
