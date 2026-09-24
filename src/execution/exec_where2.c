/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_where2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 19:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/24 19:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Enter a sourced file: its errors name `src` from line 1. The caller's
   frame, ctx string included, is parked in `saved`. */
void	where_push(t_shell *state, t_where *saved, const char *src)
{
	saved->src = state->err_src;
	saved->line = state->err_line;
	saved->pos = state->err_pos;
	saved->ctx = ft_strdup(state->ctx);
	saved->ctx_src = state->ctx_src;
	saved->ctx_line = state->ctx_line;
	state->err_src = src;
	state->err_line = 1;
	state->err_pos = (t_srcpos){0};
}

/* Leave it: the caller's frame comes back as it was, so an error reported
   right after the `source` -- by the next command, or by the dot builtin
   itself -- names the caller's line, not the file's last one. */
void	where_pop(t_shell *state, t_where *saved)
{
	state->err_src = saved->src;
	state->err_line = saved->line;
	state->err_pos = saved->pos;
	if (!saved->ctx)
		return ;
	xfree(state->ctx);
	state->ctx = saved->ctx;
	state->ctx_src = saved->ctx_src;
	state->ctx_line = saved->ctx_line;
	state->ctx_num = 0;
}
