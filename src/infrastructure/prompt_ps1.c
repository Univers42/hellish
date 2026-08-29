/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_ps1.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "version.h"
#include "env.h"

/* User-configurable prompts, bash syntax. Setting PS1 in ~/.hellishrc (or
   exporting it) replaces the built-in two-row prompt entirely; unset PS1
   keeps the rich default as the "hellish theme". Escapes supported:
   \u \h \H \w \W \$ \n \t \d \e \a \\ \j \s \v \[ \] \nnn — unknown
   escapes pass through literally, exactly like bash. $NAME expands from
   the live environment on every render (prompt_ps1b.c); ${...} goes
   through the shell's own parameter expander (prompt_ps1d.c). */

/* \w and \W: the cwd with $HOME collapsed to ~; \W keeps only the last
   component (but a bare ~ stays ~, matching bash). */
static void	ps1_cwd(t_shell *state, t_string *out, char kind)
{
	char	cwd[PATH_MAX + 1];
	char	*home;
	char	*last;
	size_t	hl;

	if (!getcwd(cwd, sizeof(cwd)))
		ft_strlcpy(cwd, "~", sizeof(cwd));
	home = env_expand(state, "HOME");
	hl = 0;
	if (home && *home)
		hl = ft_strlen(home);
	if (hl && ft_strncmp(cwd, home, hl) == 0
		&& (cwd[hl] == '\0' || cwd[hl] == '/'))
	{
		ft_memmove(cwd + 1, cwd + hl, ft_strlen(cwd + hl) + 1);
		cwd[0] = '~';
	}
	last = ft_strrchr(cwd, '/');
	if (kind == 'W' && last && last[1])
	{
		vec_push_str(out, last + 1);
		return ;
	}
	vec_push_str(out, cwd);
}

/* \t (HH:MM:SS) and \d (Tue Jul 21) via strftime, like bash. */
static void	ps1_timedate(t_string *out, char kind)
{
	time_t		now;
	struct tm	tm;
	char		buf[64];

	now = time(NULL);
	localtime_r(&now, &tm);
	if (kind == 't')
		strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
	else
		strftime(buf, sizeof(buf), "%a %b %d", &tm);
	vec_push_str(out, buf);
}

/* The zero-argument escapes: literals, readline width guards, counters. */
static bool	ps1_escape_misc(t_shell *state, t_string *out, char c)
{
	char	*jobs;

	if (c == 'n')
		return (vec_push_char(out, '\n'), true);
	if (c == 'e')
		return (vec_push_char(out, '\033'), true);
	if (c == 'a')
		return (vec_push_char(out, '\a'), true);
	if (c == '\\')
		return (vec_push_char(out, '\\'), true);
	if (c == '[')
		return (vec_push_char(out, '\001'), true);
	if (c == ']')
		return (vec_push_char(out, '\002'), true);
	if (c == 's')
		return (vec_push_str(out, "hellish"), true);
	if (c == 'v')
		return (vec_push_str(out, HELLISH_VERSION), true);
	if (c != 'j')
		return (false);
	jobs = ft_itoa(state->job_table.count);
	if (jobs)
		vec_push_str(out, jobs);
	return (xfree(jobs), true);
}

/* Full escape dispatch. \g (git branch + dirty star) and \S ("✘N" after
   a failure, empty after success) are hellish extensions: they expose
   the built-in prompt's best segments to rc-file themes, which is what
   makes a fully config-driven modern prompt possible. Unknown escapes
   emit backslash + char literally, exactly like bash. */
static void	ps1_escape(t_shell *state, t_string *out, const char *f, int *i)
{
	char	c;

	c = f[*i + 1];
	if (c >= '0' && c <= '7')
		return (ps1_octal(out, f, i));
	*i += 2;
	if (c == 'u')
		return (ps1_user(state, out));
	if (c == 'h' || c == 'H')
		return (ps1_host(out, c));
	if (c == 'w' || c == 'W')
		return (ps1_cwd(state, out, c));
	if (c == 't' || c == 'd')
		return (ps1_timedate(out, c));
	if (c == '$')
		return ((void)vec_push_char(out, "$#"[getuid() == 0]));
	if (ps1_escape_misc(state, out, c))
		return ;
	if (ps1_escape_ext(state, out, c))
		return ;
	vec_push_char(out, '\\');
	vec_push_char(out, c);
}

/* Render a PS1/PS2 format string into a prompt. Backslash escapes and
   $-expansions are live: the prompt reflects cwd, time and variables at
   every render, so `PS1='\w $(nothing) ${STATUS} > '` behaves the way a
   bash user expects (minus command substitution — deliberately not run
   from a prompt string). */
t_string	ps1_render(t_shell *state, const char *fmt)
{
	t_string	out;
	int			i;

	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	while (fmt[i])
	{
		if (fmt[i] == '\\' && fmt[i + 1])
			ps1_escape(state, &out, fmt, &i);
		else if (fmt[i] == '$' && (is_var_name_p1(fmt[i + 1])
				|| fmt[i + 1] == '{' || ps1_is_special(fmt[i + 1])))
			ps1_dollar(state, &out, fmt, &i);
		else
			vec_push_char(&out, fmt[i++]);
	}
	vec_push_char(&out, '\0');
	out.len--;
	return (out);
}
