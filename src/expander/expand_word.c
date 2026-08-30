/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:41:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:41:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "ft_glob.h"
#include "brace_expand.h"
#include "decomposer.h"
#include "sys.h"

char	*arith_expand(t_shell *state, const char *expr, int len);

/* A word participates in brace expansion only when an *unquoted* subtoken
   (TT_WORD) contains a '{'.  In "$LFS"/{a,b} the literal /{a,b} piece IS a
   TT_WORD, so it brace-expands; the "$LFS" piece is TT_ENVVAR and is carried
   along verbatim.  Braces inside quotes ("{a,b}") have no TT_WORD child at
   all, so the loop naturally skips them -- no explicit quoting check needed. */
static bool	word_has_unquoted_brace(t_ast_node *node)
{
	size_t		i;
	int			j;
	t_ast_node	*c;

	if (!node->children.ctx || node->children.len == 0)
		return (false);
	i = 0;
	while (i < node->children.len)
	{
		c = &((t_ast_node *)node->children.ctx)[i++];
		if (c->node_type != AST_TOKEN || c->token.tt != TT_WORD)
			continue ;
		j = 0;
		while (j < c->token.len)
			if (c->token.start[j++] == '{')
				return (true);
	}
	return (false);
}

static void	expand_brace_result(t_shell *state, t_vec *results, t_vec *args)
{
	char		*fs;
	t_ast_node	w;
	t_token		tmp;
	size_t		i;

	i = 0;
	while (i < results->len)
	{
		fs = ((char **)results->ctx)[i++];
		tmp.start = fs;
		tmp.len = (int)ft_strlen(fs);
		tmp.tt = TT_WORD;
		w = reparse_word(tmp, false);
		expand_word(state, &w, args, false);
		xfree(fs);
	}
}

/* Brace-expand `node` into multiple words and push each through the full
   pipeline (tilde → param → cmdsub → split → glob).  The flat source is
   stringified via word_to_brace_src (which preserves $var and quotes so
   re-lexing sees the right token types), brace-expanded into N strings,
   then each is re-parsed and passed to expand_word.  Returns true when it
   consumed the node so the caller knows NOT to run the normal pipeline.

   `set +B` (braceexpand off) short-circuits the whole pass, leaving the
   braces as literal text -- the node then goes through the ordinary word
   pipeline untouched, which is what bash does. */
bool	try_brace_expand(t_shell *state, t_ast_node *node, t_vec *args)
{
	t_string	flat;
	char		*fs;
	t_vec		results;

	if (!(state->setopt & SETOPT_BRACEEXPAND))
		return (false);
	if (!word_has_unquoted_brace(node))
		return (false);
	flat = word_to_brace_src(*node);
	fs = ft_strndup((char *)flat.ctx, flat.len);
	xfree(flat.ctx);
	if (brace_find_expandable(fs, &(int){0}) < 0)
		return (xfree(fs), false);
	results = brace_expand_str(fs);
	xfree(fs);
	expand_brace_result(state, &results, args);
	return (xfree(results.ctx), free_ast(node), true);
}

static bool	has_plain_literal_meta(char c, int *lbr, int i)
{
	if (c == '$' || c == '`' || c == '*' || c == '?' || c == '~'
		|| c == '{' || c == '\\' || c == '\'' || c == '"'
		|| c == ' ' || c == '\t' || c == '\n')
		return (true);
	if (c == '[')
		*lbr = i;
	else if (c == ']' && *lbr >= 0 && i > *lbr + 1)
		return (true);
	return (false);
}

/* A word that is a single, unquoted TT_WORD token with zero shell
   metacharacters expands to itself — no IFS split, no glob, no allocation.
   This is the most common case for command names like "ls" or "-la";
   detecting it here avoids calling the full pipeline at all.
   Note: '[' is only a metachar when matched by a later ']' (glob bracket),
   so `lbr` tracks the position of an unmatched '['.
     An extglob group is a metacharacter run that has to be spotted by
   LOOKAHEAD, not by its first byte: `@(a|b)` and `+(a)` contain none of the
   characters above, so this fast path claimed them as plain literals and
   the glob walk never ran -- `echo @(aa|ab)` printed itself. `*(` and `?(`
   escaped that only by accident, because their first byte is already a
   wildcard. */
bool	word_is_plain_literal(t_ast_node *node)
{
	t_token	*t;
	int		i;
	int		lbr;
	char	c;

	if (node->children.len != 1
		|| ((t_ast_node *)node->children.ctx)[0].node_type != AST_TOKEN)
		return (false);
	t = &((t_ast_node *)node->children.ctx)[0].token;
	if (t->tt != TT_WORD)
		return (false);
	i = 0;
	lbr = -1;
	while (i < t->len)
	{
		c = t->start[i];
		if (has_plain_literal_meta(c, &lbr, i) || extglob_ahead(t->start + i))
			return (false);
		i++;
	}
	return (true);
}
