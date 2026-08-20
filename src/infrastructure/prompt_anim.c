/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_anim.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "env.h"

/* Config-driven prompt animation. Place \A anywhere in PS1 and pick a
   style with HELLISH_ANIM (spinner | pulse | ember | off): the glyph
   advances ~10x/second while the shell waits at the prompt, repainted
   in place by the readline idle hook without disturbing typed input.
   \A renders nothing when animation is off, so themes can keep it in
   the string permanently. (This shadows bash's \A 24h-clock escape —
   the trade is documented in hellishrc.example.) */

/* Braille spinner frames, each one column wide. */
static const char	*g_spin[PANIM_FRAMES] = {
	"\xe2\xa0\x8b", "\xe2\xa0\x99", "\xe2\xa0\xb9", "\xe2\xa0\xb8",
	"\xe2\xa0\xbc", "\xe2\xa0\xb4", "\xe2\xa0\xa6", "\xe2\xa0\xa7",
};

/* Pulse: a ✦ breathing through magenta — triangle wave of brightness. */
static const char	*g_pulse[PANIM_FRAMES] = {
	"\033[1;38;2;79;48;88m", "\033[1;38;2;109;66;122m",
	"\033[1;38;2;139;84;155m", "\033[1;38;2;168;102;188m",
	"\033[1;38;2;198;120;221m", "\033[1;38;2;168;102;188m",
	"\033[1;38;2;139;84;155m", "\033[1;38;2;109;66;122m",
};

/* Ember: a ▲ flickering through fire hues in a jittery sequence. */
static const char	*g_ember[PANIM_FRAMES] = {
	"\033[1;38;2;255;140;60m", "\033[1;38;2;224;90;50m",
	"\033[1;38;2;255;190;80m", "\033[1;38;2;240;120;40m",
	"\033[1;38;2;200;70;40m", "\033[1;38;2;255;160;70m",
	"\033[1;38;2;230;110;50m", "\033[1;38;2;235;170;90m",
};

/* The fork-inherited frame store (see prompt_private.h). */
t_panim	*anim_cells(void)
{
	static t_panim	a;

	return (&a);
}

/* 0 = off/unset/unknown, 1 = spinner, 2 = pulse, 3 = ember.
   UNSET means OFF: animation is opt-in. It used to be that only the \A
   escape consulted this, while the built-in prompt animated no matter
   what -- so a user who set HELLISH_ANIM=off, or never set it at all,
   still got a prompt whose mascot changed on every render and no way to
   stop it short of writing a whole custom PS1. */
int	anim_style(t_shell *state)
{
	char	*s;

	s = env_expand(state, "HELLISH_ANIM");
	if (!s || !*s)
		return (0);
	if (!ft_strcmp(s, "spinner"))
		return (1);
	if (!ft_strcmp(s, "pulse"))
		return (2);
	if (!ft_strcmp(s, "ember"))
		return (3);
	return (0);
}

/* The \A escape: one frame of the selected style at the current global
   frame counter, nothing at all when animation is off. */
void	ps1_anim(t_shell *state, t_string *out)
{
	int	f;
	int	st;

	st = anim_style(state);
	if (st == 0)
		return ;
	f = (int)(*anim_frame() % PANIM_FRAMES);
	if (st == 1)
		vec_push_ansi(out, "\033[1;38;2;125;207;255m");
	else if (st == 2)
		vec_push_ansi(out, g_pulse[f]);
	else
		vec_push_ansi(out, g_ember[f]);
	if (st == 1)
		vec_push_str(out, (char *)g_spin[f]);
	else if (st == 2)
		vec_push_str(out, "\xe2\x9c\xa6");
	else
		vec_push_str(out, "\xe2\x96\xb2");
	vec_push_ansi(out, "\033[0m");
}

/* Pre-render one prompt variant per frame into the cells. Variants
   differ only in the \A glyph, so the row count and layout are stable
   and the idle hook can swap them freely. A prompt too big for the
   fixed cells simply leaves count at 0 — animation off, prompt fine. */
static void	anim_build(t_shell *state, char *ps1)
{
	t_panim		*a;
	t_string	v;
	size_t		base;
	int			k;

	a = anim_cells();
	base = *anim_frame();
	k = 0;
	while (k < PANIM_FRAMES)
	{
		*anim_frame() = base + k;
		v = ps1_render(state, ps1);
		if (v.len + 1 > PANIM_MAX)
			return (xfree(v.ctx), (void)(*anim_frame() = base));
		ft_memcpy(a->buf[k], v.ctx, v.len + 1);
		xfree(v.ctx);
		k++;
	}
	*anim_frame() = base;
	a->count = PANIM_FRAMES;
}

/* PS1 entry used by prompt_normal: render the prompt, and when it
   contains \A with a live style, build the frame variants and record
   the last row's visible width (the repaint wrap math needs it). */
t_string	ps1_animated(t_shell *state, char *ps1)
{
	t_string	r;
	char		*last;

	anim_cells()->count = 0;
	r = ps1_render(state, ps1);
	if (anim_style(state) == 0 || !ft_strstr(ps1, "\\A"))
		return (r);
	anim_build(state, ps1);
	last = ft_strrchr((char *)r.ctx, '\n');
	if (last)
		anim_cells()->last_w = visible_width_cstr(last + 1);
	else
		anim_cells()->last_w = visible_width_cstr((char *)r.ctx);
	return (r);
}
