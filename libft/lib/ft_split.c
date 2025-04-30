/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_split.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/16 07:43:05 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:31:38 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

size_t	word_count(char *str, char c)
{
	size_t	i;
	size_t	count;
	int		flg;

	i = 0;
	count = 0;
	flg = 0;
	while (str[i] != '\0')
	{
		if (str[i] == c)
			flg = 0;
		else if (flg == 0)
		{
			flg = 1;
			count++;
		}
		i++;
	}
	return (count);
}

void	write_word(char *word, char *start, char c)
{
	size_t	i;

	i = 0;
	while (start[i] != c && start[i] != '\0')
	{
		word[i] = start[i];
		i++;
	}
	word[i] = '\0';
}

void	*null_free(char **ans, int wnum)
{
	while (wnum > 0)
	{
		wnum--;
		free(ans[wnum]);
	}
	free(ans);
	return (NULL);
}

void	*ft_allocate_word(char **ans, char *str, char c)
{
	size_t	wc;
	size_t	i;
	size_t	j;

	wc = 0;
	i = 0;
	j = 0;
	while (str[i])
	{
		if (str[i] == c)
			i++;
		else
		{
			j = 0;
			while (str[i + j] != c && str[i + j] != '\0')
				j++;
			ans[wc] = (char *)malloc(sizeof(char) * (j + 1));
			if (!ans[wc])
				return (null_free(ans, wc));
			write_word(ans[wc], &str[i], c);
			i += j;
			wc++;
		}
	}
	return (ans);
}

char	**ft_split(const char *s, char c)
{
	char	**ans;
	char	*str;
	size_t	wc;

	if (!s)
		return (NULL);
	str = (char *)s;
	wc = word_count(str, c);
	ans = (char **)malloc(sizeof(char *) * (wc + 1));
	if (!ans)
		return (NULL);
	ans[wc] = 0;
	ans = ft_allocate_word(ans, str, c);
	if (!ans)
		return (NULL);
	return (ans);
}
