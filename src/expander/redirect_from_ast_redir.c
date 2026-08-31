/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_redirect.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:14 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:14 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include <sys/stat.h>

int	get_default_src_fd(t_tt tt);

/* Implement the `set -C` / noclobber guard for `>` redirections.  When active,
   attempting to truncate an existing regular file with `>` is an error; the
   user must use `>|` (TT_CLOBBER) to override.  `&>` truncates too, so it is
   guarded alike; append (`>>`, `&>>`) and read-write (`<>`) are allowed. */
static int	noclobber_blocks(t_shell *state, t_tt tt, const char *fname)
{
	struct stat	st;

	if ((tt != TT_REDIRECT_RIGHT && tt != TT_AMP_REDIR)
		|| !state->opt_noclobber || !fname)
		return (0);
	if (stat(fname, &st) == 0 && S_ISREG(st.st_mode))
		return (1);
	return (0);
}

/* Extract the numeric fd prefix from an operator token (e.g. "2>" → fd 2).
   The default fd depends on direction: < → 0 (stdin), > → 1 (stdout).
   The token's start pointer already includes the digits before the operator
   symbol, so we just scan them off and compute the decimal value. */
int	parse_src_fd(t_tt tt, t_token op_tok)
{
	int		src_fd;
	char	*p;

	src_fd = get_default_src_fd(tt);
	p = op_tok.start;
	if (p && ft_isdigit((unsigned char)*p))
	{
		src_fd = 0;
		while (ft_isdigit((unsigned char)*p))
		{
			src_fd = src_fd * 10 + (*p - '0');
			p++;
		}
	}
	return (src_fd);
}

/* Expand the redirect target (child index 1) to a single string.  Process
   substitutions (<(...)) are handled specially — they return a /dev/fd/N
   path rather than a filename.  Plain word targets use expand_word_single_ro
   which reports ambiguous redirections (field-splitting result != 1 field). */
static char	*expand_redir_fname(t_shell *state, t_ast_node *curr)
{
	t_ast_node	*target;

	if (curr->children.len < 2)
		return (NULL);
	target = &((t_ast_node *)curr->children.ctx)[1];
	if (target->node_type == AST_PROC_SUB)
		return (expand_proc_sub(state, target));
	if (target->node_type == AST_WORD)
		return (expand_word_single_ro(state, target));
	return (NULL);
}

static t_token_old	get_target_token(t_ast_node *curr)
{
	t_ast_node	*target;

	target = &((t_ast_node *)curr->children.ctx)[1];
	if (target->node_type == AST_WORD)
		return (get_old_token(*target));
	return (init_token_old());
}

/* Expand and open one redirection, storing the t_redir in state->redirects.
   Noclobber check happens before the open() call.  On success the redir_idx
   is recorded in curr->redir_idx so the executor can retrieve the fd later.
   On failure (ambiguous or file error) print a message and return -1. */
int	try_create_redir(t_shell *state, t_ast_node *curr,
			t_tt tt, int src_fd)
{
	t_redir		new_redir;
	t_token_old	full_token;
	char		*fname;

	if (tt == TT_HERESTRING)
		return (herestring_redir(state, curr, src_fd));
	full_token = get_target_token(curr);
	fname = expand_redir_fname(state, curr);
	if (noclobber_blocks(state, tt, fname))
	{
		ft_eprintf("%s: %s: cannot overwrite existing file\n",
			state->ctx, fname);
		return (xfree(fname), -1);
	}
	if (!create_redir_4(tt, fname, &new_redir, src_fd))
	{
		print_redir_err(state, full_token, fname);
		return (xfree(fname), -1);
	}
	curr->redir_idx = state->redirects.len;
	curr->has_redirect = true;
	vec_push(&state->redirects, &new_redir);
	return (0);
}
