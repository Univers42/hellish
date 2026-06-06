/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_ulimit.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <sys/resource.h>

/* The table of resources ulimit can query/set. Each entry has:
     opt    — the single-letter flag (e.g. 'f' for -f file size)
     res    — the RLIMIT_* constant passed to getrlimit/setrlimit
     scale  — divide kernel units by this to get displayed units
     label  — the description printed by -a

   We return a pointer to the static array from a function rather than
   exposing a global so the definition stays in this translation unit. */
t_ulim	*ulim_table(void)
{
	static t_ulim	t[] = {
	{'c', RLIMIT_CORE, 512, "core file size (blocks, -c)"},
	{'d', RLIMIT_DATA, 1024, "data seg size (kbytes, -d)"},
	{'f', RLIMIT_FSIZE, 512, "file size (blocks, -f)"},
	{'n', RLIMIT_NOFILE, 1, "open files (-n)"},
	{'s', RLIMIT_STACK, 1024, "stack size (kbytes, -s)"},
	{'t', RLIMIT_CPU, 1, "cpu time (seconds, -t)"},
	{'u', RLIMIT_NPROC, 1, "max user processes (-u)"},
	{'v', RLIMIT_AS, 1024, "virtual memory (kbytes, -v)"},
	{0, 0, 0, NULL}};

	return (t);
}

/* Print one resource's current limit (soft unless `hard`), scaled. */
void	ulimit_show(const t_ulim *u, int hard, int with_label)
{
	struct rlimit	rl;
	rlim_t			val;
	char			*s;

	if (getrlimit(u->res, &rl) != 0)
		return ;
	val = rl.rlim_cur;
	if (hard)
		val = rl.rlim_max;
	if (with_label)
		ft_printf("%-32s ", u->label);
	if (val == RLIM_INFINITY)
		ft_printf("unlimited\n");
	else
	{
		s = ft_utoa((unsigned int)(val / u->scale));
		if (s)
			ft_printf("%s\n", s);
		else
			ft_printf("0\n");
		xfree(s);
	}
}

/* Set a resource limit. `hard` == 1 means set the hard limit, 0 means soft,
   -1 means both (the default when neither -H nor -S was given). We always
   call getrlimit first so we only overwrite the field(s) the user specified
   and leave the other one unchanged. */
int	ulimit_set(t_shell *st, const t_ulim *u, char *v, int hard)
{
	struct rlimit	rl;
	rlim_t			nv;

	if (getrlimit(u->res, &rl) != 0)
		return (1);
	if (!ft_strcmp(v, "unlimited"))
		nv = RLIM_INFINITY;
	else
		nv = (rlim_t)ft_atoi(v) * u->scale;
	if (hard != 1)
		rl.rlim_cur = nv;
	if (hard != 0)
		rl.rlim_max = nv;
	if (setrlimit(u->res, &rl) != 0)
		return (ft_eprintf("%s: ulimit: cannot modify limit\n", st->ctx), 1);
	return (0);
}
