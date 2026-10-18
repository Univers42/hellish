/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   collect4.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/24 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heredoc_private.h"
#include "parena.h"

/* A heredoc inside a function or loop body is re-read at every call, so
   its raw body has to live on the node. When it was not pre-extracted --
   a script's `f() { cat <<E; }` whose body is still ahead in the input,
   or the same line typed at a prompt -- it is read from the live input
   here, the way write_heredoc would have read it, and kept. The old
   fallback wrote it into THIS command's redirect slot; the slot died
   with the command, and the first call of f indexed a freed slot. */

/* Is this raw line the delimiter? Leading tabs are ignored for `<<-`, and
   a trailing newline is not part of the line. */
static bool	live_is_delim(t_hdoc *req, const char *line, size_t len)
{
	size_t	i;
	size_t	sl;

	i = 0;
	while (req->remove_tabs && i < len && line[i] == '\t')
		i++;
	if (len > i && line[len - 1] == '\n')
		len--;
	sl = ft_strlen(req->sep);
	return (len - i == sl && ft_strncmp(line + i, req->sep, sl) == 0);
}

/* Read one raw line onto the body, newline-terminated; true when it was
   the delimiter line. End of input sets req->finished (and warns, once,
   here at definition time) and returns false. */
static bool	live_line(t_shell *state, t_hdoc *req, t_string *body)
{
	t_string	line;
	bool		delim;

	if (get_line_heredoc(state, req, &line))
		return (xfree(line.ctx), false);
	if (line.ctx)
		vec_push_nstr(body, (char *)line.ctx, line.len);
	if (!line.len || ((char *)line.ctx)[line.len - 1] != '\n')
		vec_push_char(body, '\n');
	delim = live_is_delim(req, (char *)line.ctx, line.len);
	req->finished = delim;
	xfree(line.ctx);
	return (delim);
}

/* The body always ends on a delimiter line, the one read or (input ran
   out) one added: materialize_heredoc reads it back through the same
   reader at every call, and a body ending on end-of-file would repeat
   the warning each time. */
bool	capture_heredoc_live(t_shell *state, t_ast_node *node)
{
	t_string	sep;
	t_hdoc		req;
	t_string	body;
	bool		seen;

	sep = word_to_hrdoc_string(((t_ast_node *)node->children.ctx)[1]);
	if (!sep.ctx || !vec_ensure_space_n(&sep, 1))
		return (xfree(sep.ctx), false);
	((char *)sep.ctx)[sep.len] = '\0';
	req = (t_hdoc){.sep = (char *)sep.ctx, .remove_tabs = heredoc_op_strips(
			((t_ast_node *)node->children.ctx)[0].token.start)};
	vec_init(&body);
	body.elem_size = 1;
	seen = false;
	state->rl.line_exact = true;
	while (!req.finished)
		seen = live_line(state, &req, &body);
	state->rl.line_exact = false;
	if (!seen)
		(vec_push_str(&body, (char *)sep.ctx), vec_push_char(&body, '\n'));
	vec_push_char(&body, '\0');
	xfree(sep.ctx);
	node->heredoc_body = (char *)body.ctx;
	parena_note_attach();
	return (true);
}
