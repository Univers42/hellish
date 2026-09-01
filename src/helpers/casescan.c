/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   casescan.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/01 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "casescan.h"
#include "libft.h"

/* Keywords after which the next word is STILL in command position, so a
   `case` right after them is recognized: `$(if true; then case ...)`.
   `for` is absent on purpose (its next word is a variable name), and so
   are the closers (`fi`, `done`, `esac` end a command). */
static const char	*g_cs_openers[] = {
	"if", "then", "elif", "else", "do", "while", "until", NULL
};

void	casescan_init(t_casescan *cs, int len)
{
	cs->len = len;
	cs->ncase = 0;
	cs->cmdpos = true;
}

/* End of the alnum/underscore run starting at i — the word the keyword
   tests compare against. Command position already guarantees a word
   boundary BEFORE it; this provides the one after. */
static int	cs_word_end(const t_casescan *cs, const char *s, int i)
{
	while ((cs->len < 0 || i < cs->len) && s[i]
		&& (ft_isalnum(s[i]) || s[i] == '_'))
		i++;
	return (i);
}

static bool	cs_is(const char *s, int i, int e, const char *kw)
{
	return (ft_strlen(kw) == (size_t)(e - i)
		&& ft_strncmp(s + i, kw, e - i) == 0);
}

/* Word step. `case` pushes the current depth, `esac` pops, the opener
   keywords keep command position, anything else closes it. A lone `{`
   or `!` keeps an already-open command position (`$({ case ...; })`)
   but never opens one (`a{b` is brace-expansion material). */
static int	cs_word(t_casescan *cs, const char *s, int *i, int depth)
{
	int	e;
	int	k;

	e = cs_word_end(cs, s, *i);
	if (e == *i)
	{
		cs->cmdpos = (cs->cmdpos && (s[*i] == '{' || s[*i] == '!'));
		return ((*i)++, 0);
	}
	if (cs->cmdpos && cs_is(s, *i, e, "case") && cs->ncase < CASESCAN_MAX)
		cs->at[cs->ncase++] = depth;
	else if (cs->cmdpos && cs_is(s, *i, e, "esac") && cs->ncase > 0)
		cs->ncase--;
	else if (cs->cmdpos)
	{
		k = 0;
		while (g_cs_openers[k] && !cs_is(s, *i, e, g_cs_openers[k]))
			k++;
		cs->cmdpos = (g_cs_openers[k] != NULL);
		return (*i = e, 0);
	}
	cs->cmdpos = false;
	return (*i = e, 0);
}

int	casescan_step(t_casescan *cs, const char *s, int *i, int depth)
{
	char	c;

	c = s[*i];
	if (c == '(')
		return (cs->cmdpos = true, (*i)++, 1);
	if (c == ')')
	{
		(*i)++;
		if (cs->ncase > 0 && cs->at[cs->ncase - 1] == depth)
			return (cs->cmdpos = true, 0);
		return (cs->cmdpos = false, -1);
	}
	if (c == ';' || c == '|' || c == '&' || c == '\n')
		return (cs->cmdpos = true, (*i)++, 0);
	if (c == ' ' || c == '\t')
		return ((*i)++, 0);
	return (cs_word(cs, s, i, depth));
}
