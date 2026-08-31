/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_flags.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 18:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 18:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* ${(flags)x} -- zsh parameter-expansion flags.
**
** 90 uses across 8 of the 12 plugins in the corpus, which makes this the
** single construct that decides whether real zsh code runs here at all.
** `${(f)$(git branch)}` is how every plugin turns command output into a
** list; there is no bash spelling of it, so without this they all stop at
** the first line.
**
** Gated on zsh_mode(): in the default dialect `${(f)x}` is a bad
** substitution, exactly as bash reports it, and the 3790 golden cases never
** reach this file.  The gate is checked once, here, so no other file in the
** expander has to know the dialect exists.
**
** THE RULE FOR ANYTHING NOT IMPLEMENTED IS THAT IT FAILS LOUDLY.  A flag we
** do not know must never fall through to the unflagged value: `${(M)x}`
** quietly answering `$x` would produce a plausible wrong string that no test
** would catch and that the plugin author could not debug.  Unknown flags are
** a bad substitution that names the flag.
**
** `array` is the single fact the rest of the pipeline keeps asking about,
** and it was worth four bugs to learn: in zsh a flag list only reshapes an
** expansion that IS an array, and double quotes join it to a scalar BEFORE
** the flags run.  So `"${(o)arr}"` does NOT sort -- it prints the elements
** in their original order -- while `${(o)arr}` and `"${(@o)arr}"` do.  Every
** one of those spellings looks like it sorts.  Nothing but a real zsh says
** otherwise, which is why tests/zsh_flags_test.py diffs against one.
*/

/* A flag argument: the character after the letter is the delimiter, and the
   argument runs to the next one.  zsh accepts any delimiter -- (s:/:) and
   (s./.) mean the same -- and pairs the bracket forms, which is why (s(x))
   also turns up in the wild.  Returns the index just past the closing
   delimiter, or -1 when it is missing. */
static int	zf_arg(const char *s, int slen, int i, char **out)
{
	char	close;
	int		start;

	if (i >= slen)
		return (-1);
	close = s[i];
	if (close == '(')
		close = ')';
	else if (close == '[')
		close = ']';
	else if (close == '{')
		close = '}';
	start = ++i;
	while (i < slen && s[i] != close)
		i++;
	if (i >= slen)
		return (-1);
	xfree(*out);
	*out = ft_strndup(s + start, i - start);
	return (i + 1);
}

/* Take one flag letter, consuming its argument when it has one.  Returns the
   next index, or -1 on a malformed argument. */
static int	zf_one(const char *s, int slen, int i, t_zflags *f)
{
	f->set[f->n++] = s[i];
	if (s[i] == 's')
		return (zf_arg(s, slen, i + 1, &f->sep));
	if (s[i] == 'j')
		return (zf_arg(s, slen, i + 1, &f->join));
	return (i + 1);
}

/* Parse the (flags) prefix.  `s` points at the '(' .  Returns the index just
   past the ')', or -1 on a malformed list -- in which case the caller
   reports a bad substitution and nothing is consumed. */
int	zf_parse(const char *s, int slen, t_zflags *f)
{
	int	i;

	i = 1;
	while (i < slen && s[i] != ')')
	{
		if (f->n >= ZF_MAX)
			return (-1);
		i = zf_one(s, slen, i, f);
		if (i < 0)
			return (-1);
	}
	if (i >= slen)
		return (-1);
	return (i + 1);
}

/* Is this expansion an ARRAY, which is what decides whether (o), (O) and (u)
   do anything at all?  Three ways to be one: the expansion is unquoted, the
   (@) flag asks for it, or the operand carries a [@] / [*] subscript --
   `"${(o)${(f)x}[@]}"` sorts for exactly that last reason, while the same
   line without the subscript does not. */
bool	zf_arrayness(t_zflags *f, t_token *tt, const char *s, int slen)
{
	return (zf_has(f, '@') || tt->tt != TT_DQENVVAR
		|| zn_at_len(s, slen) != 0);
}

/* Entry point, called from try_array_forms ahead of every bash form: only a
   body that literally begins with '(' can reach the parser, and only in zsh
   mode, so every other expansion in the shell is untouched. */
bool	expand_zsh_flags(t_shell *state, t_token *tt, bool split_ctx)
{
	t_zflags	f;
	int			end;
	char		*val;

	if (!zsh_mode(state) || tt->len < 2)
		return (false);
	if (tt->start[0] != '(')
		return (zsh_bare_nested(state, tt, split_ctx));
	f = (t_zflags){0};
	f.split = split_ctx;
	end = zf_parse(tt->start, tt->len, &f);
	if (end < 0)
		return (zf_free(&f), zf_bad(state, tt, '\0'), true);
	f.array = zf_arrayness(&f, tt, tt->start + end, tt->len - end);
	if (zf_hash_form(state, &f, tt, end))
		return (zf_free(&f), true);
	val = zf_inner(state, tt, tt->start + end, tt->len - end);
	zf_finish(state, &f, tt, val);
	zf_free(&f);
	return (true);
}
