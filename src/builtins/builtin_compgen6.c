/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_compgen6.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 00:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/02 00:40:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "ft_glob.h"

bool	case_match(const char *s, const char *p);

/* compgen -X FILTERPAT: candidates the pattern matches are dropped; a
   leading `!` keeps ONLY the matches. Matched with the engine case/esac
   uses, extglob armed for the span: bash-completion writes its filters
   in extglob (`-X '!*.@(pdf|ps)'`) whether or not the option is on,
   the same rule [[ ]] operands follow (db_pattern_match). */
static bool	cg_keep(t_cgopt *o, const char *cand)
{
	const char	*pat;
	bool		neg;
	bool		m;
	int			saved;

	if (!o || !o->xfilter)
		return (true);
	pat = o->xfilter;
	neg = (pat[0] == '!');
	if (neg)
		pat++;
	saved = *glob_extglob_cell();
	*glob_extglob_cell() = 1;
	m = case_match(cand, pat);
	*glob_extglob_cell() = saved;
	return (m == neg);
}

/* The single exit for every compgen candidate: the -X filter decides
   whether it appears at all, then -P/-S wrap it. Returns 1 when a line
   was printed so the callers' "did anything match" accounting counts
   what the USER saw, not what the generator produced. */
int	cg_print(t_cgopt *o, const char *cand)
{
	const char	*p;
	const char	*s;

	if (!cg_keep(o, cand))
		return (0);
	p = "";
	s = "";
	if (o && o->prefix)
		p = o->prefix;
	if (o && o->suffix)
		s = o->suffix;
	ft_printf("%s%s%s\n", p, cand, s);
	return (1);
}

/* Slice variant of cg_emit for arr_next's (ptr,len) elements. */
int	cg_emit_n(t_cgopt *o, const char *s, int n, const char *pfx)
{
	char	*dup;
	int		hit;

	dup = ft_strndup((char *)s, n);
	hit = cg_emit(o, dup, pfx);
	return (xfree(dup), hit);
}
