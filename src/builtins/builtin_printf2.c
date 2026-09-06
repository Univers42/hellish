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
   characters.

   Length modifiers (l, ll, h, hh, j, z, t, L) are skipped and discarded
   between the spec and the conversion byte. bash accepts them and ignores
   them -- it renders every integer through intmax_t, so `printf %hd 70000`
   prints 70000 there rather than truncating to a short. We used to stop at
   the modifier instead and report "`%l': invalid format character", which
   broke every C-habit format string a user brought with them.

   pf->stop aborts both this pass and the outer loop in
   builtin_printf immediately: it is set by \c inside a %b argument, or by
   a bad directive (which also sets err so the builtin exits 1). */
/* One %-directive, *i on the byte after the '%'.  A '(' opens the strftime
   form %(fmt)T (printf_time.c); when that spec is malformed bash prints the
   '%' literally and resumes right after it, so the flags and the paren come
   out as text, which is what rewinding *i to pct does here. */
static void	run_directive(t_pf *pf, const char *fmt, int *i)
{
	t_spec	sp;
	int		pct;

	pct = *i;
	pf_parse_spec(pf, fmt, i, &sp);
	while (fmt[*i] && ft_strchr("lhjztL", fmt[*i]))
		(*i)++;
	if (fmt[*i] == '(')
	{
		if (!pf_conv_time(pf, &sp, fmt, i))
		{
			vec_push_char(pf->out, '%');
			*i = pct;
		}
		return ;
	}
	if (!fmt[*i] || !ft_strchr("diouxXeEfgGaAcsbq%", fmt[*i]))
		return (pf_bad_fmt(pf, fmt[*i]));
	pf_conv(pf, &sp, fmt[*i]);
	(*i)++;
}

static void	run_format(t_pf *pf, const char *fmt)
{
	int		i;

	i = 0;
	while (fmt[i] && !pf->stop)
	{
		if (fmt[i] == '\\' && fmt[i + 1])
			vec_push_char(pf->out, pf_escape(fmt, &i, NULL));
		else if (fmt[i] == '%')
		{
			i++;
			run_directive(pf, fmt, &i);
		}
		else
			vec_push_char(pf->out, fmt[i++]);
	}
}

/* Deliver the rendered output to stdout in one go. Returns printf's exit
   status — 1 if any conversion saw an invalid argument or directive,
   else 0. The -v target goes through subscript_assign so
   `printf -v "a[0]" ...` lands in the array element like bash, not in a
   scalar literally NAMED a[0] — bash-completion's word assembly does
   `printf -v "$2[$j]"` on every TAB (issue #105, wave 2). */
static int	pf_finish(t_pf *pf)
{
	char	*val;
	t_env	ev;

	if (pf->vname)
	{
		val = ft_strndup((char *)pf->out->ctx, pf->out->len);
		ev = env_create(ft_strdup(pf->vname), val, false);
		subscript_assign(pf->state, &ev);
		env_set(&pf->state->env, ev);
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
