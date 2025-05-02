/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strtok.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/02 13:28:11 by akunimot          #+#    #+#             */
/*   Updated: 2025/05/02 13:28:14 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
** ft_strtok: ポインタを進めながら delim 区切りで1トークンずつ取り出す
** str_ptr: 分割対象のポインタのアドレス
** delim: 区切り文字（例: ':'）
** 戻り値: トークンへのポインタ（呼び出し元でfree不要）
*/

#include "../libft.h"

char	*ft_strtok(char **str_ptr, char delim)
{
	char	*start;
	char	*cur;

	if (!str_ptr || !*str_ptr || !**str_ptr)
		return (NULL);
	start = *str_ptr;
	cur = start;
	while (*cur && *cur != delim)
		cur++;
	if (*cur == delim)
	{
		*cur = '\0';
		*str_ptr = cur + 1;
	}
	else
	{
		*str_ptr = cur;
	}
	return (start);
}
