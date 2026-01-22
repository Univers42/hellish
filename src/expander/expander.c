/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:37 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:37 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include "expander.h"
#include "ft_glob.h"
#include "env.h"
#include "helpers.h"
#include <errno.h>
// Added for command substitution
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include "arith.h"

char *expand_word_single(t_shell *state, t_ast_node *curr)
{
	t_vec args;
	size_t i;
	char *temp;

	vec_init(&args);
	args.elem_size = sizeof(char *);
	expand_word(state, curr, &args, false);
	/* If expansion produced exactly one item, return it (transfer ownership) */
	if (args.len == 1)
	{
		temp = ((char **)args.ctx)[0];
		free(args.ctx);
		return (temp);
	}
	/* If expansion produced zero items, return the original word string
	   (behaviour: unmatched glob stays literal). */
	if (args.len == 0)
	{
		t_string s = word_to_string(*curr);
		/* ensure NUL-termination */
		if (!vec_ensure_space_n(&s, 1))
		{
			/* allocation failure: cleanup and return NULL to indicate error */
			free(s.ctx);
			return (NULL);
		}
		((char *)s.ctx)[s.len] = '\0';
		/* transfer ownership of s.ctx to caller */
		return ((char *)s.ctx);
	}
	/* args.len > 1: ambiguous expansion -> cleanup and return NULL */
	i = 0;
	while (i < args.len)
	{
		free(((char **)args.ctx)[i]);
		i++;
	}
	free(args.ctx);
	return (0);
}

/* Check if a command string is empty or whitespace-only */
static bool is_empty_command(const char *cmd)
{
	if (!cmd)
		return true;
	while (*cmd)
	{
		if (*cmd != ' ' && *cmd != '\t' && *cmd != '\n')
			return false;
		cmd++;
	}
	return true;
}

/* Run a shell with -c to execute `cmd` and capture its stdout. Returns malloc'd
   string with trailing newlines removed (or empty string). Returns NULL on error. */
static char *capture_subshell_output(t_shell *state, const char *cmd)
{
	int pipefd[2];
	pid_t pid;
	t_string out;
	char *ret;

	/* Handle empty command - return empty string immediately */
	if (is_empty_command(cmd))
		return ft_strdup("");

	if (pipe(pipefd) == -1)
		return ft_strdup("");
	pid = fork();
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return ft_strdup("");
	}
	if (pid == 0)
	{
		/* child: write stdout to pipe */
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		/* exec /bin/sh -c cmd with current environment */
		char *const argv[] = {"/bin/sh", "-c", (char *)cmd, NULL};
		char **envp = get_envp(state, "/bin/sh");
		execve("/bin/sh", argv, envp);
		/* if exec failed */
		if (envp)
			free_tab(envp);
		_exit(127);
	}
	/* parent */
	close(pipefd[1]);
	vec_init(&out);
	out.elem_size = 1;
	/* read entire stdout into t_string */
	vec_append_fd(pipefd[0], &out);
	close(pipefd[0]);
	{
		int status;
		while (waitpid(pid, &status, 0) == -1 && errno == EINTR)
			;
	}
	/* ensure NUL-termination */
	ret = malloc(out.len + 1);
	if (!ret)
	{
		free(out.ctx);
		return ft_strdup("");
	}
	if (out.len)
		memcpy(ret, out.ctx, out.len);
	ret[out.len] = '\0';
	/* strip trailing newlines (all) to mimic shell command-substitution */
	size_t nlen = out.len;
	while (nlen > 0 && ret[nlen - 1] == '\n')
		nlen--;
	ret[nlen] = '\0';
	free(out.ctx);
	return ret;
}

/* Expand all occurrences of $((...)) arithmetic and $(...) command substitution
   inside token strings of a node. */
static void expand_cmd_substitutions(t_shell *state, t_ast_node *node)
{
	size_t i;

	if (!node->children.ctx)
		return;
	i = 0;
	while (i < node->children.len)
	{
		t_ast_node *ch = &((t_ast_node *)node->children.ctx)[i];
		if (ch->node_type == AST_TOKEN)
		{
			t_token *tok = &ch->token;
			/* Only process TT_WORD tokens for substitution */
			if (tok->tt == TT_WORD)
			{
				const char *s = tok->start;
				int len = tok->len;
				int pos = 0;
				bool changed = false;
				t_string outbuf;
				vec_init(&outbuf);
				outbuf.elem_size = 1;
				while (pos < len)
				{
					/* Check for $(( arithmetic expansion first */
					if (s[pos] == '$' && pos + 2 < len &&
						s[pos + 1] == '(' && s[pos + 2] == '(')
					{
						/* Find matching )) */
						int depth = 2;
						int j = pos + 3;
						while (j < len && depth > 0)
						{
							if (s[j] == '(' && j + 1 < len && s[j + 1] == '(')
							{
								depth += 2;
								j += 2;
							}
							else if (s[j] == ')' && j + 1 < len && s[j + 1] == ')')
							{
								depth -= 2;
								j += 2;
							}
							else if (s[j] == '(')
							{
								depth++;
								j++;
							}
							else if (s[j] == ')')
							{
								depth--;
								j++;
							}
							else
								j++;
						}
						if (depth == 0)
						{
							/* Append prefix before $(( if this is first match */
							if (pos > 0 && outbuf.len == 0)
								vec_push_nstr(&outbuf, s, pos);
							/* Extract arithmetic expression (between (( and )) */
							int expr_start = pos + 3;
							int expr_end = j - 2;
							int expr_len = expr_end - expr_start;
							char *result = arith_expand(state, s + expr_start, expr_len);
							if (result)
							{
								vec_push_nstr(&outbuf, result, ft_strlen(result));
								free(result);
							}
							/* Update scanning */
							s = tok->start + j;
							len = tok->len - j;
							pos = 0;
							changed = true;
							continue;
						}
					}
					/* Check for $( command substitution */
					else if (s[pos] == '$' && pos + 1 < len && s[pos + 1] == '(')
					{
						int depth = 1;
						int j = pos + 2;
						while (j < len && depth > 0)
						{
							if (s[j] == '(')
								depth++;
							else if (s[j] == ')')
								depth--;
							j++;
						}
						if (depth == 0)
						{
							if (pos > 0 && outbuf.len == 0)
								vec_push_nstr(&outbuf, s, pos);
							int inner_start = pos + 2;
							int inner_end = j - 1;
							int inlen = inner_end - inner_start;
							char *inner = malloc(inlen + 1);
							if (inner)
							{
								memcpy(inner, s + inner_start, inlen);
								inner[inlen] = '\0';
								char *subout = capture_subshell_output(state, inner);
								free(inner);
								if (!subout)
									subout = ft_strdup("");
								if (*subout)
									vec_push_nstr(&outbuf, subout, ft_strlen(subout));
								free(subout);
							}
							s = tok->start + j;
							len = tok->len - j;
							pos = 0;
							changed = true;
							continue;
						}
					}
					/* Copy single character */
					char c = s[pos++];
					vec_push(&outbuf, &c);
				}
				if (changed)
				{
					char *newstr = malloc(outbuf.len + 1);
					if (newstr)
					{
						if (outbuf.len)
							memcpy(newstr, outbuf.ctx, outbuf.len);
						newstr[outbuf.len] = '\0';
						if (tok->allocated && tok->start)
							free((char *)tok->start);
						tok->start = newstr;
						tok->len = outbuf.len;
						tok->allocated = true;
					}
					free(outbuf.ctx);
				}
				else
					free(outbuf.ctx);
			}
		}
		expand_cmd_substitutions(state, &((t_ast_node *)node->children.ctx)[i]);
		i++;
	}
}

t_token_old get_old_token(t_ast_node word)
{
	t_token_old ret;

	ft_assert(word.node_type == AST_WORD);
	ft_assert(word.children.len);
	ret = ((t_ast_node *)word.children.ctx)[0].token.full_word;
	ft_assert(ret.present);
	return (ret);
}

void expand_node_glob(t_ast_node *node, t_vec *args, bool keep_as_one)
{
	t_vec glob_words;
	t_string temp;
	size_t j;

	glob_words = expand_word_glob(*node);
	if (get_g_sig()->should_unwind)
		return;
	vec_init(&temp);
	j = 0;
	while (j < glob_words.len)
	{
		if (!keep_as_one)
			vec_push(args, &((char **)glob_words.ctx)[j]);
		else
		{
			vec_push_str(&temp, ((char **)glob_words.ctx)[j]);
			free(((char **)glob_words.ctx)[j]);
		}
		if (j++ + 1 < glob_words.len && keep_as_one)
		{
			char ch = ' ';
			vec_push(&temp, &ch);
		}
	}
	if (keep_as_one)
		vec_push(args, &temp.ctx);
	free(glob_words.ctx);
	free_ast(node);
}

void expand_word(t_shell *state, t_ast_node *node,
				 t_vec *args, bool keep_as_one)
{
	t_vec_nd words;
	size_t i;

	expand_tilde_word(state, node);
	/* expand command substitutions $(...) before variable expansion
	   NOTE: expand_cmd_substitutions only processes TT_WORD tokens,
	   not TT_SQWORD or TT_DQWORD which are literal */
	expand_cmd_substitutions(state, node);
	expand_env_vars(state, node);
	vec_init(&words);
	words.elem_size = sizeof(t_ast_node);
	if (!keep_as_one)
		words = split_words(state, node);
	else
	{
		vec_push(&words, node);
		*node = (t_ast_node){};
	}
	i = 0;
	while (i < words.len)
	{
		expand_node_glob(&((t_ast_node *)words.ctx)[i], args, keep_as_one);
		if (get_g_sig()->should_unwind)
			while (i < words.len)
				free_ast(&((t_ast_node *)words.ctx)[i++]);
		if (get_g_sig()->should_unwind)
			break;
		i++;
	}
	free(words.ctx);
	free_ast(node);
	return;
}
