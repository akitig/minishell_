/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expansion.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: somukaid <somukaid@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/30 11:51:06 by somukaid          #+#    #+#             */
/*   Updated: 2025/04/30 14:41:44 by somukaid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

t_list	*un_single_quote(t_list *list)
{
	t_list	*tmp;
	char	*str;
	char	*first;
	char	*second;
	size_t	i;
	size_t	count;

	tmp = list;
	while (tmp != NULL)
	{
		str = (char *)tmp->content;
		i = 0;
		while (str[i] != '\0')
		{
			if (str[i] == '\'')
			{
				first = ft_substr(str, 0, i);
				second = ft_substr(str, i + 1, ft_strlen(str) - i - 1);
				free(tmp->content);
				tmp->content = ft_strjoin(first, second);
				free(first);
				free(second);
				str = (char *)tmp->content;
				count++;
			}
			i++;
		}
		if (count % 2 != 0)
		{
			printf("minishell: syntax error near Unclosed single quote\n");
			return (NULL);
		}
		tmp = tmp->next;
	}
	return (list);
}

t_list	*un_double_quote(t_list *list)
{
	t_list	*tmp;
	char	*str;
	char	*first;
	char	*second;
	size_t	i;
	size_t	count;

	tmp = list;
	while (tmp != NULL)
	{
		str = (char *)tmp->content;
		i = 0;
		while (str[i] != '\0')
		{
			if (str[i] == '"')
			{
				first = ft_substr(str, 0, i);
				second = ft_substr(str, i + 1, ft_strlen(str) - i - 1);
				free(tmp->content);
				tmp->content = ft_strjoin(first, second);
				free(first);
				free(second);
				str = (char *)tmp->content;
				count++;
			}
			i++;
		}
		if (count % 2 != 0)
		{
			printf("minishell: syntax error near Unclosed double quote\n");
			return (NULL);
		}
		tmp = tmp->next;
	}
	return (list);
}

t_list	*expand(t_list *list)
{
	t_list	*new_list;

	new_list = un_single_quote(list);
	if (new_list == NULL)
		return (NULL);
	new_list = un_double_quote(list);
	if (new_list == NULL)
		return (NULL);
	return (new_list);
}
