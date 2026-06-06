/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:08:58 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:08:58 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Return true when `cmd` is NULL, empty, or contains only whitespace.
   Used before forking for $(...) so we avoid spawning a child just to
   run nothing — capture_subshell_output returns "" immediately instead. */
bool	is_empty_command(const char *cmd)
{
	if (!cmd)
		return (true);
	while (*cmd)
	{
		if (*cmd != ' ' && *cmd != '\t' && *cmd != '\n')
			return (false);
		cmd++;
	}
	return (true);
}

/* True when `word` is a single TT_WORD token whose text is exactly "export".
   Used to detect the command name so assignment words that follow it are
   expanded with EW_KEEP_AS_ONE rather than field-splitting. */
bool	is_export(t_ast_node word)
{
	t_ast_node	c;

	if (word.children.len != 1)
		return (false);
	c = ((t_ast_node *)word.children.ctx)[0];
	if (c.token.tt != TT_WORD)
		return (false);
	if (ft_strncmp(c.token.start, "export", c.token.len))
		return (false);
	return (true);
}

/* Expand and register a redirect node from a simple command's child list.
   On error, clears should_unwind (the error is handled by returning the
   AMBIGUOUS_REDIRECT sentinel, not by propagating a signal-style unwind)
   and pushes the index into the caller's redirects list on success. */
int	expand_simple_cmd_redir(t_shell *state,
		t_expander_simple_cmd *exp, t_vec_int *redirects)
{
	int			redir_idx;

	if (redirect_from_ast_redir(state, exp->curr, &redir_idx))
	{
		get_g_sig()->should_unwind = 0;
		return (AMBIGUOUS_REDIRECT);
	}
	vec_push_int(redirects, redir_idx);
	return (0);
}

/* Retrieve the original pre-expansion token from a word node.  The lexer
   stashes the full source text in token.full_word for assignment and glob
   display purposes.  Returns init_token_old() (present=false) when the
   token has no full_word annotation (e.g. synthesised nodes). */
t_token_old	get_old_token(t_ast_node word)
{
	t_token_old	ret;

	if (word.node_type != AST_WORD || word.children.len == 0
		|| !word.children.ctx)
		return (init_token_old());
	if (!((t_ast_node *)word.children.ctx)[0].token.full_word)
		return (init_token_old());
	ret = *((t_ast_node *)word.children.ctx)[0].token.full_word;
	return (ret);
}

/* Return true when token `t`'s text begins with the literal string `str`.
   Used by expand_tilde_word to match the ~, ~+, ~-, ~/ etc. prefixes. */
bool	token_starts_with(t_token t, char *str)
{
	if (t.len < (int)ft_strlen(str))
		return (false);
	return (ft_strncmp(t.start, str, ft_strlen(str)) == 0);
}
