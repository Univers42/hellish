/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_ctx.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 19:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/24 19:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* ctx, the "script: line N" every runtime error starts with, kept pointing
   at the executing command (exec_where.c decides where that is). It is
   rebuilt only when the file changes; a new line in the same file is
   written over the old number in place. */

/* Write `line` over the digits at the end of ctx. ctx_rebuild left room
   for any int, so a loop body moving between its lines costs this and no
   allocation -- ctx changes on every command there. */
static void	ctx_renumber(t_shell *state, int line)
{
	char	digits[16];
	int		n;
	long	v;

	v = line;
	if (v < 0)
		v = 0;
	n = 0;
	while (n == 0 || v)
	{
		digits[n++] = (char)('0' + v % 10);
		v /= 10;
	}
	v = (long)state->ctx_num;
	while (n > 0)
		state->ctx[v++] = digits[--n];
	state->ctx[v] = '\0';
}

/* bash's prefixes: "FILE: line N" from a script or -c, "bash: FILE: line
   N" at a prompt; "NAME: line N" for the script or -c text itself, and
   the bare name at a prompt, which has no line to give. */
static void	ctx_rebuild(t_shell *state, const char *src)
{
	char	*pre;
	size_t	n;

	if (src && sh_interactive(state))
		pre = ft_asprintf("%s: %s: line ", state->dft_ctx, src);
	else if (src)
		pre = ft_asprintf("%s: line ", src);
	else if (state->rl.should_update_ctx)
		pre = ft_asprintf("%s: line ", state->dft_ctx);
	else
		pre = ft_strdup(state->dft_ctx);
	if (!pre)
		return ;
	xfree(state->ctx);
	state->ctx_num = 0;
	state->ctx = pre;
	if (!src && !state->rl.should_update_ctx)
		return ;
	n = ft_strlen(pre);
	state->ctx = xmalloc(n + 16);
	if (!state->ctx)
		return ((void)(state->ctx = pre));
	ft_memcpy(state->ctx, pre, n);
	state->ctx_num = n;
	xfree(pre);
}

/* Point ctx at `line` of `src` (NULL: the script or -c text itself). */
void	ctx_follow(t_shell *state, const char *src, int line)
{
	if (src == state->ctx_src && line == state->ctx_line)
		return ;
	if (src != state->ctx_src || !state->ctx_num)
		ctx_rebuild(state, src);
	if (state->ctx_num)
		ctx_renumber(state, line);
	state->ctx_src = src;
	state->ctx_line = line;
}
