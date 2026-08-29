/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word6.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:41:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:41:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sys.h"

static char	*build_proc_sub_result(t_shell *state, t_ast_node *cmd_word,
				bool is_input)
{
	t_string	cmd_str;
	char		*result;

	cmd_str = word_to_string(*cmd_word);
	if (!cmd_str.ctx)
		return (ft_strdup(BLACK_HOLE));
	if (!vec_ensure_space_n(&cmd_str, 1))
		return (xfree(cmd_str.ctx), NULL);
	((char *)cmd_str.ctx)[cmd_str.len] = '\0';
	if (is_input)
		result = create_procsub_input(state, (char *)cmd_str.ctx);
	else
		result = create_procsub_output(state, (char *)cmd_str.ctx);
	xfree(cmd_str.ctx);
	return (result);
}

/* Stringify the command and run it into a temp file -- zsh's `=(cmd)`.
   Split out because the file form takes no direction: it is always the
   command's output, and the difference from `<(cmd)` is the KIND of path
   handed back (a real file, seekable and reopenable) rather than which
   way the data flows. */
static char	*build_proc_sub_file(t_shell *state, t_ast_node *cmd_word)
{
	t_string	cmd_str;
	char		*result;

	cmd_str = word_to_string(*cmd_word);
	result = create_procsub_file(state, (char *)cmd_str.ctx);
	xfree(cmd_str.ctx);
	return (result);
}

/* Top-level process-substitution expander: reads the direction token
   (TT_PROC_SUB_IN, TT_PROC_SUB_OUT, or zsh's TT_PROC_SUB_FILE) to pick
   the mode, then delegates to create_procsub_input / output / file.
   The returned string is the
   /dev/fd/N path that the shell word is replaced with. */
char	*expand_proc_sub(t_shell *state, t_ast_node *node)
{
	t_token		*tok;
	t_ast_node	*cmd_word;
	bool		is_input;

	if (!node || node->node_type != AST_PROC_SUB || node->children.len < 2)
		return (NULL);
	tok = &((t_ast_node *)node->children.ctx)[0].token;
	cmd_word = &((t_ast_node *)node->children.ctx)[1];
	if (tok->tt == TT_PROC_SUB_FILE)
		return (build_proc_sub_file(state, cmd_word));
	is_input = (tok->tt == TT_PROC_SUB_IN);
	return (build_proc_sub_result(state, cmd_word, is_input));
}
