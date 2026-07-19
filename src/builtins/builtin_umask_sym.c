/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_umask_sym.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/19 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* POSIX symbolic umask grammar (chmod-style, bash 5.3 / dash agree):
   mode   := clause (',' clause)*
   clause := who* action+          who    := 'u'|'g'|'o'|'a'
   action := op (permlist | permcopy)?
   op     := '+'|'-'|'='           permlist := ('r'|'w'|'x'|'s'|'t')*
   All work happens on the ALLOWED bits (~mask & 0777); the result is
   complemented back by the caller.  An empty clause (`u-r,,u-r`) or a
   stray character is a parse error and must leave the mask untouched,
   which is why parsing completes before anything is applied. */

/* Union of the named classes; empty means "no who given" (0), which the
   apply step widens to all classes -- both bash and dash do that for
   every operator, not just '='. */
static int	sym_who(const char **s)
{
	int	who;

	who = 0;
	while (**s == 'u' || **s == 'g' || **s == 'o' || **s == 'a')
	{
		if (**s == 'u')
			who |= 0700;
		else if (**s == 'g')
			who |= 0070;
		else if (**s == 'o')
			who |= 0007;
		else
			who |= 0777;
		(*s)++;
	}
	return (who);
}

/* permcopy (`a=u`): replicate one class of the INITIAL allowed bits into
   all three positions.  Both referees copy from the mask as it was when
   the builtin started, not the running value being built (`a=,a=u` from
   0124 yields 0111, not 0777). */
static int	sym_copy(const char **s, int initial)
{
	int	cls;

	cls = initial & 7;
	if (**s == 'u')
		cls = (initial >> 6) & 7;
	else if (**s == 'g')
		cls = (initial >> 3) & 7;
	(*s)++;
	return ((cls << 6) | (cls << 3) | cls);
}

/* Permission letters replicated across all classes (the who mask trims
   them at apply time).  's' and 't' are accepted and contribute nothing:
   the umask only carries rwx bits, and bash 5.3 treats them exactly so. */
static int	sym_perm(const char **s, int initial)
{
	int	perm;

	if (**s == 'u' || **s == 'g' || **s == 'o')
		return (sym_copy(s, initial));
	perm = 0;
	while (**s == 'r' || **s == 'w' || **s == 'x'
		|| **s == 's' || **s == 't')
	{
		if (**s == 'r')
			perm |= 0444;
		else if (**s == 'w')
			perm |= 0222;
		else if (**s == 'x')
			perm |= 0111;
		(*s)++;
	}
	return (perm);
}

static void	sym_apply(char op, int *bits, int who, int perm)
{
	if (who == 0)
		who = 0777;
	if (op == '+')
		*bits |= (perm & who);
	else if (op == '-')
		*bits &= ~(perm & who);
	else
		*bits = (*bits & ~who) | (perm & who);
}

/* Parse `s` against the initial allowed bits and return the new allowed
   bits, or -1 on any syntax error.  A clause may chain several actions
   (`u+r+w+x`, `=+=`); each op consumes its perms and applies in order. */
int	umask_sym_parse(const char *s, int initial)
{
	int		bits;
	int		who;
	char	op;

	bits = initial;
	while (1)
	{
		who = sym_who(&s);
		if (*s != '+' && *s != '-' && *s != '=')
			return (-1);
		while (*s == '+' || *s == '-' || *s == '=')
		{
			op = *s;
			s++;
			sym_apply(op, &bits, who, sym_perm(&s, initial));
		}
		if (*s == '\0')
			return (bits);
		if (*s != ',')
			return (-1);
		s++;
	}
}
