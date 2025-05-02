#include "../libft/libft.h"
#include <limits.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void			fatal_error(const char *msg) __attribute__((noreturn));

void	fatal_error(const char *msg)
{
	dprintf(STDERR_FILENO, "Fatal Error: %s\n", msg);
	exit(1);
}

static size_t	copy_dir(char *full_path, const char *dir)
{
	size_t	i;

	i = 0;
	while (dir[i] && i < PATH_MAX)
	{
		full_path[i] = dir[i];
		i += 1;
		if (i + 1 == PATH_MAX)
			break ;
	}
	return (i);
}

static void	append_filename(char *full_path, const char *filename, size_t i)
{
	size_t	j;

	j = 0;
	if ((int)i + 1 >= PATH_MAX - 1)
		return ;
	full_path[i] = '/';
	i += 1;
	while (filename[j] && i < PATH_MAX - 1)
	{
		full_path[i] = filename[j];
		i += 1;
		j += 1;
	}
	full_path[i] = '\0';
}

static void	build_path(char *full_path, const char *dir, const char *filename)
{
	size_t	i;

	i = copy_dir(full_path, dir);
	append_filename(full_path, filename, i);
}

static int	is_executable(char *cursor, const char *filename, char *out_path)
{
	char	*dir;
	int		success;

	success = 0;
	while (!success)
	{
		dir = ft_strtok(&cursor, ':');
		if (dir == NULL)
			break ;
		build_path(out_path, dir, filename);
		if (access(out_path, X_OK) == 0)
			success = 1;
	}
	return (success);
}

static char	*check_path(char *path_copy, const char *filename)
{
	char	full_path[PATH_MAX];
	char	*result;
	int		is_valid;

	is_valid = is_executable(path_copy, filename, full_path);
	if (is_valid)
		result = ft_strdup(full_path);
	else
		result = NULL;
	return (result);
}

char	*search_path(const char *filename)
{
	char	*env_path;
	char	*path_copy;
	char	*result;

	if (!filename)
		return (NULL);
	env_path = getenv("PATH");
	if (!env_path)
		return (NULL);
	path_copy = ft_strdup(env_path);
	if (!path_copy)
		return (NULL);
	result = check_path(path_copy, filename);
	free(path_copy);
	return (result);
}

static char	*extract_first_line(char *line)
{
	size_t	len;
	char	*first;

	len = 0;
	while (line[len])
	{
		if (line[len] == '\n')
			break ;
		len += 1;
	}
	first = malloc(len + 1);
	if (!first)
		return (NULL);
	ft_strlcpy(first, line, len + 1);
	return (first);
}

static int	skip_empty_line(char *line)
{
	char	*trimmed;
	int		is_empty;

	trimmed = ft_strtrim(line, " \t\n");
	if (!trimmed)
		return (1);
	if (*trimmed == '\0')
		is_empty = 1;
	else
		is_empty = 0;
	free(trimmed);
	return (is_empty);
}

static int	execute_cmd(char *path, char *first, char **environ, int *wstatus)
{
	pid_t	pid;
	char	*argv[2];
	int		result;

	argv[0] = first;
	argv[1] = NULL;
	pid = fork();
	if (pid < 0)
		fatal_error("fork");
	if (pid == 0)
		execve(path, argv, environ);
	wait(wstatus);
	result = WEXITSTATUS(*wstatus);
	return (result);
}

int	interpret(char *line)
{
	extern char	**environ;
	char		*path;
	char		*first;
	int			wstatus;
	int			status;

	first = extract_first_line(line);
	if (!first)
		return (127);
	if (skip_empty_line(first))
	{
		free(first);
		return (127);
	}
	if (ft_strchr(first, '/'))
		path = ft_strdup(first);
	else
		path = search_path(first);
	if (!path || access(path, X_OK) != 0)
	{
		dprintf(STDERR_FILENO, "command not found: %s\n", first);
		free(first);
		free(path);
		return (127);
	}
	wstatus = 0;
	status = execute_cmd(path, first, environ, &wstatus);
	free(first);
	free(path);
	return (status);
}

int	main(void)
{
	int status;
	char *line;

	rl_outstream = stderr;
	status = 0;
	while (1)
	{
		line = readline("minishell > ");
		if (!line)
			break ;
		if (*line)
			add_history(line);
		status = interpret(line);
		free(line);
	}
	exit(status);
}