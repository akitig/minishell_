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
#define MAX_ARGS 256

int					g_syntax_error = 0;

typedef struct s_token
{
	char			*word;
	int				kind;
	int joined; // ← 前のトークンと連結すべきなら 1
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

t_token	*word_token(char **rest, char *start, int joined)
{
	char	*p;
	char	*word;
	t_token	*tok;

	p = start;
	while (*p && !is_metacharacter(*p))
		p++;
	word = strndup(start, p - start);
	if (!word)
		fatal_error("strndup");
	*rest = p;
	tok = new_token(word, TK_WORD);
	tok->joined = joined;
	return (tok);
}

t_token	*quote_token(char **rest, char *start, int joined)
{
	char	quote;
	char	*p;
	char	*word;
	t_token	*tok;

	quote = *start;
	start++; // skip quote
	p = start;
	while (*p && *p != quote)
		p++;
	if (*p != quote)
	{
		dprintf(2, "minishell: syntax error: unclosed quote\n");
		g_syntax_error = 1;
		return (NULL);
	}
	word = strndup(start, p - start);
	if (!word)
		fatal_error("strndup");
	*rest = p + 1;
	tok = new_token(word, TK_WORD);
	tok->joined = joined;
	return (tok);
}

int	is_joined(const char *line, int prev_blank)
{
	if (*line == '\0')
		return (0);
	if (*line == '\'' || *line == '"')
		return (!prev_blank);
	if (is_word(line))
		return (!prev_blank);
	return (0);
}

t_token	*tokenize(char *line)
{
	t_token	head;
	t_token	*tok;
	t_token	*new;
	int		prev_blank;

	prev_blank = 1;
	head.next = NULL;
	tok = &head;
	while (*line)
	{
		if (consume_blank(&line, line))
		{
			prev_blank = 1;
			continue ;
		}
		if (*line == '\'' || *line == '"')
			new = quote_token(&line, line, is_joined(line, prev_blank));
		else if (is_word(line))
			new = word_token(&line, line, is_joined(line, prev_blank));
		else if (is_operator(line))
		{
			new = operator_token(&line, line);
			prev_blank = 1;
		}
		else
			fatal_error("Unexpected token");
		if (!new)
			return (NULL);
		tok = tok->next = new;
		prev_blank = 0;
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

t_token	*expand_and_merge_tokens(t_token *tokens)
{
	t_token	*head;
	t_token	**cur;
	t_token	*t;
	char	*merged;
	char	*tmp;

	head = NULL;
	cur = &head;
	t = tokens;
	merged = NULL;
	while (t && t->kind != TK_EOF)
	{
		if (t->joined)
		{
			if (!merged)
				merged = strdup(t->word);
			else
			{
				tmp = malloc(strlen(merged) + strlen(t->word) + 1);
				if (!tmp)
					fatal_error("malloc");
				strcpy(tmp, merged);
				strcat(tmp, t->word);
				free(merged);
				merged = tmp;
			}
		}
		else
		{
			if (merged)
			{
				tmp = malloc(strlen(merged) + strlen(t->word) + 1);
				if (!tmp)
					fatal_error("malloc");
				strcpy(tmp, merged);
				strcat(tmp, t->word);
				*cur = new_token(tmp, TK_WORD);
				free(merged);
				merged = NULL;
			}
			else
				*cur = new_token(strdup(t->word), t->kind);
			cur = &(*cur)->next;
		}
		t = t->next;
	}
	*cur = new_token(NULL, TK_EOF);
	return (head);
}

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
	for (t_token *t = token; t && t->kind != TK_EOF; t = t->next)
		fprintf(stderr, "[%s] joined=%d\n", t->word, t->joined);
	if (!token)
	{
		if (g_syntax_error)
		{
			g_syntax_error = 0;
			return (258);
		}
		return (127);
	}
	token = expand_and_merge_tokens(token);
	head = token;
	i = 0;
	while (token && token->kind == TK_WORD && i < MAX_ARGS - 1)
	{
		argv[i++] = token->word;
		token = token->next;
	}
	argv[i] = NULL;
	if (!argv[0])
		return (127);
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
	{
		// fprintf(stderr, "=== ARGV ===\n");
		// for (int i = 0; argv[i]; i++)
		// 	fprintf(stderr, "argv[%d] = [%s]\n", i, argv[i]);
		// fprintf(stderr, "============\n");
		execve(path, argv, envp);
	}
	wait(&status);
	free(path);
	free_tokens(head);
	return (WEXITSTATUS(status));
}

int	main(int argc, char **argv, char **envp)
{
	char *line;
	int status;

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