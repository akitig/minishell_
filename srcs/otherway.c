/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/14 10:00:00 by akitig            #+#    #+#             */
/*   Updated: 2025/05/14 17:24:34 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <ctype.h>
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

/* 新規トークン作成 */
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

/* 末尾にトークン追加 */
void	append_tok(t_token **head, t_token *tok)
{
	t_token	*p;

	if (!*head)
		*head = tok;
	else
	{
		p = *head;
		while (p->next)
			p = p->next;
		p->next = tok;
	}
}

/* 変数代入文を一つのトークンとして扱う */
static t_token	*assign_token(char **rest, char *l, int prev_blank)
{
	char	*p;
	char	*q;
	char	quote;
	char	*w;
	t_token	*t;

	p = l;
	if ((isalpha((unsigned char)*p) || *p == '_'))
	{
		q = p + 1;
		while (*q && (isalnum((unsigned char)*q) || *q == '_'))
			q++;
		if (*q == '=')
		{
			q++;
			if (*q == '\'' || *q == '"')
			{
				quote = *q++;
				while (*q && *q != quote)
					q++;
				if (*q == quote)
					q++;
			}
			else
			{
				while (*q && *q != ' ' && *q != '\t' && *q != '\n')
					q++;
			}
			w = strndup(l, q - l);
			if (!w)
				fatal_error("strndup");
			*rest = q;
			t = new_token(w, TK_WORD);
			t->joined = prev_blank ? 0 : 1;
			return (t);
		}
	}
	return (NULL);
}

/* 空白文字判定 */
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

/* 接頭語比較 */
int	startswith(const char *s, const char *kw)
{
	return (strncmp(s, kw, strlen(kw)) == 0);
}

/* メタ文字判定 */
int	is_metacharacter(char c)
{
	return (strchr("|&);()<> \t\n'\"", c) != NULL);
}

/* 通常単語判定 */
int	is_word(const char *s)
{
	return (*s && !is_metacharacter(*s));
}

/* 演算子判定 */
int	is_operator(const char *s)
{
	static char	*ops[] = {"||", "&&", ";", ";;", "(", "|", ")", "\n", ">>",
			"<<", ">", "<"};
	int			i;

	i = 0;
	while (i < (int)(sizeof(ops) / sizeof(*ops)))
	{
		if (startswith(s, ops[i]))
			return (1);
		i++;
	}
	return (0);
}

/* 連結判定 */
int	is_joined(const char *l, int prev_blank)
{
	if (*l == '\'' || *l == '"' || is_word(l))
		return (!prev_blank);
	return (0);
}

/* 単語トークン */
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

/* クォートトークン */
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

/* 演算子トークン */
t_token	*operator_token(char **rest, char *l)
{
	static char	*ops[] = {"||", "&&", ";", ";;", "(", "|", ")", "\n", ">>",
			"<<", ">", "<"};
	int			i;
	char		*w;

	i = 0;
	while (i < (int)(sizeof(ops) / sizeof(*ops)))
	{
		if (startswith(l, ops[i]))
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

/* トークナイズ */
t_token	*tokenize(char *line, t_context *ctx)
{
	int	prev_blank;

	t_token head, *cur = &head, *n;
	prev_blank = 1;
	head.next = NULL;
	while (*line)
	{
		if (consume_blank(&line, line))
		{
			prev_blank = 1;
			continue ;
		}
		n = NULL;
		if (prev_blank)
			n = assign_token(&line, line, prev_blank);
		if (!n && (*line == '\'' || *line == '"'))
			n = quote_token(&line, line, is_joined(line, prev_blank), ctx);
		else if (!n && is_word(line))
			n = word_token(&line, line, is_joined(line, prev_blank));
		else if (!n && is_operator(line))
			n = operator_token(&line, line);
		if (!n)
			fatal_error("Unexpected token");
		cur->next = n;
		cur = n;
		prev_blank = (n->kind == TK_OP);
	}
	cur->next = new_token(NULL, TK_EOF);
	return (head.next);
}

/* 引用符除去 */
char	*remove_quotes(const char *w)
{
	size_t	i;
	size_t	j;
	size_t	len;
	char	q;
	char	*nw;

	i = 0;
	i = 0, j = 0, len = strlen(w);
	q = 0, nw = malloc(len + 1);
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

/* 変数展開 */
static char	*expand_vars(const char *s)
{
	size_t	i;
	size_t	j;
	size_t	k;
	size_t	newlen;

	i = 0, j = 0, k = 0, newlen = 0;
	char var[NAME_MAX + 1], *val, *res, *v;
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
	res = malloc(newlen + 1);
	if (!res)
		fatal_error("malloc");
	i = 0;
	j = 0;
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

/* 引用除去＋展開＋分割 */
void	expand_token(t_token *t)
{
	char	*fld;

	bool has_sq, has_dq;
	char *orig, *noq, *exp, *tmp, *save;
	t_token *cur, *n;
	while (t)
	{
		if (t->kind == TK_WORD)
		{
			has_sq = false;
			has_dq = false;
			for (char *p = t->word; *p; ++p)
				if (*p == '\'')
					has_sq = true;
				else if (*p == '"')
					has_dq = true;
			orig = t->word;
			if (has_sq || has_dq)
			{
				noq = remove_quotes(orig);
				free(orig);
				orig = noq;
			}
			if (!has_sq)
			{
				exp = expand_vars(orig);
				free(orig);
				orig = exp;
			}
			if (!has_dq && !has_sq && (strchr(orig, ' ') || strchr(orig, '\t')
					|| strchr(orig, '\n')))
			{
				tmp = strdup_safe(orig);
				fld = strtok_r(tmp, " \t\n", &save);
				if (fld)
				{
					free(t->word);
					t->word = strdup_safe(fld);
					cur = t;
					while ((fld = strtok_r(NULL, " \t\n", &save)))
					{
						n = new_token(strdup_safe(fld), TK_WORD);
						n->next = cur->next;
						cur->next = n;
						cur = n;
					}
				}
				free(tmp);
			}
			else
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
	t_token	*n;
	size_t	nl;
	char	*tmp;

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

/* トークン複製 */
t_token	*tokdup(t_token *tok)
{
	return (new_token(strdup_safe(tok->word), tok->kind));
}

/* EOF 判定 */
int	at_eof(t_token *t)
{
	return (t->kind == TK_EOF);
}

/* AST ノード作成 */
t_node	*new_node(e_node_kind k)
{
	t_node	*n;

	n = calloc(1, sizeof(*n));
	if (!n)
		fatal_error("calloc");
	n->kind = k;
	return (n);
}

/* 構文解析（リダイレクトをスキップ） */
t_node	*parse(t_token *t)
{
	t_node	*n;

	n = new_node(ND_SIMPLE_CMD);
	while (t && !at_eof(t))
	{
		if (t->kind == TK_OP && (!strcmp(t->word, ">") || !strcmp(t->word, ">>")
				|| !strcmp(t->word, "<") || !strcmp(t->word, "<<")))
		{
			t = t->next->next;
			continue ;
		}
		append_tok(&n->args, tokdup(t));
		t = t->next;
	}
	return (n);
}

/* PATH 探索 */
char	*search_path(const char *f)
{
	char		*cp;
	char		*tok;
	char		*first_fail;
	struct stat	st;
	char		full[PATH_MAX];

	cp = getenv("PATH");
	first_fail = NULL;
	if (!cp || !*cp)
		return (NULL);
	cp = strdup_safe(cp);
	tok = strtok(cp, ":");
	while (tok)
	{
		if (!strcmp(tok, ".") || !*tok)
		{
			tok = strtok(NULL, ":");
			continue ;
		}
		snprintf(full, PATH_MAX, "%s/%s", tok, f);
		if (stat(full, &st) == 0 && S_ISREG(st.st_mode))
		{
			if (access(full, X_OK) == 0)
			{
				free(cp);
				return (strdup_safe(full));
			}
			if (!first_fail)
				first_fail = strdup_safe(full);
		}
		tok = strtok(NULL, ":");
	}
	free(cp);
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
	int		i;

	i = 1;
	while (a[i])
	{
		eq = strchr(a[i], '=');
		if (eq)
		{
			*eq = '\0';
			setenv(a[i], eq + 1, 1);
		}
		else
			setenv(a[i], "", 1);
		i++;
	}
	return (0);
}

/* built-in unset */
int	builtin_unset(char **a)
{
	int	i;

	i = 1;
	while (a[i])
		unsetenv(a[i++]);
	return (0);
}

/* built-in 処理 */
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
		free_tokens(t);
		free(argv);
	}
	return (r);
}

/* argv 生成 */
char	**build_argv(t_token *t)
{
	char	**a;
	int		i;

	a = calloc(MAX_ARGS, sizeof(char *));
	i = 0;
	if (!a)
		fatal_error("calloc");
	while (t && i < MAX_ARGS - 1)
	{
		a[i++] = t->word;
		t = t->next;
	}
	a[i] = NULL;
	return (a);
}

/*--------------------------------------------*/
/* ① ヘルパー関数群（トークン前に配置）       */
/*--------------------------------------------*/
void	fatal_error(const char *msg); /* 既存 */

static void	append_char(char **dst, char c)
{
	size_t	len;

	len = strlen(*dst);
	*dst = realloc(*dst, len + 2);
	if (!*dst)
		fatal_error("realloc");
	(*dst)[len] = c;
	(*dst)[len + 1] = '\0';
}

static void	append_str(char **dst, const char *s)
{
	size_t	len;
	size_t	add;

	len = strlen(*dst);
	add = strlen(s);
	*dst = realloc(*dst, len + add + 1);
	if (!*dst)
		fatal_error("realloc");
	memcpy(*dst + len, s, add + 1);
}

static bool	is_variable(char *p)
{
	return (p[0] == '$' && (p[1] == '?' || isalnum((unsigned char)p[1])
			|| p[1] == '_'));
}
/* Expand $VAR or $? and append its value to *dst */
static void	expand_variable(char **dst, char **pos)
{
	char		*p;
	char		name[NAME_MAX + 1];
	int			len;
	const char	*value;

	p = *pos;
	len = 0;
	/* special parameter $? */
	if (p[1] == '?')
	{
		value = getenv("?");
		if (!value)
			value = "0";
		append_str(dst, value);
		*pos = p + 2;
		return ;
	}
	/* collect variable name */
	p++; /* skip '$' */
	while (*p && (isalnum((unsigned char)*p) || *p == '_') && len < NAME_MAX)
		name[len++] = *p++;
	name[len] = '\0';
	/* lookup and append */
	value = getenv(name);
	if (value)
		append_str(dst, value);
	/* advance input pointer */
	*pos = p;
}
static bool	is_command_subst(char *p)
{
	(void)p;
	return (false); /* 未実装: 必要なら `\`...` とか $(...) を判定 */
}
static void	expand_command_subst_str(char **dst, char **pp, char *orig)
{
	(void)dst;
	(void)pp;
	(void)orig;
	/* 未実装: コマンド置換 無視 */
}

static bool	is_arith_expr(char *p)
{
	(void)p;
	return (false);
	/* 未実装: $(( ... )) の判定 */
}
static void	expand_arith_str(char **dst, char **pp, char *orig)
{
	(void)dst;
	(void)pp;
	(void)orig;
	return ; /* 未実装: 算術展開 無視 */
}

/*--------------------------------------------*/
/* ② expand_heredoc_line の実装              */
/*--------------------------------------------*/
char	*expand_heredoc_line(char *line)
{
	char	*new;
	char	*p;

	new = calloc(1, 1);
	if (!new)
		fatal_error("calloc");
	p = line;
	while (*p)
	{
		if (is_variable(p))
			/* ← ここを修正 */
			expand_variable(&new, &p);
		else if (is_command_subst(p))
			expand_command_subst_str(&new, &p, p);
		else if (is_arith_expr(p))
			expand_arith_str(&new, &p, p);
		else if (*p == '\\' && (p[1] == '\\' || p[1] == '$' || p[1] == '`'))
		{
			append_char(&new, p[1]);
			p += 2;
		}
		else
			append_char(&new, *p++);
	}
	free(line);
	return (new);
}

/*------------------------------------------------------------------------------*/
/* ③ Word Splitting 用関数                                                    */
/*------------------------------------------------------------------------------*/
t_token	*word_splitting(t_token *head)
{
	t_token	*newh;
	t_token	*n;

	newh = NULL;
	char *save, *fld;
	for (t_token *t = head; t && t->kind != TK_EOF; t = t->next)
	{
		/* 未引用（展開後）なら空白で分割 */
		if (t->kind == TK_WORD && strchr(t->word, ' '))
		{
			fld = strtok_r(t->word, " \t\n", &save);
			while (fld)
			{
				n = new_token(strdup_safe(fld), TK_WORD);
				append_tok(&newh, n);
				fld = strtok_r(NULL, " \t\n", &save);
			}
		}
		else
		{
			/* それ以外はそのまま */
			n = new_token(strdup_safe(t->word), t->kind);
			append_tok(&newh, n);
		}
	}
	append_tok(&newh, new_token(NULL, TK_EOF));
	return (newh);
}

/* 入力リダイレクト */
void	apply_input_redirection(t_token *t)
{
	bool	used;
	char	*expanded;

	used = false;
	int pipefd[2], fd, do_exp;
	char *orig, *tmp, *delim, *line2;
	while (t && !at_eof(t))
	{
		if (t->kind == TK_OP && !strcmp(t->word, "<<") && t->next)
		{
			orig = t->next->word;
			if (used)
			{
				close(pipefd[0]);
				close(pipefd[1]);
			}
			if (pipe(pipefd) < 0)
				fatal_error("pipe");
			used = true;
			tmp = strdup_safe(orig);
			delim = remove_quotes(tmp);
			free(tmp);
			do_exp = !strchr(orig, '\'') && !strchr(orig, '"');
			while ((line2 = readline("> ")) != NULL)
			{
				if (strcmp(line2, delim) == 0)
				{
					free(line2);
					break ;
				}
				if (do_exp)
				{
					expanded = expand_heredoc_line(line2);
					write(pipefd[1], expanded, strlen(expanded));
					free(expanded);
				}
				else
				{
					write(pipefd[1], line2, strlen(line2));
				}
				write(pipefd[1], "\n", 1);
				free(line2);
			}
			close(pipefd[1]);
			free(delim);
		}
		else if (!used && t->kind == TK_OP && !strcmp(t->word, "<") && t->next)
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

/* 出力リダイレクト */
void	apply_output_redirection(t_token *t)
{
	int	fd;

	while (t && !at_eof(t))
	{
		if (t->kind == TK_OP && !strcmp(t->word, ">>") && t->next)
			fd = open(t->next->word, O_WRONLY | O_CREAT | O_APPEND, 0644);
		else if (t->kind == TK_OP && !strcmp(t->word, ">") && t->next)
			fd = open(t->next->word, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		else
		{
			t = t->next;
			continue ;
		}
		if (fd < 0)
			fatal_error("open");
		dup2(fd, STDOUT_FILENO);
		close(fd);
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

/* ディレクトリ・権限チェック */
int	check_and_run(char *p, char **a, t_token *t, char **envp)
{
	struct stat	sb;

	if (stat(p, &sb) == 0 && S_ISDIR(sb.st_mode))
	{
		dprintf(2, "%s: is a directory\n", a[0]);
		return (126);
	}
	if (access(p, X_OK) != 0)
	{
		if (errno == EACCES)
		{
			dprintf(2, "%s: Permission denied\n", a[0]);
			return (126);
		}
		dprintf(2, "command not found: %s\n", a[0]);
		return (127);
	}
	return (run_command(p, a, t, envp));
}

/* 外部起動 */
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

/* パイプ検出 */
int	contains_pipe(t_token *t)
{
	while (t && !at_eof(t))
	{
		if (t->kind == TK_OP && !strcmp(t->word, "|"))
			return (1);
		t = t->next;
	}
	return (0);
}

/* 単一実行 */
void	run_simple(t_token *toks, char **envp)
{
	t_token	*xt;
	t_node	*nd;
	char	**argv;

	xt = expand_and_merge_tokens(toks);
	/* ← ここで分割を入れる */
	xt = word_splitting(xt);
	nd = parse(xt);
	expand_token(nd->args);
	argv = build_argv(nd->args);
	if (!argv[0])
		exit(127);
	if (handle_builtin(argv, xt) >= 0)
		exit(0);
	launch_external(argv, xt, envp);
	exit(0);
}

/* パイプライン実行 */
int	execute_pipeline(t_token *toks, char **envp)
{
	int		cmds;
	int		i;
	int		j;
	int		idx;
	int		status;
	pid_t	*pids;

	cmds = 1;
	i = 0;
	j = 0;
	idx = 0;
	status = 0;
	t_token *p, *seg = toks, *next;
	int(*pipefd)[2];
	for (p = toks; !at_eof(p); p = p->next)
		if (p->kind == TK_OP && !strcmp(p->word, "|"))
			cmds++;
	pipefd = malloc((cmds - 1) * sizeof *pipefd);
	if ((cmds - 1) > 0 && !pipefd)
		fatal_error("malloc");
	pids = malloc(cmds * sizeof *pids);
	if (!pids)
		fatal_error("malloc");
	while (i < cmds - 1)
		if (pipe(pipefd[i++]) < 0)
			fatal_error("pipe");
	i = 0;
	for (p = toks;; p = p->next)
	{
		if ((p->kind == TK_OP && !strcmp(p->word, "|")) || at_eof(p))
		{
			next = at_eof(p) ? NULL : p->next;
			p->kind = TK_EOF;
			if ((pids[idx] = fork()) < 0)
				fatal_error("fork");
			if (pids[idx] == 0)
			{
				if (idx > 0)
					dup2(pipefd[idx - 1][0], STDIN_FILENO);
				if (idx < cmds - 1)
					dup2(pipefd[idx][1], STDOUT_FILENO);
				for (j = 0; j < cmds - 1; j++)
				{
					close(pipefd[j][0]);
					close(pipefd[j][1]);
				}
				run_simple(seg, envp);
			}
			p->kind = TK_OP;
			idx++;
			if (at_eof(p))
				break ;
			seg = next;
		}
	}
	for (i = 0; i < cmds - 1; i++)
	{
		close(pipefd[i][0]);
		close(pipefd[i][1]);
	}
	for (i = 0; i < cmds; i++)
		waitpid(pids[i], &status, 0);
	free(pipefd);
	free(pids);
	return (status);
}

/* interpret 本体 */
int	interpret(char *line, char **envp, t_context *ctx)
{
	t_token	*toks;
	t_node	*nd;
	char	**argv;

	int e, bi;
	toks = tokenize(line, ctx);
	if (!toks)
	{
		e = ctx->syntax_error;
		ctx->syntax_error = 0;
		return (e ? 258 : 127);
	}
	if (contains_pipe(toks))
		return (execute_pipeline(toks, envp));
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

/* メインループ */
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
		{
			add_history(line);
			status = interpret(line, envp, &ctx);
		}
		free(line);
		{
			char buf[12];
			snprintf(buf, sizeof buf, "%d", status);
			setenv("?", buf, 1);
		}
	}
	return (status);
}