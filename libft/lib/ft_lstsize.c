/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_lstsize.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/23 13:42:46 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:31:06 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

int	ft_lstsize(t_list *lst)
{
	size_t	ans;

	ans = 0;
	while (lst != NULL)
	{
		lst = lst->next;
		ans++;
	}
	return (ans);
}
