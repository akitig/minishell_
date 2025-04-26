/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strtrim.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/16 07:33:11 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:32:23 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

int	ft_with_c(char c, const char *str)
{
	size_t	i;

	i = 0;
	while (str[i] != '\0')
	{
		if (str[i] == c)
			return (1);
		i++;
	}
	return (0);
}

char	*ft_strtrim(char const *s1, char const *set)
{
	size_t	i;
	size_t	start;
	size_t	end;
	char	*ans;

	if (!s1 || !set)
		return (NULL);
	start = 0;
	while (s1[start] && ft_with_c(s1[start], set))
		start++;
	end = ft_strlen(s1);
	while (start < end && ft_with_c(s1[end - 1], set))
		end--;
	ans = (char *)malloc(sizeof(*s1) * (end - start + 1));
	if (!ans)
	{
		errno = ENOMEM;
		return (NULL);
	}
	i = 0;
	while (start < end)
		ans[i++] = s1[start++];
	ans[i] = 0;
	return (ans);
}
