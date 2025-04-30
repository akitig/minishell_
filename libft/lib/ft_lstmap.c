/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_lstmap.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/23 13:42:37 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:30:59 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

t_list	*ft_lstmap(t_list *lst, void *(*f)(void *), void (*del)(void *))
{
	t_list	*ans_lst;
	t_list	*new_node;

	if (lst == NULL || f == NULL)
		return (NULL);
	ans_lst = NULL;
	while (lst != NULL)
	{
		new_node = ft_lstnew((*f)(lst->content));
		if (new_node == NULL)
		{
			ft_lstclear(&ans_lst, del);
			return (NULL);
		}
		ft_lstadd_back(&ans_lst, new_node);
		lst = lst->next;
	}
	return (ans_lst);
}
