#include <limits.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define TK_WORD 0
#define TK_RESERVED 1
#define TK_OP 2
#define TK_EOF 3

int					g_syntax_error = 0;

typedef struct s_token
{
	char			*word;
	int				kind;
	struct s_token	*next;
}					t_token;

void	fatal_error(const char *msg)
{
	dprintf(STDERR_FILENO, "Fatal Error: %s\n", msg);
	exit(1);
}

t_token	*new_token(char *word, int kind)
{
	t_token	*tok;

	tok = calloc(1, sizeof(*tok));
	if (!tok)
		fatal_error("calloc");
	tok->word = word;
	tok->kind = kind;
	return (tok);
}

int	is_blank(char c)
{
	return (c == ' ' || c == '\t' || c == '\n');
}

int	consume_blank(char **rest, char *line)
{
	if (is_blank(*line))
	{
		while (*line && is_blank(*line))
			line++;
		*rest = line;
		return (1);
	}
	*rest = line;
	return (0);
}

int	startswith(const char *s, const char *kw)
{
	return (strncmp(s, kw, strlen(kw)) == 0);
}

int	is_metacharacter(char c)
{
	return (strchr("|&;()<> \t\n", c) != NULL);
}

int	is_operator(const char *s)
{
	static char	*ops[] = {"||", "&", "&&", ";", ";;", "(", ")", "|", "\n"};
	size_t		i;

	i = 0;
	while (i < sizeof(ops) / sizeof(*ops))
	{
		if (startswith(s, ops[i]))
			return (1);
		i++;
	}
	return (0);
}

int	is_word(const char *s)
{
	return (*s && !is_metacharacter(*s));
}

t_token	*operator_token(char **rest, char *line)
{
	static char	*ops[] = {"||", "&", "&&", ";", ";;", "(", ")", "|", "\n"};
	char		*op;
	size_t		i;

	i = 0;
	while (i < sizeof(ops) / sizeof(*ops))
	{
		if (startswith(line, ops[i]))
		{
			op = strdup(ops[i]);
			if (!op)
				fatal_error("strdup");
			*rest = line + strlen(op);
			return (new_token(op, TK_OP));
		}
		i++;
	}
	fatal_error("Unexpected operator");
	return (NULL);
}

t_token	*word_token(char **rest, char *line)
{
	const char	*start = line;
	char		*word;

	while (*line && !is_metacharacter(*line))
		line++;
	word = strndup(start, line - start);
	if (!word)
		fatal_error("strndup");
	*rest = line;
	return (new_token(word, TK_WORD));
}
t_token	*quote_token(char **rest, char *line)
{
	char	*start;
	char	*word;
	size_t	len;
	char	quote;

	quote = *line;
	line++;
	start = line;
	while (*line && *line != quote)
		line++;
	if (*line != quote)
	{
		dprintf(2, "minishell: syntax error near ");
		if (quote == '\'')
			dprintf(2, "Unclosed single quote\n");
		else
			dprintf(2, "Unclosed double quote\n");
		g_syntax_error = 1;
		return (NULL);
	}
	len = line - start;
	word = strndup(start, len);
	if (!word)
		fatal_error("strndup");
	*rest = line + 1;
	return (new_token(word, TK_WORD));
}
t_token	*tokenize(char *line)
{
	t_token	head;
	t_token	*tok;
	t_token	*new;

	head.next = NULL;
	tok = &head;
	while (*line)
	{
		if (consume_blank(&line, line))
			continue ;
		else if (*line == '\'' || *line == '"')
		{
			new = quote_token(&line, line);
			if (!new)
				return (NULL);
			tok = tok->next = new;
		}
		else if (is_operator(line))
			tok = tok->next = operator_token(&line, line);
		else if (is_word(line))
			tok = tok->next = word_token(&line, line);
		else
			fatal_error("Unexpected token");
	}
	tok->next = new_token(NULL, TK_EOF);
	return (head.next);
}

char	*search_path(const char *filename)
{
	char	*path;
	char	*paths;
	char	*token;
	char	full[PATH_MAX];

	path = getenv("PATH");
	paths = strdup(path);
	token = strtok(paths, ":");
	while (token)
	{
		snprintf(full, PATH_MAX, "%s/%s", token, filename);
		if (access(full, X_OK) == 0)
		{
			free(paths);
			return (strdup(full));
		}
		token = strtok(NULL, ":");
	}
	free(paths);
	return (NULL);
}

void	free_tokens(t_token *token)
{
	t_token	*tmp;

	while (token)
	{
		tmp = token->next;
		free(token->word);
		free(token);
		token = tmp;
	}
}

char	*remove_quotes(const char *word)
{
	size_t	len;
	char	*new_word;
	size_t	i;
	size_t	j;

	len = strlen(word);
	new_word = malloc(len + 1);
	i = 0;
	j = 0;
	if (!new_word)
		fatal_error("malloc");
	while (word[i])
	{
		if (word[i] != '\'' && word[i] != '"')
			new_word[j++] = word[i];
		i++;
	}
	new_word[j] = '\0';
	return (new_word);
}

void	expand_token(t_token *token)
{
	char	*new_word;

	while (token)
	{
		if (token->kind == TK_WORD && (strchr(token->word, '\'')
					|| strchr(token->word, '"')))
		{
			new_word = remove_quotes(token->word);
			free(token->word);
			token->word = new_word;
		}
		token = token->next;
	}
}

#define MAX_ARGS 256

int	interpret(char *line, char **envp)
{
	t_token	*token;
	t_token	*head;
	char	*path;
	char	*argv[MAX_ARGS];
	int		status;
	pid_t	pid;
	int		i;

	token = tokenize(line);
	if (!token)
	{
		if (g_syntax_error)
		{
			g_syntax_error = 0;
			return (258);
		}
		return (127);
	}
	head = token;
	expand_token(token);
	i = 0;
	while (token && token->kind == TK_WORD && i < MAX_ARGS - 1)
	{
		argv[i++] = token->word;
		token = token->next;
	}
	argv[i] = NULL;
	if (strchr(argv[0], '/'))
		path = strdup(argv[0]);
	else
		path = search_path(argv[0]);
	if (!path || access(path, X_OK) != 0)
	{
		dprintf(2, "command not found: %s\n", argv[0]);
		free(path);
		free_tokens(head);
		return (127);
	}
	pid = fork();
	if (pid < 0)
		fatal_error("fork");
	if (pid == 0)
		execve(path, argv, envp);
	wait(&status);
	free(path);
	free_tokens(head);
	return (WEXITSTATUS(status));
}

int	main(int argc, char **argv, char **envp)
{
	char	*line;
	int		status;

	(void)argc;
	(void)argv;
	rl_outstream = stderr;
	status = 0;
	while (1)
	{
		line = readline("minishell > ");
		if (!line)
			break ;
		if (*line)
			add_history(line);
		status = interpret(line, envp);
		free(line);
	}
	exit(status);
}
