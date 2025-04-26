/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: somukaid <somukaid@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/24 15:09:52 by somukaid          #+#    #+#             */
/*   Updated: 2025/04/26 16:02:00 by somukaid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

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

int	is_metachar(char c)
{
	if (c == '|' || c == '&' || c == ';' || c == '(' || c == ')' || c == '<' || c == '>' || c == ' ') 
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
	while (line[i] != '\0')
	{
		if (is_metachar(line[i]))
		{
			if (i != 0)
			{
				tmp = ft_substr(line, start, i - start);
				tmp_list = ft_lstnew((void *)tmp);
				ft_lstadd_back(&list, tmp_list);
			}
			while (line[i] == ' ')
				i++;
			start = i;
			if (is_metachar(line[i]))
			{
			while (is_metachar(line[i]) && line[i] != ' ')
				i++;
			tmp = ft_substr(line, start, i - start);
			tmp_list = ft_lstnew((void *)tmp);
			ft_lstadd_back(&list, tmp_list);
			while (line[i] == ' ')
				i++;
			start = i;
			}
		}
		i++;
	}
	tmp = ft_substr(line, start, i - start);
	tmp_list = ft_lstnew((void *)tmp);
	ft_lstadd_back(&list, tmp_list);
	return (list);
}

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

char	**split_word(char *line)
{
	char	**split;
	t_list	*list;

	list = line2list(line);
	if (list == NULL)
		return (NULL);
	print_list(list);
	split = list2split(list);
	if (split == NULL)
		return (NULL);
	return (split);
}
	
void	command_exec(char *line, char **envp)
{
	int		forkid;
	int		status;
	char	**split;

	split = NULL;
	split = split_word(line);
	make_absolute_path(&(split[0]), envp);
	forkid = fork();
	if (forkid == 0)
	{
		execve(split[0], split, envp);
		exit(1);
	}
	else
	{
		wait(&status);
		if (split != NULL)
			frees(split);
	}
	return ;
}

int	minishell(char **envp)
{
	char	*line;

	while (1)
	{
		line = readline("prompt> ");
		command_exec(line, envp);
	}
	return (0);
}

int	main(int ac, char **ag, char **envp)
{
	(void)ac;
	(void)ag;
	minishell(envp);
	return (0);
}
