#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
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
static char	*expand_vars(const char *s)
{
	size_t	i;
	size_t	j;
	size_t	k;
	size_t	newlen;

	i = 0;
	j = 0;
	k = 0;
	newlen = 0;
	char var[NAME_MAX + 1], *val, *res, *v;
	/* 1) 必要長を計算 */
	while (s[i])
	{
		if (s[i] == '$' && s[i + 1] == '?')
		{
			v = getenv("?");
			if (!v)
				v = "0";
			newlen += strlen(v);
			i += 2;
		}
		else if (s[i] == '$' && (isalnum((unsigned char)s[i + 1]) || s[i
				+ 1] == '_'))
		{
			i++;
			k = 0;
			while (s[i] && (isalnum((unsigned char)s[i]) || s[i] == '_')
				&& k < NAME_MAX)
				var[k++] = s[i++];
			var[k] = '\0';
			if ((val = getenv(var)))
				newlen += strlen(val);
		}
		else
			newlen++, i++;
	}
	/* 2) バッファ確保 */
	res = malloc(newlen + 1);
	if (!res)
		fatal_error("malloc");
	/* 3) 再構築 */
	i = 0;
	while (s[i])
	{
		if (s[i] == '$' && s[i + 1] == '?')
		{
			v = getenv("?");
			if (!v)
				v = "0";
			strcpy(res + j, v);
			j += strlen(v);
			i += 2;
		}
		else if (s[i] == '$' && (isalnum((unsigned char)s[i + 1]) || s[i
				+ 1] == '_'))
		{
			i++;
			k = 0;
			while (s[i] && (isalnum((unsigned char)s[i]) || s[i] == '_')
				&& k < NAME_MAX)
				var[k++] = s[i++];
			var[k] = '\0';
			if ((val = getenv(var)))
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
/* トークンのクォート除去＋変数展開 */
void	expand_token(t_token *t)
{
	bool	has_sq;
	bool	has_dq;
	char	*orig;
	char	*noq;
	char	*exp;
	char	*p;

	while (t)
	{
		if (t->kind == TK_WORD)
		{
			/* 元の文字列を調べる */
			has_sq = false;
			has_dq = false;
			for (p = t->word; *p; ++p)
				if (*p == '\'')
					has_sq = true;
				else if (*p == '"')
					has_dq = true;
			orig = t->word;
			/* 引用符を取り除く */
			if (has_sq || has_dq)
			{
				noq = remove_quotes(orig);
				free(orig);
				orig = noq;
			}
			/* シングルクォート内以外で変数展開 */
			if (!has_sq)
			{
				exp = expand_vars(orig);
				free(orig);
				orig = exp;
			}
			t->word = orig;
		}
		t = t->next;
	}
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

// --- 単純コマンドの AST を作る（リダイレクトと空文字列トークンは除外） ---
t_node	*parse(t_token *t)
{
	t_node	*n;

	n = new_node(ND_SIMPLE_CMD);
	while (t && !at_eof(t))
	{
		// リダイレクト演算子なら「演算子＋ファイル名」を飛ばす
		if (t->kind == TK_OP && (strcmp(t->word, ">") == 0 || strcmp(t->word,
					">>") == 0 || strcmp(t->word, "<") == 0 || strcmp(t->word,
					"<<") == 0))
		{
			t = t->next ? t->next->next : NULL;
			continue ;
		}
		// 空文字列トークン（例: 未定義変数の展開）を飛ばす
		if (t->kind == TK_WORD && t->word[0] == '\0')
		{
			t = t->next;
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
/* シェル識別用ヘルパー */
static bool	is_valid_ident(const char *s)
{
	if (!(isalpha((unsigned char)*s) || *s == '_'))
		return (false);
	for (s++; *s; s++)
		if (!(isalnum((unsigned char)*s) || *s == '_'))
			return (false);
	return (true);
}

/* built-in export */
int	builtin_export(char **argv)
{
	int		rc;
	char	*arg;
	char	*eq;
	size_t	namelen;
	char	name[NAME_MAX + 1];

	rc = 0;
	/* 引数なし: 全 export 変数を出力 */
	if (!argv[1])
	{
		/* 実際はソートして出力するのが望ましいですが、順序は問わないテストなので省略可 */
		for (int i = 0; environ[i]; i++)
			printf("declare -x %s\n", environ[i]);
		return (0);
	}
	for (int i = 1; argv[i]; i++)
	{
		arg = argv[i];
		eq = strchr(arg, '=');
		/* 識別子部分を切り出して検証 */
		namelen = eq ? (size_t)(eq - arg) : strlen(arg);
		if (namelen > NAME_MAX)
			namelen = NAME_MAX;
		memcpy(name, arg, namelen);
		name[namelen] = '\0';
		if (!is_valid_ident(name))
		{
			fprintf(stderr, "export: `%s': not a valid identifier\n", arg);
			rc = 1;
			continue ;
		}
		if (eq)
		{
			/* name=val の場合は必ず上書き */
			setenv(name, eq + 1, 1);
		}
		else
		{
			/* name のみなら、存在しなければ空文字で作成、存在すれば値はそのまま */
			if (!getenv(name))
				setenv(name, "", 1);
		}
	}
	return (rc);
}

/* built-in unset */
int	builtin_unset(char **argv)
{
	int		rc;
	char	*name;

	rc = 0;
	/* 引数なし: 何もしない */
	if (!argv[1])
		return (0);
	for (int i = 1; argv[i]; i++)
	{
		name = argv[i];
		if (!is_valid_ident(name))
		{
			fprintf(stderr, "unset: `%s': not a valid identifier\n", name);
			rc = 1;
			continue ;
		}
		unsetenv(name);
	}
	return (rc);
}
/* exit ビルトイン本体 */
static int	builtin_exit(char **argv)
{
	char		*endptr;
	long long	val;

	/* 引数なし: 常にステータス 0 で終了 */
	if (!argv[1])
		exit(0);
	errno = 0;
	val = strtoll(argv[1], &endptr, 10);
	/* 数字で始まっていない or 範囲外 */
	if (endptr == argv[1] || *endptr != '\0' || errno == ERANGE)
	{
		fprintf(stderr, "exit: %s: numeric argument required\n", argv[1]);
		exit(2);
	}
	/* 引数が多すぎる */
	if (argv[2])
	{
		fprintf(stderr, "exit: too many arguments\n");
		return (1);
	}
	/* 正常終了：0–255 に丸めて exit */
	exit((unsigned char)val);
}
/* handle_builtin の該当部分 */
static int	builtin_env(char **argv)
{
	if (argv[1])
	{
		fprintf(stderr, "env: too many arguments\n");
		return (1);
	}
	for (int i = 0; environ[i]; i++)
		puts(environ[i]);
	return (0);
}
/* built-in cd */
static int	builtin_cd(char **argv)
{
	char	cwd[PATH_MAX];
	char	*target;

	/* 更新前のカレントを OLDPWD に */
	if (getcwd(cwd, sizeof(cwd)))
		setenv("OLDPWD", cwd, 1);
	/* 引数処理 */
	if (!argv[1])
	{
		target = getenv("HOME");
		if (!target)
			return (fprintf(stderr, "cd: HOME not set\n"), 1);
	}
	else if (argv[2])
		return (fprintf(stderr, "cd: too many arguments\n"), 1);
	else
		target = argv[1];
	/* ディレクトリ移動 */
	if (chdir(target) != 0)
		return (fprintf(stderr, "cd: %s: %s\n", target, strerror(errno)), 1);
	/* 移動後のカレントを PWD に */
	if (getcwd(cwd, sizeof(cwd)))
		setenv("PWD", cwd, 1);
	return (0);
}
/* echo ビルトイン用ヘルパー */
static bool	is_n_flag(const char *s)
{
	if (!s || s[0] != '-' || s[1] == '\0')
		return (false);
	for (int i = 1; s[i]; i++)
		if (s[i] != 'n')
			return (false);
	return (true);
}

/* echo ビルトイン */
static int	builtin_echo(char **argv)
{
	int		i;
	bool	newline;

	i = 1;
	newline = true;
	/* 先頭の -n, -nn, -nnnn… をフラグとして扱う */
	while (argv[i] && is_n_flag(argv[i]))
	{
		newline = false;
		i++;
	}
	/* 残りの引数をスペース区切りで出力 */
	for (int j = i; argv[j]; j++)
	{
		fputs(argv[j], stdout);
		if (argv[j + 1])
			fputc(' ', stdout);
	}
	/* 改行フラグが立っていれば改行 */
	if (newline)
		fputc('\n', stdout);
	return (0);
}
/* pwd ビルトイン */
static int	builtin_pwd(char **argv)
{
	char	cwd[PATH_MAX];

	(void)argv; // 引数は無視
	if (!getcwd(cwd, sizeof(cwd)))
	{
		perror("pwd");
		return (1);
	}
	printf("%s\n", cwd);
	return (0);
}
int	handle_builtin(char **argv, t_token *toks)
{
	int	r;

	r = -1;
	if (strcmp(argv[0], "export") == 0)
		r = builtin_export(argv);
	else if (strcmp(argv[0], "unset") == 0)
		r = builtin_unset(argv);
	else if (strcmp(argv[0], "env") == 0)
		r = builtin_env(argv);
	else if (strcmp(argv[0], "cd") == 0)
		r = builtin_cd(argv);
	else if (strcmp(argv[0], "pwd") == 0)
		r = builtin_pwd(argv);
	else if (strcmp(argv[0], "echo") == 0)
		r = builtin_echo(argv);
	else if (strcmp(argv[0], "exit") == 0)
		return (builtin_exit(argv));
	if (r >= 0)
	{
		free_tokens(toks);
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

// --- 標準入力リダイレクト (<) ---
void	apply_input_redirection(t_token *t)
{
	int		fd;
	char	*fname;

	while (t && !at_eof(t))
	{
		if (t->kind == TK_OP && strcmp(t->word, "<") == 0 && t->next)
		{
			// 次のトークンをファイル名とみなして open
			fname = remove_quotes(t->next->word);
			fd = open(fname, O_RDONLY);
			free(fname);
			if (fd < 0)
			{
				perror(t->next->word);
				exit(1);
			}
			dup2(fd, STDIN_FILENO);
			close(fd);
			// 演算子とファイル名トークンをスキップ
			t = t->next;
		}
		t = t->next;
	}
}

// --- 標準出力リダイレクト (>) とアペンド (>>) ---
void	apply_output_redirection(t_token *t)
{
	int		fd;
	char	*fname;

	while (t && !at_eof(t))
	{
		if (t->kind == TK_OP && strcmp(t->word, ">") == 0 && t->next)
		{
			fname = remove_quotes(t->next->word);
			fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			free(fname);
			if (fd < 0)
			{
				perror(t->next->word);
				exit(1);
			}
			dup2(fd, STDOUT_FILENO);
			close(fd);
			t = t->next; // 演算子とファイル名をスキップ
		}
		else if (t->kind == TK_OP && strcmp(t->word, ">>") == 0 && t->next)
		{
			fname = remove_quotes(t->next->word);
			fd = open(fname, O_WRONLY | O_CREAT | O_APPEND, 0644);
			free(fname);
			if (fd < 0)
			{
				perror(t->next->word);
				exit(1);
			}
			dup2(fd, STDOUT_FILENO);
			close(fd);
			t = t->next; // 演算子とファイル名をスキップ
		}
		t = t->next;
	}
}

/* 1. Ctrl-C 押下時にプロンプトを再表示するハンドラ */
static void	sigint_handler(int sig)
{
	(void)sig;
	write(STDOUT_FILENO, "\n", 1);
	rl_on_new_line();
	rl_replace_line("", 0);
	rl_redisplay();
}
/* コマンド実行 */
int	run_command(char *path, char **argv, t_token *toks, char **envp)
{
	pid_t	pid;
	int		status;

	pid = fork();
	if (pid < 0)
		fatal_error("fork");
	if (pid == 0)
	{
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		apply_input_redirection(toks);
		apply_output_redirection(toks);
		execve(path, argv, envp);
		fatal_error("execve");
	}
	wait(&status);
	return (WEXITSTATUS(status));
}
/* ディレクトリ・実行権限チェック */
int	check_and_run(char *p, char **a, t_token *t, char **envp)
{
	struct stat	sb;

	if (stat(p, &sb) == 0 && S_ISDIR(sb.st_mode))
	{
		dprintf(2, "minishell: %s: Is a directory\n", a[0]);
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
/* パイプ存在チェック */
int	contains_pipe(t_token *t)
{
	while (t && !at_eof(t))
	{
		if (t->kind == TK_OP && strcmp(t->word, "|") == 0)
			return (1);
		t = t->next;
	}
	return (0);
}
// /* フィルタリング: 空文字列トークンを取り除く */
// static char	**filter_argv(char **argv)
// {
// 	int		len;
// 	char	**p;

// 	len = 0;
// 	int i, j;
// 	while (argv[len])
// 		len++;
// 	p = calloc(len + 1, sizeof(char *));
// 	if (!p)
// 		fatal_error("calloc");
// 	j = 0;
// 	for (i = 0; i < len; i++)
// 	{
// 		if (argv[i] && argv[i][0] != '\0')
// 			p[j++] = argv[i];
// 	}
// 	p[j] = NULL;
// 	free(argv);
// 	return (p);
// }

// パイプ内部／子プロセス用：単一コマンド実行
void	run_simple(t_token *toks, char **envp)
{
	int		ret;
	t_token	*xt;
	t_node	*nd;
	char	**argv;

	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	xt = expand_and_merge_tokens(toks);
	// 出力→入力 の順でリダイレクトを適用
	apply_output_redirection(xt);
	apply_input_redirection(xt);
	nd = parse(xt);
	expand_token(nd->args);
	argv = build_argv(nd->args);
	if (!argv[0] || argv[0][0] == '\0')
		exit(0);
	if (handle_builtin(argv, xt) >= 0)
		exit(0);
	ret = launch_external(argv, xt, envp);
	exit(ret);
}

/* パイプライン実行 */
int	execute_pipeline(t_token *toks, char **envp)
{
	int		cmds;
	t_token	*p;
	pid_t	*pids;
	t_token	*seg;
	t_token	*next;
	int		old;

	cmds = 1;
	int(*pipefd)[2];
	int i, j, idx = 0, status;
	seg = toks;
	/* コマンド数をカウント */
	for (p = toks; !at_eof(p); p = p->next)
		if (p->kind == TK_OP && strcmp(p->word, "|") == 0)
			cmds++;
	/* 動的配列確保 */
	pipefd = malloc((cmds - 1) * sizeof *pipefd);
	if ((cmds - 1) > 0 && !pipefd)
		fatal_error("malloc");
	pids = malloc(cmds * sizeof *pids);
	if (!pids)
		fatal_error("malloc");
	/* パイプ作成 */
	for (i = 0; i < cmds - 1; i++)
		if (pipe(pipefd[i]) < 0)
			fatal_error("pipe");
	/* 各セグメントを fork で実行 */
	for (p = toks;; p = p->next)
	{
		if ((p->kind == TK_OP && strcmp(p->word, "|") == 0) || at_eof(p))
		{
			next = at_eof(p) ? NULL : p->next;
			/* この演算子をセグメント終端に */
			old = p->kind;
			p->kind = TK_EOF;
			/* fork */
			if ((pids[idx] = fork()) < 0)
				fatal_error("fork");
			if (pids[idx] == 0)
			{
				if (idx > 0)
					dup2(pipefd[idx - 1][0], STDIN_FILENO);
				if (idx < cmds - 1)
					dup2(pipefd[idx][1], STDOUT_FILENO);
				/* 全パイプを閉じる */
				for (j = 0; j < cmds - 1; j++)
					close(pipefd[j][0]), close(pipefd[j][1]);
				run_simple(seg, envp);
			}
			/* 演算子を元に戻す */
			p->kind = old;
			idx++;
			if (at_eof(p))
				break ;
			seg = next;
		}
	}
	/* 親はパイプを閉じて子を待つ */
	for (i = 0; i < cmds - 1; i++)
		close(pipefd[i][0]), close(pipefd[i][1]);
	status = 0;
	for (i = 0; i < cmds; i++)
		waitpid(pids[i], &status, 0);
	free(pipefd);
	free(pids);
	return (WEXITSTATUS(status));
}
// interpret 本体：パイプ／リダイレクト兼用
int	interpret(char *line, char **envp, t_context *ctx)
{
	t_node	*nd;
	char	**argv;

	t_token *toks, *xt;
	int saved_in, saved_out, status, e;
	toks = tokenize(line, ctx);
	if (!toks)
	{
		e = ctx->syntax_error;
		ctx->syntax_error = 0;
		return (e ? 258 : 127);
	}
	if (toks->kind == TK_EOF)
	{
		free_tokens(toks);
		return (0);
	}
	if (contains_pipe(toks))
		return (execute_pipeline(toks, envp));
	saved_in = dup(STDIN_FILENO);
	saved_out = dup(STDOUT_FILENO);
	xt = expand_and_merge_tokens(toks);
	// 出力→入力 の順でリダイレクトを適用
	apply_output_redirection(xt);
	apply_input_redirection(xt);
	nd = parse(xt);
	if (!nd || !nd->args)
	{
		free_tokens(xt);
		dup2(saved_in, STDIN_FILENO);
		dup2(saved_out, STDOUT_FILENO);
		close(saved_in);
		close(saved_out);
		return (127);
	}
	expand_token(nd->args);
	argv = build_argv(nd->args);
	if (!argv[0] || argv[0][0] == '\0')
	{
		free(argv);
		free_tokens(xt);
		dup2(saved_in, STDIN_FILENO);
		dup2(saved_out, STDOUT_FILENO);
		close(saved_in);
		close(saved_out);
		return (0);
	}
	if (!strcmp(argv[0], "export") || !strcmp(argv[0], "unset")
		|| !strcmp(argv[0], "env") || !strcmp(argv[0], "cd") || !strcmp(argv[0],
			"pwd") || !strcmp(argv[0], "echo") || !strcmp(argv[0], "exit"))
	{
		status = handle_builtin(argv, xt);
	}
	else
	{
		status = launch_external(argv, xt, envp);
	}
	dup2(saved_in, STDIN_FILENO);
	dup2(saved_out, STDOUT_FILENO);
	close(saved_in);
	close(saved_out);
	return (status);
}

int	main(int argc, char **argv, char **envp)
{
	t_context ctx = {0};
	char *line;
	int status;

	(void)argc;
	(void)argv;
	rl_catch_signals = 0;
	signal(SIGINT, sigint_handler);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_DFL);
	rl_outstream = stderr;

	while ((line = readline("minishell > ")))
	{
		if (*line)
			add_history(line);
		status = interpret(line, envp, &ctx);
		free(line);
		{
			char buf[16];
			snprintf(buf, sizeof buf, "%d", status);
			setenv("?", buf, 1);
		}
	}
	return (status);
}