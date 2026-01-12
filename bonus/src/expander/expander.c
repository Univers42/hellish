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
# include "expander.h"
#include "glob.h"
#include "env.h"
#include "helpers.h"
#include <errno.h>
// Added for command substitution
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

char	*expand_word_single(t_shell *state, t_ast_node *curr)
{
	t_vec	args;
	size_t		i;
	char		*temp;

	vec_init(&args);
	args.elem_size = sizeof(char *);
	expand_word(state, curr, &args, false);
	if (args.len != 1)
	{
		i = 0;
		while (i < args.len)
		{
			free(((char **)args.ctx)[i]);
			i++;
		}
		free(args.ctx);
		return (0);
	}
	temp = ((char **)args.ctx)[0];
	free(args.ctx);
	return (temp);
}

/* Run a shell with -c to execute `cmd` and capture its stdout. Returns malloc'd
   string with trailing newlines removed (or empty string). Returns NULL on error. */
static char *capture_subshell_output(t_shell *state, const char *cmd)
{
    int pipefd[2];
    pid_t pid;
    t_string out;
    char *ret;

    if (pipe(pipefd) == -1)
        return NULL;
    pid = fork();
    if (pid == -1)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }
    if (pid == 0)
    {
        /* child: write stdout to pipe */
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
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
        return NULL;
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

/* Expand all occurrences of $(...) inside token strings of a node. This is a
   simple, single-level implementation that handles nested parentheses counting. */
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
            /* only operate on word-like tokens where $(...) may appear */
            if (tok->tt == TT_WORD || tok->tt == TT_DQWORD || tok->tt == TT_DQENVVAR)
            {
                /* scan for occurrences of $( */
                const char *s = tok->start;
                int len = tok->len;
                int pos = 0;
                bool changed = false;
                t_string outbuf;
                vec_init(&outbuf);
                outbuf.elem_size = 1;
                while (pos < len)
                {
                    if (s[pos] == '$' && pos + 1 < len && s[pos + 1] == '(')
                    {
                        int depth = 1;
                        int j = pos + 2;
                        while (j < len && depth > 0)
                        {
                            if (s[j] == '(') depth++;
                            else if (s[j] == ')') depth--;
                            j++;
                        }
                        if (depth == 0)
                        {
                            /* append prefix before $( */
                            if (pos > 0)
                                vec_push_nstr(&outbuf, s, pos);
                            /* extract inner command */
                            /* j already advanced; compute inner bounds below */
                            int inner_start = pos + 2;
                            int inner_end = j - 1; /* exclusive */
                            int inlen = inner_end - inner_start;
                            char *inner = malloc(inlen + 1);
                            if (!inner)
                            {
                                free(outbuf.ctx);
                                break;
                            }
                            memcpy(inner, s + inner_start, inlen);
                            inner[inlen] = '\0';
                            char *subout = capture_subshell_output(state, inner);
                            free(inner);
                            if (!subout)
                                subout = ft_strdup("");
                            /* append substitution result */
                            if (*subout)
                                vec_push_nstr(&outbuf, subout, ft_strlen(subout));
                            free(subout);
                            pos = j;
                            /* move s pointer forward by pos bytes, but easier to copy suffix later */
                            /* mark changed */
                            changed = true;
                            /* continue scanning remaining tail */
                            s = tok->start + pos;
                            len = tok->len - pos;
                            pos = 0;
                        }
                        else
                        {
                            /* unmatched: just copy the rest and break */
                            vec_push_nstr(&outbuf, s + pos, len - pos);
                            break;
                        }
                    }
                    else
                    {
                        /* copy single character and advance */
                        char c = s[pos++];
                        vec_push(&outbuf, &c);
                    }
                }
                if (changed)
                {
                    /* append any remaining (if loop exited with modified s/len) */
                    if (pos < len && s)
                        vec_push_nstr(&outbuf, s + pos, len - pos);
                    /* replace token contents */
                    char *newstr = malloc(outbuf.len + 1);
                    if (newstr)
                    {
                        if (outbuf.len)
                            memcpy(newstr, outbuf.ctx, outbuf.len);
                        newstr[outbuf.len] = '\0';
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
        /* recurse into children */
        expand_cmd_substitutions(state, &((t_ast_node *)node->children.ctx)[i]);
        i++;
    }
}

t_token_old	get_old_token(t_ast_node word)
{
	t_token_old	ret;

	ft_assert(word.node_type == AST_WORD);
	ft_assert(word.children.len);
	ret = ((t_ast_node *)word.children.ctx)[0].token.full_word;
	ft_assert(ret.present);
	return (ret);
}

void	expand_node_glob(t_ast_node *node, t_vec *args, bool keep_as_one)
{
	t_vec	glob_words;
	t_string	temp;
	size_t		j;

	glob_words = expand_word_glob(*node);
	if (g_should_unwind)
		return ;
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
		if (j++ + 1 < glob_words.len && keep_as_one) {
			char ch = ' ';
			vec_push(&temp, &ch);
		}
	}
	if (keep_as_one)
		vec_push(args, &temp.ctx);
	free(glob_words.ctx);
	free_ast(node);
}

/* Replace occurrences of $(...) in a string by running the inner command and
   inserting its stdout (trailing newlines stripped). Returns a newly malloc'd
   string (caller must free). */
static char *perform_subst(t_shell *state, const char *s)
{
    int len = (int)ft_strlen(s);
    int pos = 0;
    t_string out;
    vec_init(&out);
    out.elem_size = 1;
    while (pos < len)
    {
        if (s[pos] == '$' && pos + 1 < len && s[pos+1] == '(')
        {
            int depth = 1;
            int j = pos + 2;
            while (j < len && depth > 0)
            {
                if (s[j] == '(') depth++;
                else if (s[j] == ')') depth--;
                j++;
            }
            if (depth == 0)
            {
                /* append prefix */
                if (pos > 0)
                    vec_push_nstr(&out, s, pos);
                int inner_start = pos + 2;
                int inner_end = j - 1;
                int inlen = inner_end - inner_start;
                char *inner = malloc(inlen + 1);
                if (!inner) { free(out.ctx); return NULL; }
                memcpy(inner, s + inner_start, inlen);
                inner[inlen] = '\0';
                char *sub = capture_subshell_output(state, inner);
                free(inner);
                if (!sub) sub = ft_strdup("");
                if (*sub)
                    vec_push_nstr(&out, sub, ft_strlen(sub));
                free(sub);
                pos = j;
                /* continue without copying the consumed part */
                /* adjust s/len by moving remaining part later */
                /* We'll continue scanning remaining characters */
                /* but need to update s pointer for copying later parts */
                s += pos;
                len -= pos;
                pos = 0;
            }
            else
            {
                /* unmatched, copy rest and break */
                vec_push_nstr(&out, s + pos, len - pos);
                break;
            }
        }
        else
        {
            char c = s[pos++];
            vec_push(&out, &c);
        }
    }
    /* build result */
    char *res = malloc(out.len + 1);
    if (!res) { free(out.ctx); return NULL; }
    if (out.len)
        memcpy(res, out.ctx, out.len);
    res[out.len] = '\0';
    free(out.ctx);
    return res;
}

void	expand_word(t_shell *state, t_ast_node *node,
    t_vec *args, bool keep_as_one)
{
    t_vec_nd    words;
    size_t      i;
    size_t      start_args = args->len;

    expand_tilde_word(state, node);
    /* expand command substitutions $(...) before variable expansion */
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
        if (g_should_unwind)
            while (i < words.len)
                free_ast(&((t_ast_node *)words.ctx)[i++]);
        if (g_should_unwind)
            break ;
        i++;
    }
    free(words.ctx);
    free_ast(node);
    /* Post-process newly added args for command substitution inside final strings */
    {
        size_t j = start_args;
        while (j < args->len)
        {
            char *orig = ((char **)args->ctx)[j];
            if (orig && ft_strstr(orig, "$("))
            {
                char *new = perform_subst(state, orig);
                free(orig);
                ((char **)args->ctx)[j] = new ? new : ft_strdup("");
            }
            j++;
        }
    }
    return ;
}
