/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander_simple_cmd.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:22 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:22 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"
#include "shell.h"
#include <stdbool.h>
# include "expander.h"
# include "env.h"

bool	is_export(t_ast_node word)
{
	t_ast_node	c;

	if (word.children.len != 1)
		return (false);
	c = ((t_ast_node *)word.children.ctx)[0];
	if (c.token.tt != TT_WORD)
		return (false);
	if (ft_strncmp(c.token.start, "export", c.token.len))
		return (false);
	return (true);
}

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
		if (g_should_unwind)
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

int	expand_simple_cmd_redir(t_shell *state,
		t_expander_simple_cmd *exp, t_vec_int *redirects)
{
	int			redir_idx;

	if (redirect_from_ast_redir(state, exp->curr, &redir_idx))
	{
		g_should_unwind = 0; // ensure not treated as canceled
		return (AMBIGUOUS_REDIRECT);
	}
	{ int idx = redir_idx; vec_push(redirects, &idx); } // <-- changed
	return (0);
}

int	expand_simple_cmd_word(t_shell *state,
		t_expander_simple_cmd *exp, t_executable_cmd *ret)
{
	if (!exp->found_first && is_export(*exp->curr))
		exp->export = true;
	expand_word(state, exp->curr, &ret->argv, false);
	if (g_should_unwind)
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

int	expand_simple_command(t_shell *state, t_ast_node *node,
		t_executable_cmd *ret, t_vec_int *redirects)
{
	t_expander_simple_cmd	exp;

	if (node == NULL || node->children.len == 0 || node->children.ctx == NULL)
		return (1);
	/* defensive sanity: ensure child elem_size set so vec_idx arithmetic works */
	if (node->children.elem_size == 0)
		node->children.elem_size = sizeof(t_ast_node);
	exp = (typeof(exp)){};
	*ret = (t_executable_cmd){};
	/* init pre_assigns vector to hold t_env entries */
	vec_init(&ret->pre_assigns);
	ret->pre_assigns.elem_size = sizeof(t_env);
	vec_init(&ret->argv);
	ret->argv.elem_size = sizeof(char *);
	while (exp.i < node->children.len)
	{
		exp.curr = (t_ast_node *)vec_idx(&node->children, exp.i);
		if (exp.curr->node_type == AST_WORD)
			exp.exit_stat = expand_simple_cmd_word(state, &exp, ret);
		else if (exp.curr->node_type == AST_ASSIGNMENT_WORD)
			exp.exit_stat = expand_simple_cmd_assignment(state, &exp, ret);
		else if (exp.curr->node_type == AST_REDIRECT)
			exp.exit_stat = expand_simple_cmd_redir(state, &exp, redirects);
		else if (exp.curr->node_type == AST_PROC_SUB)
		{
			/* Expand process substitution and add result to argv */
			char *path = expand_proc_sub(state, exp.curr);
			if (path)
			{
				vec_push(&ret->argv, &path);
				exp.found_first = true;
			}
			exp.exit_stat = 0;
		}
		else
		{
#if TRACE_DEBUG
			ft_eprintf("debug: expand_simple_command unexpected node_type=%d at idx=%lu\n",
				(int)exp.curr->node_type, (unsigned long)exp.i);
#endif
			return (1);
		}
		if (exp.exit_stat)
			return (exp.exit_stat);
		exp.i++;
	}
	/* If expansion produced only assignments and no argv (assignment-only form),
	   apply those to the shell environment now to take ownership of their strings.
	   This prevents leaks and double-free later when the temporary cmd is freed. */
	if (ret->argv.len == 0 && ret->pre_assigns.len > 0)
	{
		/* Pop each pre_assign and insert it into the shell env (transfer ownership) */
		while (ret->pre_assigns.len)
		{
			t_env tmp = *(t_env *)vec_pop(&ret->pre_assigns);
			/* ensure exported flag is preserved */
			env_set(&state->env, tmp);
		}
		/* free the backing storage and reinit vector */
		free(ret->pre_assigns.ctx);
		vec_init(&ret->pre_assigns);
	}
	return (0);
}
