/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   replace_trailing_equal_with_full_token.c           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 01:20:04 by marvin            #+#    #+#             */
/*   Updated: 2026/01/23 01:20:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Wrap a freshly allocated string in a minimal AST_TOKEN / TT_ENVVAR node
   with allocated=true.  Used by split_envvar and emit_positional_at to build
   field tokens from IFS-split pieces without going through the parser. */
t_ast_node	new_env_node(char *new_start)
{
	t_ast_node	ret;

	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	ret = (t_ast_node){.node_type = AST_TOKEN,
		.token = {.allocated = true,
		.len = ft_strlen(new_start),
		.start = new_start,
		.tt = TT_ENVVAR}};
	return (ret);
}

/* Return a dup of the original source text for `node` (via get_old_token),
   or NULL when no full_word annotation is present.  This text is used to
   replace the "trailing-equals" argv entry with the original user text. */
static char	*dup_full_token_if_available(t_ast_node *node)
{
	t_token_old	full;

	if (!node || node->node_type != AST_WORD
		|| node->children.len == 0 || !node->children.ctx)
		return (NULL);
	full = get_old_token(*node);
	if (!full.present || full.len <= 0 || !full.start)
		return (NULL);
	return (ft_strndup(full.start, full.len));
}

/* If argv[idx] ends with '=', it came from an assignment-word expansion that
   field-split the value away.  Replace it with the original source text so
   `export` and similar builtins display the full "NAME=value" rather than
   just "NAME=". */
static void	replace_arg_if_trailing_equal(t_vec *argv, size_t idx,
				t_ast_node *node)
{
	char	*val;
	char	*dup;

	if (!argv || !argv->ctx || idx >= argv->len)
		return ;
	val = ((char **)argv->ctx)[idx];
	if (!val || !val[0] || val[ft_strlen(val) - 1] != '=')
		return ;
	dup = dup_full_token_if_available(node);
	if (!dup)
		return ;
	word_free(((char **)argv->ctx)[idx]);
	((char **)argv->ctx)[idx] = dup;
}

/* Walk the entire argv vector and replace any "NAME=" entry with the original
   full token text from `node`.  Called after expand_word_glob_ctl so the
   field-split product is patched up before being handed to the executor. */
void	replace_trailing_equal_with_full_token(t_ast_node *node, t_vec *argv)
{
	size_t	ai;

	if (!argv || !argv->ctx)
		return ;
	ai = 0;
	while (ai < argv->len)
	{
		replace_arg_if_trailing_equal(argv, ai, node);
		ai++;
	}
}
