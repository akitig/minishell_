/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: somukaid <somukaid@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/30 12:08:02 by somukaid          #+#    #+#             */
/*   Updated: 2025/04/30 12:09:33 by somukaid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

char	**list2split(t_list *list)
{
	char	**split;
	size_t	size;
	size_t	i;

	i = 0;
	size = ft_lstsize(list);
	split = (char **)malloc(sizeof(char *) * size + 1);
	if (split == NULL)
		return (NULL);
	split[size] = NULL;
	while (list != NULL)
	{
		split[i] = ft_strdup((char *)list->content);
		if (split[i] == NULL)
			frees(split);
		list = list->next;
		i++;
	}
	return (split);
}

void	print_list(t_list *list)
{
	t_list	*tmp;

	tmp = list;
	printf("print_list\n");
	while (tmp != NULL)
	{
		printf("%s\n", (char *)tmp->content);
		tmp = tmp->next;
	}
}
