/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   buffered_readline_utils.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:00 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

void	buff_readline_update(t_rl *l)
{
	if (!l->buff.ctx || l->buff.len == 0)
	{
		l->cursor = 0;
		l->has_line = false;
		return ;
	}
	if (l->cursor > l->buff.len)
		l->cursor = l->buff.len;
	l->has_line = l->cursor != l->buff.len;
}

void	buff_readline_reset(t_rl *l)
{
	if (l->buff.len > l->cursor)
		ft_memmove((char *)l->buff.ctx, (char *)l->buff.ctx + l->cursor,
			l->buff.len - l->cursor);
	else if (l->buff.len > 0)
		ft_memmove((char *)l->buff.ctx, (char *)l->buff.ctx, l->buff.len);
	l->buff.len -= l->cursor;
	if (l->buff.ctx)
		((char *)l->buff.ctx)[l->buff.len] = 0;
	l->cursor = 0;
	buff_readline_update(l);
}

void	buff_readline_init(t_rl *ret)
{
	*ret = (t_rl){};
}

void	update_ctx(t_shell *state)
{
	if (!state->rl.should_update_ctx)
		return ;
	free(state->ctx);
	state->ctx = (char *)ft_asprintf("%s: line %i",
			state->dft_ctx, state->rl.line);
}

int	get_more_input_notty(t_shell *state)
{
	char	buff[4096 * 2];
	int		ret;
	int		status;

	status = 1;
	set_unwind_sig_norestart();
	state->rl.buff.elem_size = 1;
	while (1)
	{
		ret = read(0, buff, sizeof(buff));
		if (ret < 0 && errno == EINTR)
			status = 2;
		if (ret == 0)
			state->rl.has_finished = true;
		if (ret == 0)
			vec_push_str(&state->rl.buff, "\n");
		if (ret <= 0)
			break ;
		status = 0;
		vec_push_nstr(&state->rl.buff, buff, ret);
		if (ft_strnchr(buff, '\n', ret))
			break ;
	}
	return (set_unwind_sig(),
		buff_readline_update(&state->rl), status);
}
