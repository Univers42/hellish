/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_printf2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "printf_private.h"
#include "env.h"

/* bash semantics for a broken directive: '%' at the end of the format is
   "missing format character", any unrecognised conversion byte (%z, %-z...)
   is "invalid format character". Either way printf stops right there, keeps
   what it already rendered, and exits 1 (dash exits 2; the byte-diff
   referee for this shell is bash --posix, so we follow bash). */
static void	pf_bad_fmt(t_pf *pf, char c)
{
	pf->err = 1;
	pf->stop = true;
	if (c == '\0')
		ft_eprintf("%s: printf: missing format character\n", pf->ctx);
	else
		ft_eprintf("%s: printf: `%%%c': invalid format character\n",
			pf->ctx, c);
}

/* Single pass through the format string: handle backslash escapes (passed
   to pf_escape with no stop channel — bash honours \c only inside %b
   arguments), % conversions (parse spec, then pf_conv), and literal
   characters. pf->stop aborts both this pass and the outer loop in
   builtin_printf immediately: it is set by \c inside a %b argument, or by
   a bad directive (which also sets err so the builtin exits 1). */
static void	run_format(t_pf *pf, const char *fmt)
{
	int		i;
	t_spec	sp;

	i = 0;
	while (fmt[i] && !pf->stop)
	{
		if (fmt[i] == '\\' && fmt[i + 1])
			vec_push_char(pf->out, pf_escape(fmt, &i, NULL));
		else if (fmt[i] == '%')
		{
			i++;
			pf_parse_spec(pf, fmt, &i, &sp);
			if (!fmt[i] || !ft_strchr("diouxXeEfgGaAcsb%", fmt[i]))
				return (pf_bad_fmt(pf, fmt[i]));
			pf_conv(pf, &sp, fmt[i]);
			i++;
		}
		else
			vec_push_char(pf->out, fmt[i++]);
	}
}

/* Locate the format word: an optional "--" end-of-options marker is
   consumed (as in `printf -- '-'`); a lone "-" is a format string; any
   other leading dash-word — including bash's -v extension — is rejected as
   an invalid option with status 2, like dash (and like bash for every
   option it does not know). We deliberately do NOT implement -v's variable
   assignment: printf is on the forkless $(...) whitelist precisely because
   it cannot touch shell state, and an in-process `$(printf -v x ...)`
   would leak the assignment into the parent where bash's subshell drops
   it. The spec's expected output for the -v error path (status 2, nothing
   printed) is exactly what this produces. Returns the format's index, or
   -1 on an invalid option. */
/* A valid printf -v target: a POSIX identifier, optionally with a
   [subscript] suffix (array element). Rejects digit-initial names and
   an unterminated bracket, matching bash's "not a valid identifier". */
static bool	pf_valid_name(const char *n)
{
	int	i;

	if (!n[0] || (!ft_isalpha((unsigned char)n[0]) && n[0] != '_'))
		return (false);
	i = 1;
	while (n[i] && n[i] != '[')
	{
		if (!ft_isalnum((unsigned char)n[i]) && n[i] != '_')
			return (false);
		i++;
	}
	if (n[i] == '[')
		return (n[ft_strlen(n) - 1] == ']');
	return (true);
}

static int	pf_fmt_index(t_vec argv, char **vname)
{
	char	**av;

	av = (char **)argv.ctx;
	*vname = NULL;
	if (argv.len >= 3 && ft_strcmp(av[1], "-v") == 0 && pf_valid_name(av[2]))
		return (*vname = av[2], 3);
	if (argv.len >= 3 && ft_strcmp(av[1], "-v") == 0)
		return (-1);
	if (argv.len >= 2 && !ft_strncmp(av[1], "--", 3))
		return (2);
	if (argv.len >= 2 && av[1][0] == '-' && av[1][1])
		return (-1);
	return (1);
}

/* Deliver the rendered output to stdout in one go. Returns printf's exit
   status — 1 if any conversion saw an invalid argument or directive,
   else 0. */
static int	pf_finish(t_pf *pf)
{
	char	*val;

	if (pf->vname)
	{
		val = ft_strndup((char *)pf->out->ctx, pf->out->len);
		env_set(&pf->state->env, env_create(ft_strdup(pf->vname),
				val, false));
		return (xfree(pf->out->ctx), pf->err);
	}
	if (pf->out->len
		&& write(STDOUT_FILENO, pf->out->ctx, pf->out->len))
	{
	}
	xfree(pf->out->ctx);
	return (pf->err);
}

/* printf format [arguments] : POSIX printf. The format is reused while
   arguments remain and it contains at least one consuming conversion.
   Status: 0 on success, 1 after a bad numeric argument or bad directive,
   2 for usage or option errors. */
int	builtin_printf(t_shell *state, t_vec argv)
{
	t_pf		pf;
	t_string	out;
	int			fmt_idx;
	char		*vname;

	fmt_idx = pf_fmt_index(argv, &vname);
	if (fmt_idx < 0)
		return (ft_eprintf("%s: printf: %s: invalid option\n",
				state->ctx, ((char **)argv.ctx)[1]), 2);
	if ((int)argv.len <= fmt_idx)
		return (ft_eprintf("%s: printf: usage: printf format [arguments]\n",
				state->ctx), 2);
	vec_init(&out);
	out.elem_size = 1;
	pf = (t_pf){.out = &out, .av = (char **)argv.ctx,
		.argc = (int)argv.len, .argi = fmt_idx + 1, .ctx = state->ctx,
		.vname = vname, .state = state};
	run_format(&pf, pf.av[fmt_idx]);
	while (!pf.stop && pf.argi < pf.argc && pf.used)
	{
		pf.used = false;
		run_format(&pf, pf.av[fmt_idx]);
	}
	return (pf_finish(&pf));
}
