#include "libft/libft.h"
#include <readline/history.h>
#include <readline/readline.h>

int	main(void)
{
	char *line;

	rl_outstream = stderr;
	while (1)
	{
		line = readline("minishell > ");
		if (line == NULL)
			break ;
		if (*line)
			add_history(line);
		free(line);
	}
	exit(0);
}