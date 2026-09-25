/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_decl_word.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/24 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Is the word, a single unquoted TT_WORD, exactly `name`? */
static bool	word_is(t_ast_node word, const char *name)
{
	t_ast_node	c;

	if (word.children.len != 1)
		return (false);
	c = ((t_ast_node *)word.children.ctx)[0];
	return (c.token.tt == TT_WORD && (size_t)c.token.len == ft_strlen(name)
		&& ft_strncmp(c.token.start, name, c.token.len) == 0);
}

/* DECL_UTILITY when the word names a declaration utility, DECL_COMMAND
   for a bare `command` (the NEXT word then decides), else 0.
     A declaration utility's NAME=value arguments are not field-split and
   not globbed: `local new=$2` with $2 = "sudo -e" is one assignment in
   bash and zsh alike. Only `export` was recognised, so local, declare,
   typeset and readonly split theirs -- `local v=$1` kept the first word
   of $1, and the omz sudo widget's `local old=$1 new=$2 space=${2:+ }`
   turned `vim f` into `sudo/f` on ESC ESC (#137). bash applies the rule
   to the literal names, and through a bare `command`, but not through
   `builtin` or `command -p`; so does this. */
int	decl_word_kind(t_ast_node word)
{
	if (word_is(word, "export") || word_is(word, "readonly")
		|| word_is(word, "declare") || word_is(word, "typeset")
		|| word_is(word, "local"))
		return (DECL_UTILITY);
	if (word_is(word, "command"))
		return (DECL_COMMAND);
	return (0);
}
