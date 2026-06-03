/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_styles9.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_styles.h"

/* Line 4: the forked flame-tail under the imp, flickering. (14 visible cols) */
void	devil_line4(t_string *ret, size_t f)
{
	push_fg(ret, fire_hue(f + 2));
	vec_push_str(ret, "    \xe2\x95\xb2\xe2\x95\xb1\xe2\x95\xb2\xe2\x95\xb1");
	vec_push_str(ret, A_RST "      ");
}

/* Right of the head, line 2: who and where, in warm parchment tones. */
void	flame_info_user(t_string *ret, t_prompt *p)
{
	vec_push_str(ret, "  ");
	push_fg(ret, 223);
	vec_push_str(ret, p->user);
	vec_push_str(ret, A_RST "  ");
	push_fg(ret, 180);
	vec_push_str(ret, p->short_cwd);
	vec_push_str(ret, A_RST);
}

/* Right of the head, line 3: the git branch (nothing when not in a repo). */
void	flame_info_branch(t_string *ret, t_prompt *p)
{
	if (!p->branch)
		return ;
	vec_push_str(ret, "  ");
	push_fg(ret, 173);
	vec_push_str(ret, G_BRANCH);
	vec_push_str(ret, p->branch);
	vec_push_str(ret, A_RST);
}

/* Right of the tail, line 4: the burning fuse + the clock, sized to the rest
   of the terminal width (the head block is a fixed 14 columns). */
void	flame_info_bar(t_string *ret, t_prompt *p, size_t frame)
{
	int	n;

	n = p->cols - 14 - 2 - (int)ft_strlen(p->time_buf) - 4;
	if (n < 6)
		n = 6;
	if (n > 50)
		n = 50;
	vec_push_str(ret, "  ");
	fuse_bar(ret, frame, n);
	vec_push_str(ret, "  \001\033[38;5;240m\002");
	vec_push_str(ret, p->time_buf);
	vec_push_str(ret, A_RST);
}
