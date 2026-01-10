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

#include "../shell.h"
#include <errno.h>
#include <unistd.h>

void	buff_readline_update(t_buff_readline *l)
{
	l->has_line = l->cursor != l->buff.len;
}

void	buff_readline_reset(t_buff_readline *l)
{
	ft_memmove((char *)l->buff.ctx, (char *)l->buff.ctx + l->cursor, l->buff.len - l->cursor);
	l->buff.len -= l->cursor;
	if (l->buff.ctx)
		((char *)l->buff.ctx)[l->buff.len] = 0;
	l->cursor = 0;
	buff_readline_update(l);
}

void	buff_readline_init(t_buff_readline *ret)
{
	*ret = (t_buff_readline){};
}

void	update_context(t_state *state)
{
	if (!state->readline_buff.should_update_context)
		return ;
	free(state->context);
	state->context = ft_asprintf("%s: line %i",
			state->base_context, state->readline_buff.line);
}

int	get_more_input_notty(t_state *state)
{
	char	buff[4096 * 2];
	int		ret;
	int		status;

	status = 1;
	set_unwind_sig_norestart();
	state->readline_buff.buff.elem_size = 1;
	while (1)
	{
		ret = read(0, buff, sizeof(buff));
		if (ret < 0 && errno == EINTR)
			status = 2;
		if (ret == 0)
			state->readline_buff.has_finished = true;
		if (ret == 0)
			vec_push_str(&state->readline_buff.buff, "\n");
		if (ret <= 0)
			break ;
		status = 0;
		vec_push_nstr(&state->readline_buff.buff, buff, ret);
		if (ft_strnchr(buff, '\n', ret))
			break ;
	}
	set_unwind_sig();
	buff_readline_update(&state->readline_buff);
	return (status);
}
