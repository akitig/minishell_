/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/14 10:00:00 by akitig            #+#    #+#             */
/*   Updated: 2025/05/14 10:30:49 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <readline/history.h>
#include <readline/readline.h>
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

extern char			**environ;

typedef struct s_token
{
	char			*word;
	int				kind;
	int				joined;
	struct s_token	*next;
}					t_token;

typedef struct s_context
{
	int				syntax_error;
}					t_context;

typedef enum e_node_kind
{
	ND_SIMPLE_CMD
}					e_node_kind;

typedef struct s_node
{
	t_token			*args;
	e_node_kind		kind;
	char			*infile;
	char			*outfile;
	int				append;
}					t_node;

/* error handling */
void	fatal_error(const char *msg)
{
	dprintf(STDERR_FILENO, "Fatal Error: %s\n", msg);
	exit(1);
}

char	*strdup_safe(const char *s)
{
	char	*r;

	r = strdup(s);
	if (!r)
		fatal_error("strdup");
	return (r);
}

/* token constructors */
t_token	*new_token(char *w, int k)
{
	t_token	*t;

	t = calloc(1, sizeof(*t));
	if (!t)
		fatal_error("calloc");
	t->word = w;
	t->kind = k;
	t->joined = 0;
	return (t);
}

t_token	*tokdup(t_token *tok)
{
	t_token	*n;

	n = new_token(strdup_safe(tok->word), tok->kind);
	n->joined = tok->joined;
	return (n);
}

void	append_tok(t_token **head, t_token *tok)
{
	t_token	*p;

	if (!*head)
	{
		*head = tok;
		return ;
	}
	p = *head;
	while (p->next)
		p = p->next;
	p->next = tok;
}

/* lexer helpers */
int	is_blank(char c)
{
	return (c == ' ' || c == '\t' || c == '\n');
}

int	consume_blank(char **rest, char *l)
{
	if (is_blank(*l))
	{
		while (*l && is_blank(*l))
			l++;
		*rest = l;
		return (1);
	}
	*rest = l;
	return (0);
}

int	starts_with(const char *s, const char *kw)
{
	return (strncmp(s, kw, strlen(kw)) == 0);
}

int	is_metachar(char c)
{
	return (strchr("|&);()<> \t\n'\"", c) != NULL);
}

int	is_word(const char *s)
{
	return (*s && !is_metachar(*s));
}

int	is_operator(const char *s)
{
	static char	*ops[] = {"||", "&&", ";;", "<<", ">>", ";", "(", ")", "|",
			"\n", ">", "<"};
	size_t		i;

	i = 0;
	while (i < sizeof(ops) / sizeof(*ops))
	{
		if (starts_with(s, ops[i]))
			return (1);
		i++;
	}
	return (0);
}

int	is_joined(const char *l, int prev_b)
{
	if (*l == '\'' || *l == '"' || is_word(l))
		return (!prev_b);
	return (0);
}

/* token creators */
t_token	*word_token(char **rest, char *start, int joined)
{
	char	*p;
	char	*w;
	t_token	*t;

	p = start;
	while (*p && !is_metachar(*p))
		p++;
	w = strndup(start, p - start);
	if (!w)
		fatal_error("strndup");
	*rest = p;
	t = new_token(w, TK_WORD);
	t->joined = joined;
	return (t);
}

t_token	*quote_token(char **rest, char *start, int joined, t_context *ctx)
{
	char	q;
	char	*p;
	size_t	len;
	char	*w;
	t_token	*t;

	q = *start++;
	p = start;
	while (*p && *p != q)
		p++;
	if (*p != q)
	{
		dprintf(2, "minishell: syntax error: unclosed quote\n");
		ctx->syntax_error = 1;
		return (NULL);
	}
	len = p - (start - 1) + 1;
	w = strndup(start - 1, len);
	if (!w)
		fatal_error("strndup");
	*rest = p + 1;
	t = new_token(w, TK_WORD);
	t->joined = joined;
	return (t);
}

t_token	*operator_token(char **rest, char *l)
{
	static char	*ops[] = {"||", "&&", ";;", "<<", ">>", ";", "(", ")", "|",
			"\n", ">", "<"};
	size_t		i;
	char		*w;

	i = 0;
	while (i < sizeof(ops) / sizeof(*ops))
	{
		if (starts_with(l, ops[i]))
		{
			w = strdup_safe(ops[i]);
			*rest = l + strlen(w);
			return (new_token(w, TK_OP));
		}
		i++;
	}
	fatal_error("Unexpected operator");
	return (NULL);
}

/* lexer */
t_token	*tokenize(char *line, t_context *ctx)
{
	int		prev_b;
	t_token	head;
	t_token	*cur;
	t_token	*n;

	prev_b = 1;
	head.next = NULL;
	cur = &head;
	while (*line)
	{
		if (consume_blank(&line, line))
		{
			prev_b = 1;
			continue ;
		}
		if (*line == '\'' || *line == '"')
			n = quote_token(&line, line, is_joined(line, prev_b), ctx);
		else if (is_word(line))
			n = word_token(&line, line, is_joined(line, prev_b));
		else if (is_operator(line))
			n = operator_token(&line, line);
		else
			fatal_error("Unexpected token");
		if (!n)
			return (NULL);
		cur = cur->next = n;
		prev_b = 0;
	}
	cur->next = new_token(NULL, TK_EOF);
	return (head.next);
}

/* quote removal */
char	*remove_quotes(const char *w)
{
	size_t	i;
	size_t	j;
	size_t	len;
	char	q;
	char	*nw;

	i = 0;
	j = 0;
	len = strlen(w);
	q = 0;
	nw = malloc(len + 1);
	if (!nw)
		fatal_error("malloc");
	while (i < len)
	{
		if ((w[i] == '\'' || w[i] == '"') && !q)
			q = w[i++];
		else if (w[i] == q)
			q = 0, i++;
		else
			nw[j++] = w[i++];
	}
	nw[j] = '\0';
	return (nw);
}

void	expand_token(t_token *t)
{
	char	*tmp;

	while (t)
	{
		if (t->kind == TK_WORD && (strchr(t->word, '\'') || strchr(t->word,
					'"')))
		{
			tmp = remove_quotes(t->word);
			free(t->word);
			t->word = tmp;
		}
		t = t->next;
	}
}

/* merge joined tokens */
t_token	*expand_and_merge_tokens(t_token *t)
{
	t_token	*head;
	t_token	*cur;
	t_token	*prev;
	size_t	nl;
	char	*tmp;
	t_token	*n;

	head = NULL;
	cur = NULL;
	prev = NULL;
	while (t && t->kind != TK_EOF)
	{
		if (t->joined && prev)
		{
			nl = strlen(prev->word) + strlen(t->word);
			tmp = malloc(nl + 1);
			if (!tmp)
				fatal_error("malloc");
			strcpy(tmp, prev->word);
			strcat(tmp, t->word);
			free(prev->word);
			prev->word = tmp;
		}
		else
		{
			n = new_token(strdup_safe(t->word), t->kind);
			if (!head)
				head = n;
			else
				cur->next = n;
			cur = n;
			prev = n;
		}
		t = t->next;
	}
	if (cur)
		cur->next = new_token(NULL, TK_EOF);
	else
		head = new_token(NULL, TK_EOF);
	return (head);
}

int	at_eof(t_token *t)
{
	return (t->kind == TK_EOF);
}

/* parse: record only filenames, skip heredoc */
t_node	*parse(t_token *t)
{
	t_node	*n;

	n = calloc(1, sizeof(*n));
	if (!n)
		fatal_error("calloc");
	n->kind = ND_SIMPLE_CMD;
	n->infile = NULL;
	n->outfile = NULL;
	n->append = 0;
	while (t && t->kind != TK_EOF)
	{
		if (t->kind == TK_OP && t->next)
		{
			if (!strcmp(t->word, "<<"))
			{
				t = t->next->next;
				continue ;
			}
			if (!strcmp(t->word, "<"))
			{
				free(n->infile);
				n->infile = strdup_safe(t->next->word);
				t = t->next->next;
				continue ;
			}
			if (!strcmp(t->word, ">"))
			{
				free(n->outfile);
				n->outfile = strdup_safe(t->next->word);
				n->append = 0;
				t = t->next->next;
				continue ;
			}
			if (!strcmp(t->word, ">>"))
			{
				free(n->outfile);
				n->outfile = strdup_safe(t->next->word);
				n->append = 1;
				t = t->next->next;
				continue ;
			}
		}
		append_tok(&n->args, tokdup(t));
		t = t->next;
	}
	return (n);
}

/* read heredoc lines into pipe, return read end */
static int	handle_heredoc(const char *lim)
{
	int		p[2];
	char	*line;

	if (pipe(p) < 0)
		fatal_error("pipe");
	while ((line = readline("> ")) && strcmp(line, lim))
	{
		write(p[1], line, strlen(line));
		write(p[1], "\n", 1);
		free(line);
	}
	free(line);
	close(p[1]);
	return (p[0]);
}

/* handle << limiter redirection */
static void	handle_heredoc_redir(t_token *t)
{
	char	*lim;
	int		rfd;

	lim = remove_quotes(t->next->word);
	rfd = handle_heredoc(lim);
	free(lim);
	dup2(rfd, STDIN_FILENO);
	close(rfd);
}

/* handle <, >, >> file redirection */
static void	handle_file_redir(t_token *t)
{
	int	fd;

	if (!strcmp(t->word, "<"))
		fd = open(t->next->word, O_RDONLY);
	else if (!strcmp(t->word, ">"))
		fd = open(t->next->word, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	else
		fd = open(t->next->word, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd < 0)
		fatal_error(t->next->word);
	if (t->word[0] == '<')
		dup2(fd, STDIN_FILENO);
	else
		dup2(fd, STDOUT_FILENO);
	close(fd);
}

/* in child: open all redirects */
static void	process_redirection(t_token *tok)
{
	while (tok && tok->kind != TK_EOF)
	{
		if (!strcmp(tok->word, "<<"))
		{
			handle_heredoc_redir(tok);
			tok = tok->next->next;
		}
		else if (!strcmp(tok->word, "<") || !strcmp(tok->word, ">")
			|| !strcmp(tok->word, ">>"))
		{
			handle_file_redir(tok);
			tok = tok->next->next;
		}
		else
			tok = tok->next;
	}
}

/* command execution */
int	run_command(char *path, char **argv, t_token *toks, char **envp)
{
	pid_t	pid;
	int		status;

	pid = fork();
	if (pid < 0)
		fatal_error("fork");
	if (pid == 0)
	{
		process_redirection(toks);
		execve(path, argv, envp);
		fatal_error("execve");
	}
	wait(&status);
	return (WEXITSTATUS(status));
}

/* builtins */
int	builtin_export(char **a)
{
	char	*eq;

	for (int i = 1; a[i]; i++)
	{
		eq = strchr(a[i], '=');
		if (eq)
		{
			*eq = '\0';
			setenv(a[i], eq + 1, 1);
		}
		else
			setenv(a[i], "", 1);
	}
	return (0);
}

int	builtin_unset(char **a)
{
	for (int i = 1; a[i]; i++)
		unsetenv(a[i]);
	return (0);
}

int	handle_builtin(char **argv, t_token *t)
{
	int	r;

	r = -1;
	if (!strcmp(argv[0], "export"))
		r = builtin_export(argv);
	else if (!strcmp(argv[0], "unset"))
		r = builtin_unset(argv);
	else if (!strcmp(argv[0], "exit"))
		exit(0);
	if (r >= 0)
	{
		free(t->word);
		free(t);
		free(argv);
	}
	return (r);
}

/* build argv array */
char	**build_argv(t_token *t)
{
	char	**a;
	int		i;

	a = calloc(MAX_ARGS, sizeof(char *));
	if (!a)
		fatal_error("calloc");
	i = 0;
	while (t && i < MAX_ARGS - 1)
	{
		a[i++] = t->word;
		t = t->next;
	}
	a[i] = NULL;
	return (a);
}

/* path lookup */
char	*search_path(const char *f)
{
	char		*cp;
	char		*tok;
	char		*first;
	char		full[PATH_MAX];
	struct stat	st;

	if (!(cp = getenv("PATH")) || !*cp)
		return (NULL);
	cp = strdup_safe(cp);
	first = NULL;
	tok = strtok(cp, ":");
	while (tok)
	{
		if (!strcmp(tok, ".") || *tok == '\0')
		{
			tok = strtok(NULL, ":");
			continue ;
		}
		snprintf(full, PATH_MAX, "%s/%s", tok, f);
		if (stat(full, &st) == 0 && S_ISREG(st.st_mode))
		{
			if (!access(full, X_OK))
			{
				free(cp);
				return (strdup_safe(full));
			}
			if (!first)
				first = strdup_safe(full);
		}
		tok = strtok(NULL, ":");
	}
	free(cp);
	return (first);
}

/* external command launcher */
int	launch_external(char **argv, t_node *nd, t_token *toks, char **envp)
{
	char		*path;
	int			ret;
	struct stat	sb;

	(void)nd;
	path = strchr(argv[0], '/') ? strdup_safe(argv[0]) : search_path(argv[0]);
	if (!path)
	{
		dprintf(2, "command not found: %s\n", argv[0]);
		free(argv);
		return (127);
	}
	if (!stat(path, &sb) && S_ISDIR(sb.st_mode))
	{
		dprintf(2, "minishell: %s: is a directory\n", argv[0]);
		free(path);
		free(argv);
		return (126);
	}
	if (access(path, X_OK))
	{
		dprintf(2, "minishell: %s: Permission denied\n", argv[0]);
		free(path);
		free(argv);
		return (126);
	}
	ret = run_command(path, argv, toks, envp);
	free(path);
	free(argv);
	return (ret);
}

/* interpreter */
int	interpret(char *line, char **envp, t_context *ctx)
{
	t_token	*toks;
	t_node	*nd;
	char	**argv;
	int		bi;

	toks = tokenize(line, ctx);
	if (!toks)
		return (ctx->syntax_error ? 258 : 127);
	toks = expand_and_merge_tokens(toks);
	nd = parse(toks);
	if (!nd || !nd->args)
		return (free(toks), 127);
	expand_token(nd->args);
	argv = build_argv(nd->args);
	if (!argv[0])
		return (free(argv), free(toks), free(nd), 127);
	bi = handle_builtin(argv, toks);
	if (bi >= 0)
		return (bi);
	return (launch_external(argv, nd, toks, envp));
}

/* main loop */
int	main(int argc, char **argv, char **envp)
{
	t_context ctx;
	char *line;
	int status;

	(void)argc;
	(void)argv;
	ctx = (t_context){0};
	rl_outstream = stderr;
	while ((line = readline("minishell > ")))
	{
		if (*line)
			add_history(line);
		status = interpret(line, envp, &ctx);
		free(line);
	}
	return (status);
}