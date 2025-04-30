/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   minishell.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: somukaid <somukaid@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/24 15:10:25 by somukaid          #+#    #+#             */
/*   Updated: 2025/04/30 12:11:38 by somukaid         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MINISHELL_H
# define MINISHELL_H

# include <stdio.h>
# include <unistd.h>
# include <sys/wait.h>
# include <readline/readline.h>
# include <readline/history.h>
# include "../libft/libft.h"

// command_exec.c
char	*make_absolute_path_helper(char *pre_path, char *name);
void	frees(char **split);
int	make_absolute_path(char **name, char **envp);

//expansion.c
t_list	*un_single_quote(t_list *list);
t_list	*un_double_quote(t_list *list);
t_list	*expand(t_list *list);

//lexer.c
int	is_metachar(char c);
int	is_metachar_without_space(char c);
t_list	*line2list(char *line);
char	**split_word(char *line);

//utils.c
char	**list2split(t_list *list);
void	print_list(t_list *list);

#endif
