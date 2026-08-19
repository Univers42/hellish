/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   brace_expand_scan.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/19 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "brace_expand.h"

/* Regions the brace scanners must treat as one opaque character.
   Brace expansion happens on the WORD, before any substitution runs, so a
   '{' ',' or '}' inside a command substitution, an arithmetic or parameter
   expansion, or a quoted span belongs to the inner language -- not to a
   brace group out here. Reading them as brace syntax made `$(cmd "{a,b}")`
   split into two words and run the command once per alternative (issue #11),
   and cutting a group's commas through a substitution could hand the word
   reparser an unbalanced quote and crash it.

   Backticks are deliberately NOT skipped. bash's own quote handling inside
   `"` ... ` ... `" ` is idiosyncratic (the inner quotes close the outer
   ones), hellish currently reproduces its observable output there, and
   making them opaque would trade one divergence for another. Modelling that
   properly is a lexer-level job, tracked separately. */

/* Skip a $-introduced expansion: $( ... ), $(( ... )) and ${ ... } are all
   matched by counting their own opening/closing character, which handles
   nesting for free. An unterminated one returns the end of the string, so
   the caller simply finds no expandable group. */
static int	skip_dollar(const char *s, int i)
{
	char	open;
	char	close;
	int		depth;

	open = s[i + 1];
	close = '}';
	if (open == '(')
		close = ')';
	depth = 0;
	i++;
	while (s[i])
	{
		if (s[i] == open)
			depth++;
		else if (s[i] == close && --depth == 0)
			return (i + 1);
		i++;
	}
	return (i);
}

/* Skip a quoted span, from its opening quote to its mate. Inside double
   quotes a backslash escapes the next character; inside single quotes
   nothing does. An unterminated quote returns the end of the string. */
static int	skip_quoted(const char *s, int i)
{
	char	q;

	q = s[i++];
	while (s[i] && s[i] != q)
	{
		if (q == '"' && s[i] == '\\' && s[i + 1])
			i++;
		i++;
	}
	if (s[i])
		return (i + 1);
	return (i);
}

/* The scanners' cursor step: advance past whatever opaque region starts at
   s[i], or by a single character when none does. Every brace scan uses this
   instead of a bare i++ so they all agree on what is inert text. */
int	brace_next(const char *s, int i)
{
	if (s[i] == '$' && (s[i + 1] == '(' || s[i + 1] == '{'))
		return (skip_dollar(s, i));
	if (s[i] == '\'' || s[i] == '"')
		return (skip_quoted(s, i));
	if (s[i] == '\\' && s[i + 1])
		return (i + 2);
	return (i + 1);
}

/* Determine if the brace body contains a top-level comma (depth 0), which
   distinguishes comma-alternation from a bare {word} that should NOT expand.
   Inner braces are tracked so {a,{b,c}} is correctly seen as having one
   top-level comma between 'a' and the inner group. */
static bool	has_top_comma(const char *s, int open, int close)
{
	int	depth;
	int	i;

	depth = 0;
	i = open + 1;
	while (i < close)
	{
		if (s[i] == '{')
			depth++;
		else if (s[i] == '}')
			depth--;
		else if (s[i] == ',' && depth == 0)
			return (true);
		i = brace_next(s, i);
	}
	return (false);
}

/* Is there an expandable group opening at s[i] -- one holding a top-level
   comma or a valid sequence spec?  Sets *close to its matching '}'. */
bool	brace_group_opens_at(const char *s, int i, int *close)
{
	int		c;
	char	*body;
	bool	seq;

	if (s[i] != '{')
		return (false);
	c = brace_match(s, i);
	if (c <= i + 1)
		return (false);
	body = ft_substr(s, i + 1, c - i - 1);
	seq = brace_gen_sequence(body, NULL);
	xfree(body);
	if (seq || has_top_comma(s, i, c))
		return (*close = c, true);
	return (false);
}
