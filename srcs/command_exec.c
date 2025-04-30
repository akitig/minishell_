/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   command_exec.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: somukaid <somukaid@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/30 11:23:34 by somukaid          #+#    #+#             */
/*   Updated: 2025/04/30 11:24:38 by somukaid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

char	*make_absolute_path_helper(char *pre_path, char *name)
{
	char	*path;
	char	*absolute_path;

	path = ft_strjoin(pre_path, "/");
	if (path == NULL)
		return (NULL);
	absolute_path = ft_strjoin(path, name);
	free(path);
	if (absolute_path == NULL)
		return (NULL);
	return (absolute_path);
}

void	frees(char **split)
{
	size_t	i;

	i = 0;
	while (split[i] != NULL)
	{
		free(split[i]);
		i++;
	}
	free(split);
}

int	make_absolute_path(char **name, char **envp)
{
	char	*env_path;
	char	**path_split;
	char	*command_path;
	size_t	i;

	(void)envp;
	env_path = getenv("PATH");
	path_split = ft_split(env_path, ':');
	if (path_split == NULL)
		return (1);
	i = 0;
	while (path_split[i] != NULL)
	{
		command_path = make_absolute_path_helper(path_split[i], *name);
		if (!access(command_path, X_OK))
		{
			free(*name);
			*name = command_path;
			break ;
		}
		free(command_path);
		i++;
	}
	frees(path_split);
	return (0);
}
