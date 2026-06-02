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

typedef struct s_ulim
{
	char		opt;
	int			res;
	long		scale;
	const char	*label;
}	t_ulim;

/* The XSI resources ulimit can query/set. scale = unit size in bytes (1 = raw
   count: open files / processes / seconds); bash reports -c/-f in 512-byte
   blocks and the memory limits in kbytes, which we mirror. */
static const t_ulim	*ulim_table(void)
{
	static const t_ulim	t[] = {
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

/* Print one resource's current limit (soft unless `hard`), scaled to its unit. */
static void	ulimit_show(const t_ulim *u, int hard, int with_label)
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
		ft_printf("%s\n", s ? s : "0");
		free(s);
	}
}

/* ulimit -a : list every resource with its label. */
static void	ulimit_show_all(int hard)
{
	const t_ulim	*t;
	int				i;

	t = ulim_table();
	i = 0;
	while (t[i].label)
	{
		ulimit_show(&t[i], hard == 1, 1);
		i++;
	}
}

/* Set one resource's soft and/or hard limit from a textual value. */
static int	ulimit_set(t_shell *st, const t_ulim *u, char *v, int hard)
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

/* ulimit [-HS] [-acdfnstuv] [limit] : query or set process resource limits. */
int	builtin_ulimit(t_shell *state, t_vec argv)
{
	char			**av;
	const t_ulim	*t;
	char			opt;
	int				hard;
	int				i;

	av = (char **)argv.ctx;
	opt = 'f';
	hard = -1;
	i = 0;
	while (++i < (int)argv.len && av[i][0] == '-' && av[i][1])
	{
		if (av[i][1] == 'H' || av[i][1] == 'S')
			hard = (av[i][1] == 'H');
		else if (av[i][1] == 'a')
			return (ulimit_show_all(hard), 0);
		else
			opt = av[i][1];
	}
	t = ulim_table();
	while (t->label && t->opt != opt)
		t++;
	if (!t->label)
		return (ft_eprintf("%s: ulimit: invalid option\n", state->ctx), 1);
	if (i < (int)argv.len)
		return (ulimit_set(state, t, av[i], hard));
	return (ulimit_show(t, hard == 1, 0), 0);
}
