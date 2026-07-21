/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_utils5.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

#define C_RST "\033[0m"

/* Hostname, fetched once and cached, domain part stripped ("web01.corp.x"
   -> "web01") — the short name is what identifies the box at a glance. */
static const char	*host_name(void)
{
	static char	host[64];

	if (!host[0])
	{
		if (gethostname(host, sizeof(host) - 1) != 0)
			ft_strlcpy(host, "host", sizeof(host));
		host[sizeof(host) - 1] = '\0';
		host[ft_strcspn(host, ".")] = '\0';
	}
	return (host);
}

/* Are we on an SSH connection? Checked once: the answer cannot change
   within a session. */
static int	ssh_active(void)
{
	static int	mode = -1;

	if (mode >= 0)
		return (mode);
	mode = 0;
	if (getenv("SSH_CONNECTION") || getenv("SSH_TTY"))
		mode = 1;
	return (mode);
}

/* Username (accent colour, red for root) plus a dim @host suffix — but the
   host only over SSH, where "which machine am I typing at" actually needs
   answering. Local sessions keep the shorter row. */
void	push_user_seg(t_string *ret, t_prompt *p)
{
	vec_push_ansi(ret, user_color());
	vec_push_str(ret, p->user);
	vec_push_ansi(ret, C_RST);
	p->vis_w += (int)ft_strlen(p->user);
	if (!ssh_active())
		return ;
	vec_push_ansi(ret, pal(PAL_HOST));
	vec_push_str(ret, "@");
	vec_push_str(ret, (char *)host_name());
	vec_push_ansi(ret, C_RST);
	p->vis_w += 1 + (int)ft_strlen(host_name());
}

/* The cwd, parents dimmed and the last component bright-bold, so the eye
   lands on where you ARE ("~/Documents/" fades, "hellish" pops). The split
   reuses short_cwd in place with a save/restore NUL — no extra allocation.
   Width bookkeeping uses measure_width, not strlen: UTF-8 dir names must
   count display columns or the right-side clock drifts. */
void	push_cwd_seg(t_string *ret, t_prompt *p)
{
	char	*cut;
	char	sv;

	cut = ft_strrchr(p->short_cwd, '/');
	if (cut && cut[1])
		cut++;
	else
		cut = p->short_cwd;
	if (cut != p->short_cwd)
	{
		sv = *cut;
		*cut = '\0';
		vec_push_ansi(ret, pal(PAL_CWDD));
		vec_push_str(ret, p->short_cwd);
		*cut = sv;
	}
	vec_push_ansi(ret, pal(PAL_CWD));
	vec_push_str(ret, cut);
	vec_push_ansi(ret, C_RST);
	p->vis_w += measure_width(p->short_cwd);
}
