/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: somukaid <somukaid@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/30 12:01:40 by somukaid          #+#    #+#             */
/*   Updated: 2025/04/30 12:04:45 by somukaid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

int	is_metachar(char c)
{
	if (c == '|' || c == '&' || c == ';' || c == '(' || c == ')' || \
		c == '<' || c == '>' || c == ' ')
		return (1);
	return (0);
}

int	is_metachar_without_space(char c)
{
	if (c == '|' || c == '&' || c == ';' || c == '(' || c == ')' || \
		c == '<' || c == '>')
		return (1);
	return (0);
}

t_list	*line2list(char *line)
{
	t_list	*list;
	t_list	*tmp_list;
	char	*tmp;
	size_t	i;
	size_t	start;

	i = 0;
	start = 0;
	list = NULL;
	while (line[i] != '\0')
	{
		if (is_metachar(line[i]))
		{
			if (start != i)
			{
				tmp = ft_substr(line, start, i - start);
				tmp_list = ft_lstnew((void *)tmp);
				ft_lstadd_back(&list, tmp_list);
				start = i;
			}
			if (line[i] == ' ')
				i++;
			else
			{
				while (line[i] == line[i + 1])
					i++;
				i++;
				tmp = ft_substr(line, start, i - start);
				tmp_list = ft_lstnew((void *)tmp);
				ft_lstadd_back(&list, tmp_list);
			}
			start = i;
			continue ;
		}
		else if (line[i] == '\"')
		{
			i++;
			while (line[i] != '\"')
				i++;
		}
		else if (line[i] == '\'')
		{
			i++;
			while (line[i] != '\'')
				i++;
		}
		i++;
	}
	tmp = ft_substr(line, start, i - start);
	tmp_list = ft_lstnew((void *)tmp);
	ft_lstadd_back(&list, tmp_list);
	return (list);
}

char	**split_word(char *line)
{
	char	**split;
	t_list	*list;
	t_list	*new_list;

	list = line2list(line);
	if (list == NULL)
		return (NULL);
	print_list(list);
	new_list = expand(list);
	if (new_list == NULL)
		return (NULL);
	print_list(new_list);
	split = list2split(new_list);
	if (split == NULL)
		return (NULL);
	return (split);
}
