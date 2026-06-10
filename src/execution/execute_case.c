/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_case.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "expander.h"

bool	case_match(const char *s, const char *p);

/* Expand a case word (subject or pattern) to a single string: parameter,
   command and arithmetic substitution and tilde, but NO field splitting and
   NO pathname (glob) expansion - a case pattern *is* the glob.  Exported:
   [[ ]] operands follow exactly these rules (expand_dbracket.c). */
char	*expand_case_word(t_shell *state, t_ast_node *node)
{
	t_ast_node	copy;
	t_string	s;
	char		*ret;

	copy = clone_ast(node);
	expand_tilde_word(state, &copy);
	expand_cmd_substitutions(state, &copy);
	expand_env_vars(state, &copy, false);
	s = word_to_string(copy);
	ret = ft_strndup((char *)s.ctx, s.len);
	xfree(s.ctx);
	free_ast(&copy);
	return (ret);
}

bool	is_quoted_tok(t_tt tt);
void	append_pat_tok(t_string *s, t_token t);

/* Like expand_case_word but for a PATTERN: quoted glob metacharacters are
   escaped so they match literally, unquoted ones stay active.  Exported:
   the word after ==/=/!= inside [[ ]] is a pattern with the same rules. */
char	*expand_case_pattern(t_shell *state, t_ast_node *node)
{
	t_ast_node	copy;
	t_string	s;
	size_t		i;
	char		*ret;

	copy = clone_ast(node);
	expand_tilde_word(state, &copy);
	expand_cmd_substitutions(state, &copy);
	expand_env_vars(state, &copy, false);
	vec_init(&s);
	s.elem_size = 1;
	i = -1;
	while (copy.children.ctx && ++i < copy.children.len
		&& ((t_ast_node *)copy.children.ctx)[i].node_type == AST_TOKEN)
		append_pat_tok(&s, ((t_ast_node *)copy.children.ctx)[i].token);
	if (s.ctx)
		ret = ft_strndup((char *)s.ctx, s.len);
	else
		ret = ft_strndup("", s.len);
	xfree(s.ctx);
	free_ast(&copy);
	return (ret);
}

/* Run the matched case-item body.  We clone the AST body before executing
   because execute_tree_node may destructively consume word nodes; cloning
   keeps the original intact so the surrounding AST is not corrupted if
   this case statement is inside a function that is called more than once. */
static t_execution_state	run_case_body(t_shell *state, t_ast_node *body)
{
	t_executable_node	exe;
	t_ast_node			copy;
	t_execution_state	st;

	copy = clone_ast(body);
	exe = create_exe_node(STDIN_FILENO, STDOUT_FILENO, &copy, true);
	st = execute_tree_node(state, &exe);
	free_ast(&copy);
	return (st);
}

/* Does any pattern of `item` (all children except the trailing body) match
   the already-expanded subject string? */
static bool	item_matches(t_shell *state, t_ast_node *item, const char *subj)
{
	size_t	i;
	char	*pat;
	bool	m;

	i = 0;
	while (i + 1 < item->children.len)
	{
		pat = expand_case_pattern(state, vec_idx(&item->children, i));
		m = case_match(subj, pat);
		xfree(pat);
		if (m)
			return (true);
		i++;
	}
	return (false);
}

/* AST_CASE: children[0]=subject word, children[1..]=AST_CASE_ITEM, each with
   pattern words followed by a trailing compound-list body. Runs the body of
   the first item with a matching pattern (no fall-through). */
t_execution_state	execute_case(t_shell *state, t_executable_node *exe)
{
	char				*subj;
	t_ast_node			*item;
	t_execution_state	st;
	size_t				i;

	ft_assert(exe->node->children.len >= 1);
	subj = expand_case_word(state, vec_idx(&exe->node->children, 0));
	st = res_status(0);
	i = 0;
	while (++i < exe->node->children.len)
	{
		item = vec_idx(&exe->node->children, i);
		if (item->node_type == AST_CASE_ITEM && item->children.len >= 2
			&& item_matches(state, item, subj))
		{
			st = run_case_body(state,
					vec_idx(&item->children, item->children.len - 1));
			break ;
		}
	}
	return (xfree(subj), st);
}
