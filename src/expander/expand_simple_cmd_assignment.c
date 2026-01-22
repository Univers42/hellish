/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_simple_cmd_assignment.c                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:29:14 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:29:14 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

int	expand_simple_cmd_assignment(t_shell *state,
		t_expander_simple_cmd *exp, t_executable_cmd *ret)
{
	if (!exp->found_first)
	{
		t_env tmp = assignment_to_env(state, exp->curr);
		vec_push(&ret->pre_assigns, &tmp);
	}
	else
	{
		t_token_old full = (t_token_old){.present = false};
		/* If this is an assignment node, the original word is stored as child[1].
		   Use that to obtain the original full token safely. */
		if (exp->curr->node_type == AST_ASSIGNMENT_WORD && exp->curr->children.len > 1)
		{
			t_ast_node *orig = &((t_ast_node *)exp->curr->children.ctx)[1];
			if (orig->node_type == AST_WORD)
				full = get_old_token(*orig);
		}
		else if (exp->curr->node_type == AST_WORD)
		{
			full = get_old_token(*exp->curr);
		}
		assignment_word_to_word(exp->curr);
		if (exp->export)
			expand_word(state, exp->curr, &ret->argv, true);
		else
			expand_word(state, exp->curr, &ret->argv, false);
		if (get_g_sig()->should_unwind)
			return (1);
		/* Debug: print assignment token and resulting argv if any */
		#ifdef DEBUG_EXPAND
		{
			if (exp->curr->children.len > 0)
			{
				t_token t = ((t_ast_node *)exp->curr->children.ctx)[0].token;
				char buf[256];
				int l = t.len < 255 ? t.len : 255;
				memcpy(buf, t.start, l);
				buf[l] = '\0';
				ft_eprintf("[DEBUG expander] assignment token after transform='%s'\n", buf);
			}
			size_t ai = 0;
			while (ai < ret->argv.len)
			{
				char *s = ((char **)ret->argv.ctx)[ai];
				ft_eprintf("[DEBUG expander] argv[%d] = '%s'\n", (int)ai, s ? s : "(null)");
				ai++;
			}
		}
		#endif
		/* Fixup: use original full token if expansion left an entry ending with '=' */
		if (full.present && full.start)
		{
			size_t ai = 0;
			while (ai < ret->argv.len)
			{
				char *s = ((char **)ret->argv.ctx)[ai];
				if (s && s[0] && s[ft_strlen(s) - 1] == '=')
				{
					char *orig = ft_strndup(full.start, full.len);
					free(((char **)ret->argv.ctx)[ai]);
					((char **)ret->argv.ctx)[ai] = orig;
					#ifdef DEBUG_EXPAND
					ft_eprintf("[DEBUG expander] fix assignment argv[%d] -> '%s'\n", (int)ai, orig);
					#endif
				}
				ai++;
			}
		}
	}
	return (0);
}