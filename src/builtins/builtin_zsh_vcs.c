/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_vcs.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "prompt.h"

/* vcs_info and the zstyle keys that drive it -- issues #91 and #112.
**
** The reporter of #112 configured vcs_info the way its manual says to:
**
**     zstyle ':vcs_info:git:*' enable git
**     zstyle ':vcs_info:git:*' check-for-changes true
**     zstyle ':vcs_info:git:*' stagedstr '%F{green}+%f'
**     zstyle ':vcs_info:git:*' unstagedstr '%F{red}*%f'
**     zstyle ':vcs_info:git:*' formats '%F{242}on%f %F{magenta} %b%c%u%f '
**
** Every key but `formats` was answered with "zstyle: not supported (needs
** the zsh completion system)" at each shell start, and the format lost its
** colours because an unknown %x was consumed EMPTY: `%F{242}` came out as
** `{242}`. Pinned against zsh 5.9 now (tests/vcs_info_zstyle_test.py):
**
**     %b branch  %s "git"  %r repo basename  %R repo root  %% a percent
**     %c stagedstr   when the index differs from HEAD      (default S)
**     %u unstagedstr when the work tree differs from index (default U)
**     %a %i %m %S    state this reader does not track: empty, as at rest
**     anything else  UNTOUCHED -- %F{..}, %f, %B are the PROMPT's escapes,
**                    and zsh leaves them for the prompt to expand
**
** %c and %u exist only under `check-for-changes true`, exactly like zsh,
** and untracked files never light either (zsh's default; our -uno scan).
** Every other key in a :vcs_info context is accepted silently: `enable`,
** `disable`, `max-exports`... are configuration for a reader that has one
** VCS and one message, and saying "not supported" about them would be
** noise about nothing the user can act on.
*/

/* A captured zstyle value, or its zsh default when never set. */
static const char	*style(t_shell *state, const char *var, const char *dflt)
{
	char	*v;

	v = env_expand(state, (char *)var);
	if (!v || !*v)
		return (dflt);
	return (v);
}

/* One %x spec of the format language (see the file comment). */
static void	vcs_spec(t_string *out, char c, t_vcs *v)
{
	const char	*slash;

	slash = ft_strrchr((char *)v->root, '/');
	if (c == 'b')
		vec_push_str(out, (char *)v->branch);
	else if (c == 's')
		vec_push_str(out, "git");
	else if (c == 'r' && slash && slash[1])
		vec_push_str(out, (char *)slash + 1);
	else if (c == 'r')
		vec_push_str(out, (char *)v->root);
	else if (c == 'R')
		vec_push_str(out, (char *)v->root);
	else if (c == 'c' && (v->bits & GIT_STAGED))
		vec_push_str(out, (char *)v->staged);
	else if (c == 'u' && (v->bits & GIT_UNSTAGED))
		vec_push_str(out, (char *)v->unstaged);
	else if (c == '%')
		vec_push_char(out, '%');
	else if (!ft_strchr("cuaimS", c))
	{
		vec_push_char(out, '%');
		vec_push_char(out, c);
	}
}

static void	vcs_expand(t_string *out, const char *f, t_vcs *v)
{
	while (*f)
	{
		if (f[0] != '%' || !f[1])
		{
			vec_push_char(out, *f);
			f++;
			continue ;
		}
		vcs_spec(out, f[1], v);
		f += 2;
	}
}

/* `vcs_info`: fill vcs_info_msg_0_ from the prompt's own fork-free cached
   git reader -- the same source \g uses, so the two can never disagree
   about the branch. Outside a repository the message is empty, exactly
   what a `${vcs_info_msg_0_}` prompt wants to interpolate. */
int	builtin_vcs_info(t_shell *state, t_vec argv)
{
	t_string	out;
	t_vcs		v;
	char		*branch;
	const char	*check;

	(void)argv;
	vec_init(&out);
	out.elem_size = 1;
	get_git_info(&branch, &v.bits);
	v.branch = branch;
	v.root = git_repo_root();
	v.staged = style(state, "HELLISH_VCS_STAGED", "S");
	v.unstaged = style(state, "HELLISH_VCS_UNSTAGED", "U");
	check = style(state, "HELLISH_VCS_CHECK", "false");
	if (ft_strcmp(check, "true") && ft_strcmp(check, "yes")
		&& ft_strcmp(check, "1") && ft_strcmp(check, "on"))
		v.bits &= ~(GIT_STAGED | GIT_UNSTAGED);
	if (branch && v.root)
		vcs_expand(&out, style(state, "HELLISH_VCS_FORMAT", " (%b)"), &v);
	vec_push_char(&out, '\0');
	env_set(&state->env, env_create(ft_strdup("vcs_info_msg_0_"),
			ft_strdup((char *)out.ctx), false));
	xfree(out.ctx);
	xfree(branch);
	return (0);
}

/* `zstyle`: the vcs_info keys are captured and honoured; the rest of a
   :vcs_info context is accepted silently (see the file comment). Anything
   outside vcs_info still goes to the loud once-per-session stub, because
   pretending to store a completion style hellish cannot consult would be
   worse than saying so. */
int	builtin_zstyle(t_shell *state, t_vec argv)
{
	static const char *const	keys[] = {"formats", "actionformats",
		"stagedstr", "unstagedstr", "check-for-changes", NULL};
	static const char *const	vars[] = {"HELLISH_VCS_FORMAT",
		"HELLISH_VCS_AFORMAT", "HELLISH_VCS_STAGED", "HELLISH_VCS_UNSTAGED",
		"HELLISH_VCS_CHECK"};
	char						**av;
	int							i;

	av = (char **)argv.ctx;
	if (argv.len < 3 || !ft_strnstr(av[1], ":vcs_info", ft_strlen(av[1])))
		return (builtin_zunsupported(state, argv));
	i = 0;
	while (keys[i] && ft_strcmp(av[2], keys[i]))
		i++;
	if (keys[i] && argv.len >= 4)
		env_set(&state->env, env_create(ft_strdup(vars[i]),
				ft_strdup(av[3]), false));
	return (0);
}
