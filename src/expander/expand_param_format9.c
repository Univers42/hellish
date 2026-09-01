/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_format9.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 00:05:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/02 00:05:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
/* Park an array-encoded value (scalar → one field, NULL → none) behind
   an ARR_MAGIC marker so the splitter emits verbatim fields. */
static bool	pf_mark_value(t_shell *state, t_token *tt, char *val)
{
	char	*enc;

	if (val && arr_is(val))
		enc = ft_strdup(val);
	else if (val)
		enc = arr_from_elems((char **)&val, 1, NULL);
	else
		enc = arr_from_elems(NULL, 0, NULL);
	tt->start = arr_mark_push(state, enc, (int)ft_strlen(enc));
	tt->len = (int)ft_strlen(tt->start);
	tt->allocated = false;
	return (xfree(enc), true);
}

/* ${p+"$@"} / ${p+"${name[@]}"}: when the operator SELECTS its word and
   the word is exactly a quoted all-elements expansion, bash emits one
   field per element, empties kept -- `${words+"${words[@]}"}` is the
   set-guarded pass-through bash-completion sends _comp_upvars' argv
   through, and a joined single field merged the last two words of every
   TAB (issue #105, wave 2). The elements are parked as an encoded value
   behind an ARR_MAGIC marker: verbatim for the splitter, regardless of
   the OUTER quoting, which is exactly how bash treats the inner quotes.
   Returns false for any other word, leaving the scalar path in charge. */
bool	pf_op_word_at_fields(t_shell *state, t_token *tt, const char *w,
			int n)
{
	char	*cnt;

	if (n < 4 || w[0] != '"' || w[n - 1] != '"')
		return (false);
	if (n == 4 && w[1] == '$' && w[2] == '@')
	{
		cnt = env_expand(state, "#");
		if (!cnt)
			return (false);
		return (pos_mark_fields(state, tt, 1, ft_atoi(cnt) + 1));
	}
	if (n < 9 || w[1] != '$' || w[2] != '{' || w[n - 2] != '}'
		|| ft_strncmp(w + n - 5, "[@]", 3) != 0
		|| !pf_is_indirect(w + 3, n - 8))
		return (false);
	return (pf_mark_value(state, tt,
			env_expand_n(state, (char *)w + 3, n - 8)));
}

/* The aggregate spelling ${a[@]+word} / ${a[@]-word}: when the word is
   the quoted all-elements form and the operator selects it, fields win
   over the joined scratch-var path — the empty-elements variant of the
   same #105 idiom (`${w[@]+"${w[@]}"}` must keep a leading "" field).
   `elem` is the aggregate's joined value (NULL = unset/empty), used only
   to decide whether the word is selected; ownership stays the caller's. */
bool	at_op_word_fields(t_shell *state, t_token *tt, char *elem, int off)
{
	const char	*op;
	int			oplen;
	int			i;
	bool		used;

	op = tt->start + off;
	oplen = tt->len - off;
	i = 0;
	if (i < oplen && op[i] == ':')
		i++;
	if (i >= oplen || (op[i] != '+' && op[i] != '-'))
		return (false);
	used = (op[i] == '+') == (elem != NULL);
	if (!used)
		return (false);
	return (pf_op_word_at_fields(state, tt, op + i + 1, oplen - i - 1));
}
