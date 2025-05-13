#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define TK_WORD 0
#define TK_RESERVED 1
#define TK_OP 2
#define TK_EOF 3
#define MAX_ARGS 256

extern char				**environ;
typedef struct s_token
{
	char				*word;
	int					kind;
	int					joined;
	struct s_token		*next;
}						t_token;

typedef struct s_context
{
	int					syntax_error;
}						t_context;

typedef enum e_node_kind
{
	ND_SIMPLE_CMD,
}						t_node_kind;

typedef struct s_node	t_node;
struct					s_node
{
	t_token				*args;
	t_node_kind			kind;
	t_node				*next;
};

int						handle_redirection(t_token **tok);

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
	return (strchr("|&;()<> \t\n'\"", c) != NULL);
}

int	is_operator(const char *s)
{
	static char	*ops[] = {"||", "&", "&&", ";", ";;", "(", ")", "|", "\n", ">",
			"<"};
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
	static char	*ops[] = {"||", "&", "&&", ";", ";;", "(", ")", "|", "\n", ">",
			"<"};
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
t_token	*quote_token(char **rest, char *start, int joined, t_context *ctx)
{
	char	quote;
	char	*p;
	char	*word;
	size_t	len;
	t_token	*tok;

	quote = *start++;
	p = start;
	while (*p && *p != quote)
		p++;
	if (*p != quote)
	{
		dprintf(2, "minishell: syntax error: unclosed quote\n");
		ctx->syntax_error = 1;
		return (NULL);
	}
	len = p - (start - 1) + 1;
	word = strndup(start - 1, len); // クォートごとコピー
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

t_token	*tokenize(char *line, t_context *ctx)
{
	t_token	head;
	t_token	*tok;
	t_token	*new;
	int		prev_blank;

	tok = &head;
	prev_blank = 1;
	head.next = NULL;
	while (*line)
	{
		if (consume_blank(&line, line))
		{
			prev_blank = 1;
			continue ;
		}
		if (*line == '\'' || *line == '"')
			new = quote_token(&line, line, is_joined(line, prev_blank), ctx);
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
char	*remove_quotes(const char *word)
{
	size_t	i;
	size_t	j;
	char	quote;
	size_t	len;
	char	*new_word;

	i = 0;
	j = 0;
	quote = 0;
	len = strlen(word);
	new_word = malloc(len + 1);
	if (!new_word)
		fatal_error("malloc");
	while (i < len)
	{
		if ((word[i] == '\'' || word[i] == '"') && !quote)
		{
			quote = word[i]; // クォート開始
			i++;
		}
		else if (word[i] == quote)
		{
			quote = 0; // クォート終了
			i++;
		}
		else
			new_word[j++] = word[i++];
	}
	new_word[j] = '\0';
	return (new_word);
}

void	expand_token(t_token *token)
{
	char	*tmp;

	while (token)
	{
		if (token->kind == TK_WORD && (strchr(token->word, '\'')
					|| strchr(token->word, '"')))
		{
			tmp = remove_quotes(token->word);
			free(token->word);
			token->word = tmp;
		}
		token = token->next;
	}
}

t_token	*expand_and_merge_tokens(t_token *tokens)
{
	t_token	*head;
	t_token	*cur;
	t_token	*prev;
	t_token	*node;
	size_t	newlen;
	char	*tmp;

	head = NULL;
	cur = NULL;
	prev = NULL;
	if (tokens->kind == TK_EOF)
		return (new_token(NULL, TK_EOF));
	while (tokens->kind != TK_EOF)
	{
		if (tokens->joined && prev)
		{
			newlen = strlen(prev->word) + strlen(tokens->word);
			tmp = malloc(newlen + 1);
			if (!tmp)
				fatal_error("malloc");
			strcpy(tmp, prev->word);
			strcat(tmp, tokens->word);
			free(prev->word);
			prev->word = tmp;
		}
		else
		{
			node = new_token(strdup(tokens->word), tokens->kind);
			if (!head)
				head = node;
			else
				cur->next = node;
			cur = node;
			prev = node;
		}
		tokens = tokens->next;
	}
	cur->next = new_token(NULL, TK_EOF);
	return (head);
}

bool	at_eof(t_token *tok)
{
	return (tok->kind == TK_EOF);
}

t_node	*new_node(t_node_kind kind)
{
	t_node	*node;

	node = calloc(1, sizeof(*node));
	if (!node)
		fatal_error("calloc");
	node->kind = kind;
	return (node);
}

t_token	*tokdup(t_token *tok)
{
	char	*word;

	word = strdup(tok->word);
	if (!word)
		fatal_error("strdup");
	return (new_token(word, tok->kind));
}

void	append_tok(t_token **tokens, t_token *tok)
{
	if (!*tokens)
	{
		*tokens = tok;
		return ;
	}
	append_tok(&(*tokens)->next, tok);
}

t_node	*parse(t_token *tok)
{
	t_node	*node;

	node = new_node(ND_SIMPLE_CMD);
	while (tok && !at_eof(tok))
	{
		if (handle_redirection(&tok))
			continue ;
		append_tok(&node->args, tokdup(tok));
		tok = tok->next;
	}
	return (node);
}

char	*search_path(const char *filename)
{
	char		*path;
	char		*paths;
	char		*token;
	char		full[PATH_MAX];
	struct stat	st;

	path = getenv("PATH");
	if (!path || strcmp(path, "") == 0)
		return (NULL); // PATHが空の場合はNULLを返す
	paths = strdup(path);
	if (!paths)
		fatal_error("strdup");
	token = strtok(paths, ":");
	while (token)
	{
		snprintf(full, PATH_MAX, "%s/%s", token, filename);
		if (stat(full, &st) == 0)
		{
			if (S_ISREG(st.st_mode) && access(full, X_OK) == 0)
			{
				free(paths);
				return (strdup(full));
			}
			else if (S_ISREG(st.st_mode))
			{
				free(paths);
				return (strdup(full)); // 実行権限がない場合も返す
			}
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

int	handle_redirection(t_token **tok)
{
	int	fd;

	if ((*tok)->kind == TK_OP && strcmp((*tok)->word, ">") == 0)
	{
		fd = open((*tok)->next->word, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd < 0)
			fatal_error("open");
		dup2(fd, STDOUT_FILENO);
		close(fd);
		*tok = (*tok)->next->next; // リダイレクトトークンとその次をスキップ
		return (1);
	}
	else if ((*tok)->kind == TK_OP && strcmp((*tok)->word, "<") == 0)
	{
		fd = open((*tok)->next->word, O_RDONLY);
		if (fd < 0)
			fatal_error("open");
		dup2(fd, STDIN_FILENO);
		close(fd);
		*tok = (*tok)->next->next; // リダイレクトトークンとその次をスキップ
		return (1);
	}
	return (0);
}

int	interpret(char *line, char **envp, t_context *ctx)
{
	t_token		*tokens;
	t_node		*node;
	t_token		*tok;
	char		*path;
	char		**argv;
	pid_t		pid;
	struct stat	st;

	int status, i = 0;
	tokens = tokenize(line, ctx);
	if (!tokens)
	{
		if (ctx->syntax_error)
		{
			ctx->syntax_error = 0;
			return (258);
		}
		return (127);
	}
	tokens = expand_and_merge_tokens(tokens);
	node = parse(tokens);
	if (!node || !node->args)
	{
		free_tokens(tokens);
		return (127);
	}
	expand_token(node->args);
	argv = calloc(MAX_ARGS, sizeof(char *));
	if (!argv)
		fatal_error("calloc");
	tok = node->args;
	while (tok && i < MAX_ARGS - 1)
	{
		if (handle_redirection(&tok))
			continue ;
		argv[i++] = tok->word;
		tok = tok->next;
	}
	argv[i] = NULL;
	if (!argv[0] || strcmp(argv[0], "") == 0 || strcmp(argv[0], "..") == 0)
	{
		dprintf(2, "command not found: %s\n", argv[0]);
		free(argv);
		free_tokens(tokens);
		return (127);
	}
	path = strchr(argv[0], '/') ? strdup(argv[0]) : search_path(argv[0]);
	if (!path)
	{
		dprintf(2, "command not found: %s\n", argv[0]);
		free(argv);
		free_tokens(tokens);
		return (127);
	}
	if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
	{
		dprintf(2, "minishell: %s: is a directory\n", argv[0]);
		free(path);
		free(argv);
		free_tokens(tokens);
		return (126);
	}
	if (access(path, X_OK) != 0)
	{
		if (errno == EACCES)
		{
			dprintf(2, "minishell: %s: Permission denied\n", argv[0]);
			free(path);
			free(argv);
			free_tokens(tokens);
			return (126);
		}
		dprintf(2, "command not found: %s\n", argv[0]);
		free(path);
		free(argv);
		free_tokens(tokens);
		return (127);
	}
	pid = fork();
	if (pid < 0)
		fatal_error("fork");
	if (pid == 0)
		execve(path, argv, envp);
	wait(&status);
	free(path);
	free(argv);
	free_tokens(tokens);
	return (WEXITSTATUS(status));
}

int	main(int argc, char **argv, char **envp)
{
	t_context ctx;
	char *line;
	int status;

	(void)argc;
	(void)argv;
	ctx.syntax_error = 0;
	status = 0;
	rl_outstream = stderr;
	while (1)
	{
		line = readline("minishell > ");
		if (!line)
			break ;
		if (*line)
			add_history(line);
		status = interpret(line, envp, &ctx);
		free(line);
	}
	exit(status);
}