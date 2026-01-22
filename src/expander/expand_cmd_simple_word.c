/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_cmd_simple_word.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:29:56 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:29:56 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

int	expand_simple_cmd_word(t_shell *state,
		t_expander_simple_cmd *exp, t_executable_cmd *ret)
{
	if (!exp->found_first && is_export(*exp->curr))
		exp->export = true;
	expand_word(state, exp->curr, &ret->argv, false);
	if (get_g_sig()->should_unwind)
		return (1);
	exp->found_first = true;
	/* Debug: print the token being expanded and the current argv contents */
	#ifdef DEBUG_EXPAND
	{
		if (exp->curr->children.len > 0)
		{
			t_token t = ((t_ast_node *)exp->curr->children.ctx)[0].token;
			char buf[256];
			int l = t.len < 255 ? t.len : 255;
			memcpy(buf, t.start, l);
			buf[l] = '\0';
			ft_eprintf("[DEBUG expander] expand_word token='%s'\n", buf);
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
	/* Fixup: if expansion produced a word that ends with '=' (no RHS),
	   replace it with the raw original token full_word which may contain the RHS. */
	{
		size_t ai = 0;
		while (ai < ret->argv.len)
		{
			char *s = ((char **)ret->argv.ctx)[ai];
			if (s && s[0] && s[ft_strlen(s) - 1] == '=')
			{
				/* attempt to get original full token only when safe */
				if (exp->curr->node_type == AST_WORD && exp->curr->children.len > 0 && exp->curr->children.ctx)
				{
					t_token_old full = get_old_token(*exp->curr);
					if (full.present && full.len > 0 && full.start)
					{
						char *orig = ft_strndup(full.start, full.len);
						free(((char **)ret->argv.ctx)[ai]);
						((char **)ret->argv.ctx)[ai] = orig;
						#ifdef DEBUG_EXPAND
						ft_eprintf("[DEBUG expander] fix argv[%d] -> '%s'\n", (int)ai, orig);
						#endif
					}
				}
			}
			ai++;
		}
	}
	return (0);
}