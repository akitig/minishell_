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
}					t_node;

/* エラー時にメッセージを出力して exit */
void	fatal_error(const char *msg)
{
	dprintf(STDERR_FILENO, "Fatal Error: %s\n", msg);
	exit(1);
}

/* strdup のチェック付き */
char	*strdup_safe(const char *s)
{
	char	*r;

	r = strdup(s);
	if (!r)
		fatal_error("strdup");
	return (r);
}

/* トークン新規作成 */
t_token	*new_token(char *w, int k)
{
	t_token	*t;

	t = calloc(1, sizeof(*t));
	if (!t)
		fatal_error("calloc");
	t->word = w;
	t->kind = k;
	return (t);
}

/* トークン複製 */
t_token	*tokdup(t_token *tok)
{
	return (new_token(strdup_safe(tok->word), tok->kind));
}

/* 引数リスト末尾にトークン追加 */
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

/* 空白判定 */
int	is_blank(char c)
{
	return (c == ' ' || c == '\t' || c == '\n');
}

/* 空白をまとめてスキップ */
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

/* 文字列先頭比較 */
int	startswith(const char *s, const char *kw)
{
	return (strncmp(s, kw, strlen(kw)) == 0);
}

/* メタ文字判定 */
int	is_metacharacter(char c)
{
	return (strchr("|&);()<> \t\n'\"", c) != NULL);
}

/* 単語先頭文字判定 */
int	is_word(const char *s)
{
	return (*s && !is_metacharacter(*s));
}

int	is_operator(const char *s)
{
	static char	*ops[] = {"||", "&&", ";", ";;", "(", ")", "|", "\n", ">>",
			"<<", ">", "<"};

	for (size_t i = 0; i < sizeof(ops) / sizeof(*ops); i++)
		if (startswith(s, ops[i]))
			return (1);
	return (0);
}

/* 連結フラグ判定 */
int	is_joined(const char *l, int prev_blank)
{
	if (*l == '\'' || *l == '"' || is_word(l))
		return (!prev_blank);
	return (0);
}

/* 単語トークン生成 */
t_token	*word_token(char **rest, char *start, int joined)
{
	char	*p;
	char	*w;
	t_token	*t;

	p = start;
	while (*p && !is_metacharacter(*p))
		p++;
	w = strndup(start, p - start);
	if (!w)
		fatal_error("strndup");
	*rest = p;
	t = new_token(w, TK_WORD);
	t->joined = joined;
	return (t);
}

/* クォートトークン生成 */
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
/* operator_token の修正：>> と << を優先してマッチ */
t_token	*operator_token(char **rest, char *l)
{
	static char	*ops[] = {"||", "&&", ";", ";;", "(", ")", "|", "\n", ">>",
			"<<", ">", "<"};
	char		*w;

	for (size_t i = 0; i < sizeof(ops) / sizeof(*ops); i++)
	{
		if (startswith(l, ops[i]))
		{
			w = strdup_safe(ops[i]);
			*rest = l + strlen(w);
			return (new_token(w, TK_OP));
		}
	}
	fatal_error("Unexpected operator");
	return (NULL);
}

t_token	*tokenize(char *line, t_context *ctx)
{
	int	prev_blank;

	t_token head, *cur, *n;
	prev_blank = 1;
	head.next = NULL;
	cur = &head;
	while (*line)
	{
		if (consume_blank(&line, line))
		{
			prev_blank = 1;
			continue ;
		}
		n = NULL;
		if (*line == '\'' || *line == '\"')
			n = quote_token(&line, line, is_joined(line, prev_blank), ctx);
		else if (is_word(line))
			n = word_token(&line, line, is_joined(line, prev_blank));
		else if (is_operator(line))
			n = operator_token(&line, line);
		else
			fatal_error("Unexpected token");
		if (!n)
			return (NULL);
		cur->next = n;
		cur = n;
		/* 演算子のあとは必ず“空白あり”扱いに */
		prev_blank = (n->kind == TK_OP);
	}
	cur->next = new_token(NULL, TK_EOF);
	return (head.next);
}

/* クォート除去 */
char	*remove_quotes(const char *w)
{
	size_t	i;
	size_t	j;
	size_t	len;
	char	q;
	char	*nw;

	i = 0, j = 0, len = strlen(w);
	q = 0;
	nw = malloc(len + 1);
	if (!nw)
		fatal_error("malloc");
	while (i < len)
	{
		if ((w[i] == '\'' || w[i] == '\"') && !q)
			q = w[i++];
		else if (w[i] == q)
			q = 0, i++;
		else
			nw[j++] = w[i++];
	}
	nw[j] = '\0';
	return (nw);
}

/* 展開 */
void	expand_token(t_token *t)
{
	char	*tmp;

	while (t)
	{
		if (t->kind == TK_WORD && (strchr(t->word, '\'') || strchr(t->word,
					'\"')))
		{
			tmp = remove_quotes(t->word);
			free(t->word);
			t->word = tmp;
		}
		t = t->next;
	}
}
/* 簡易変数展開 */
static char	*expand_vars(const char *s)
{
	size_t	i;
	size_t	j;
	size_t	len;
	char	*res;
	char	var[NAME_MAX + 1];
	char	*val;
	size_t	k;

	i = 0, j = 0, len = strlen(s);
	res = malloc(len * 2 + 1);
	if (!res)
		fatal_error("malloc");
	while (s[i])
	{
		if (s[i] == '$' && (isalnum((unsigned char)s[i + 1]) || s[i
				+ 1] == '_'))
		{
			k = 0;
			i++;
			while (s[i] && (isalnum((unsigned char)s[i]) || s[i] == '_')
				&& k < NAME_MAX)
				var[k++] = s[i++];
			var[k] = '\0';
			val = getenv(var);
			if (val)
			{
				strcpy(res + j, val);
				j += strlen(val);
			}
		}
		else
			res[j++] = s[i++];
	}
	res[j] = '\0';
	return (res);
}

/* 連結＋EOF追加 */
t_token	*expand_and_merge_tokens(t_token *t)
{
	t_token	*head;
	t_token	*cur;
	t_token	*prev;
	size_t	nl;
	char	*tmp;
	t_token	*n;

	head = NULL, cur = NULL, prev = NULL;
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

/* EOF判定 */
int	at_eof(t_token *t)
{
	return (t->kind == TK_EOF);
}

/* ASTノード作成 */
t_node	*new_node(e_node_kind k)
{
	t_node	*n;

	n = calloc(1, sizeof(*n));
	if (!n)
		fatal_error("calloc");
	n->kind = k;
	return (n);
}

/* 構文解析 */

/* 構文解析：>, >>, <, << をスキップ */
t_node	*parse(t_token *t)
{
	t_node	*n;

	n = new_node(ND_SIMPLE_CMD);
	while (t && !at_eof(t))
	{
		if (t->kind == TK_OP && (strcmp(t->word, ">") == 0 || strcmp(t->word,
					">>") == 0 || strcmp(t->word, "<") == 0 || strcmp(t->word,
					"<<") == 0))
		{
			t = t->next->next;
			continue ;
		}
		append_tok(&n->args, tokdup(t));
		t = t->next;
	}
	return (n);
}
/* PATH 探索：実行可能ファイルを優先、なければ通常ファイルを返す */
char	*search_path(const char *f)
{
	char		*cp;
	char		*tok;
	char		*first_fail;
	struct stat	st;
	char		full[PATH_MAX];

	/* PATH 環境変数取得＆空チェック */
	if (!(cp = getenv("PATH")) || !*cp)
		return (NULL);
	/* 複製して切り出し用にする */
	cp = strdup_safe(cp);
	first_fail = NULL;
	/* ':' 区切りで１件ずつ調査 */
	tok = strtok(cp, ":");
	while (tok)
	{
		/* "." または空要素はスキップ */
		if (!strcmp(tok, ".") || *tok == '\0')
		{
			tok = strtok(NULL, ":");
			continue ;
		}
		/* ディレクトリ + "/" + コマンド名 */
		snprintf(full, PATH_MAX, "%s/%s", tok, f);
		/* 存在かつ通常ファイルか？ */
		if (stat(full, &st) == 0 && S_ISREG(st.st_mode))
		{
			/* 実行権限ありなら即返却 */
			if (access(full, X_OK) == 0)
			{
				free(cp);
				return (strdup_safe(full));
			}
			/* 最初の “非実行可能” ファイルを覚えておく */
			if (!first_fail)
				first_fail = strdup_safe(full);
		}
		tok = strtok(NULL, ":");
	}
	free(cp);
	/* 実行可能はなかったが通常ファイルが見つかっていればそれを返す */
	return (first_fail);
}
/* トークン解放 */
void	free_tokens(t_token *t)
{
	t_token	*nx;

	while (t)
	{
		nx = t->next;
		free(t->word);
		free(t);
		t = nx;
	}
}

/* built-in export */
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

/* built-in unset */
int	builtin_unset(char **a)
{
	for (int i = 1; a[i]; i++)
		unsetenv(a[i]);
	return (0);
}

/* built-in処理 */
int	handle_builtin(char **argv, t_token *t)
{
	int	r;

	r = -1;
	if (strcmp(argv[0], "export") == 0)
		r = builtin_export(argv);
	else if (strcmp(argv[0], "unset") == 0)
		r = builtin_unset(argv);
	else if (strcmp(argv[0], "exit") == 0)
		exit(0);
	if (r >= 0)
	{
		free_tokens(t);
		free(argv);
	}
	return (r);
}

/* argv生成 */
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
void	apply_input_redirection(t_token *t)
{
	bool	used;
	char	*ev;

	int fd, pipefd[2], do_exp;
	char *orig, *tmp, *delim, *line2;
	used = false;
	while (t && !at_eof(t))
	{
		if (t->kind == TK_OP && strcmp(t->word, "<<") == 0 && t->next)
		{
			orig = t->next->word;
			/* 前のヒアドキュメント用パイプを捨てる */
			if (used)
			{
				close(pipefd[0]);
				close(pipefd[1]);
			}
			if (pipe(pipefd) < 0)
				fatal_error("pipe");
			used = true;
			/* 区切り文字からクォートを除去 */
			tmp = strdup_safe(orig);
			delim = remove_quotes(tmp);
			free(tmp);
			/* 区切り文字にクォートがなければ変数展開する */
			do_exp = !strchr(orig, '\'') && !strchr(orig, '\"');
			while ((line2 = readline("> ")) != NULL)
			{
				if (strcmp(line2, delim) == 0)
				{
					free(line2);
					break ;
				}
				if (do_exp)
				{
					ev = expand_vars(line2);
					write(pipefd[1], ev, strlen(ev));
					free(ev);
				}
				else
					write(pipefd[1], line2, strlen(line2));
				write(pipefd[1], "\n", 1);
				free(line2);
			}
			close(pipefd[1]);
			free(delim);
		}
		else if (!used && t->kind == TK_OP && strcmp(t->word, "<") == 0
			&& t->next)
		{
			fd = open(t->next->word, O_RDONLY);
			if (fd < 0)
				fatal_error("open");
			dup2(fd, STDIN_FILENO);
			close(fd);
		}
		t = t->next;
	}
	if (used)
	{
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
	}
}

/* 出力リダイレクト (>) とアペンド (>>) */
void	apply_output_redirection(t_token *t)
{
	int	fd;

	while (t && !at_eof(t))
	{
		if (t->kind == TK_OP && strcmp(t->word, ">>") == 0 && t->next)
		{
			fd = open(t->next->word, O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (fd < 0)
				fatal_error("open");
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}
		else if (t->kind == TK_OP && strcmp(t->word, ">") == 0 && t->next)
		{
			fd = open(t->next->word, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (fd < 0)
				fatal_error("open");
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}
		t = t->next;
	}
}
/* コマンド実行 */
int	run_command(char *p, char **a, t_token *t, char **envp)
{
	pid_t	pid;
	int		st;

	pid = fork();
	if (pid < 0)
		fatal_error("fork");
	if (pid == 0)
	{
		apply_input_redirection(t);
		apply_output_redirection(t);
		execve(p, a, envp);
		fatal_error("execve");
	}
	wait(&st);
	return (WEXITSTATUS(st));
}

/* ディレクトリ・実行権限チェック */
int	check_and_run(char *p, char **a, t_token *t, char **envp)
{
	struct stat	sb;

	if (stat(p, &sb) == 0 && S_ISDIR(sb.st_mode))
	{
		dprintf(2, "minishell: %s: is a directory\n", a[0]);
		return (126);
	}
	if (access(p, X_OK) != 0)
	{
		if (errno == EACCES)
		{
			dprintf(2, "minishell: %s: Permission denied\n", a[0]);
			return (126);
		}
		dprintf(2, "command not found: %s\n", a[0]);
		return (127);
	}
	return (run_command(p, a, t, envp));
}

/* 外部コマンド起動 */
int	launch_external(char **a, t_token *t, char **envp)
{
	char	*path;
	int		ret;

	path = strchr(a[0], '/') ? strdup_safe(a[0]) : search_path(a[0]);
	if (!path)
	{
		dprintf(2, "command not found: %s\n", a[0]);
		free(a);
		free_tokens(t);
		return (127);
	}
	ret = check_and_run(path, a, t, envp);
	free(path);
	free(a);
	free_tokens(t);
	return (ret);
}

/* interpret 本体 */
int	interpret(char *line, char **envp, t_context *ctx)
{
	t_token	*toks;
	int		e;
	t_node	*nd;
	char	**argv;
	int		bi;

	toks = tokenize(line, ctx);
	if (!toks)
	{
		e = ctx->syntax_error;
		ctx->syntax_error = 0;
		return (e ? 258 : 127);
	}
	toks = expand_and_merge_tokens(toks);
	nd = parse(toks);
	if (!nd || !nd->args)
		return (free_tokens(toks), 127);
	expand_token(nd->args);
	argv = build_argv(nd->args);
	if (!argv[0])
		return (free(argv), free_tokens(toks), 127);
	bi = handle_builtin(argv, toks);
	if (bi >= 0)
		return (bi);
	return (launch_external(argv, toks, envp));
}

/* メイン */
int	main(int argc, char **argv, char **envp)
{
	t_context ctx = {0};
	char *line;
	int status = 0;
	(void)argc;
	(void)argv;
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