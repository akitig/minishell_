#include "../libft/libft.h"
#include <readline/history.h>
#include <readline/readline.h>
#include <sys/types.h>
#include <sys/wait.h>

void		fatal_error(const char *msg) __attribute__((noreturn));

void	fatal_error(const char *msg)
{
	dprintf(STDERR_FILENO, "Fatal Error: %s\n", msg);
	exit(1);
}

static void	build_path(char *full_path, const char *dir, const char *filename)
{
	size_t	i;
	size_t	j;

	i = 0;
	j = 0;
	while (dir[i] && i < PATH_MAX - 1)
	{
		full_path[i] = dir[i];
		i++;
	}
	if (i + 1 >= PATH_MAX - 1)
		return ;
	full_path[i++] = '/';
	while (filename[j] && i < PATH_MAX - 1)
		full_path[i++] = filename[j++];
	full_path[i] = '\0';
}

static char	*check_path(char *path_copy, const char *filename)
{
	char	*cursor;
	char	*dir;
	char	full_path[PATH_MAX];

	cursor = path_copy;
	while ((dir = ft_strtok(&cursor, ':')))
	{
		build_path(full_path, dir, filename);
		if (access(full_path, X_OK) == 0)
			return (ft_strdup(full_path));
	}
	return (NULL);
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

int	interpret(char *line)
{
	extern char	**environ;
	char		*path;
	char		*argv[] = {line, NULL};
	pid_t		pid;
	int			wstatus;

	path = search_path(line);
	if (!path)
	{
		dprintf(STDERR_FILENO, "command not found: %s\n", line);
		return (127);
	}
	pid = fork();
	if (pid < 0)
		fatal_error("fork");
	else if (pid == 0)
	{
		// child process
		execve(path, argv, environ);
		fatal_error("execve");
	}
	// parent process
	wait(&wstatus);
	free(path);
	return (WEXITSTATUS(wstatus));
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
		if (line == NULL)
			break ;
		if (*line)
			add_history(line);
		status = interpret(line);
		free(line);
	}
	exit(status);
}