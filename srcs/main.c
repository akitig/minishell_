/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: somukaid <somukaid@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/24 15:09:52 by somukaid          #+#    #+#             */
/*   Updated: 2025/04/30 12:13:54 by somukaid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/minishell.h"

int	command_exec(char *line, char **envp)
{
	int		forkid;
	int		status;
	char	**split;

	split = split_word(line);
	if (split)
		make_absolute_path(&(split[0]), envp);
	forkid = fork();
	if (forkid < 0)
	{
		perror("fork");
		return (1);
	}
	if (forkid == 0)
	{
		execve(split[0], split, envp);
		perror("execve");
		exit(1);
	}
	else
	{
		wait(&status);
		if (split != NULL)
			frees(split);
	}
	return (status);
}

int	minishell(char **envp)
{
	char	*line;
	int		status;

	status = 0;
	while (1)
	{
		line = readline("prompt> ");
		if (line == NULL)
			break ;
		if (*line)
			add_history(line);
		status = command_exec(line, envp);
		free(line);
	}
	return (status);
}

int	main(int ac, char **ag, char **envp)
{
	(void)ac;
	(void)ag;
	minishell(envp);
	return (0);
}
