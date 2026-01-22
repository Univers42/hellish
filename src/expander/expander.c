/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:37 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:37 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"


/* Expand all occurrences of $((...)) arithmetic and $(...) command substitution
   inside token strings of a node. */
void expand_cmd_substitutions(t_shell *state, t_ast_node *node)
{
	size_t i;

	if (!node->children.ctx)
		return;
	i = 0;
	while (i < node->children.len)
	{
		t_ast_node *ch = &((t_ast_node *)node->children.ctx)[i];
		if (ch->node_type == AST_TOKEN)
		{
			t_token *tok = &ch->token;
			/* Only process TT_WORD tokens for substitution */
			if (tok->tt == TT_WORD)
			{
				const char *s = tok->start;
				int len = tok->len;
				int pos = 0;
				bool changed = false;
				t_string outbuf;
				vec_init(&outbuf);
				outbuf.elem_size = 1;
				while (pos < len)
				{
					/* Check for $(( arithmetic expansion first */
					if (s[pos] == '$' && pos + 2 < len &&
						s[pos + 1] == '(' && s[pos + 2] == '(')
					{
						/* Find matching )) */
						int depth = 2;
						int j = pos + 3;
						while (j < len && depth > 0)
						{
							if (s[j] == '(' && j + 1 < len && s[j + 1] == '(')
							{
								depth += 2;
								j += 2;
							}
							else if (s[j] == ')' && j + 1 < len && s[j + 1] == ')')
							{
								depth -= 2;
								j += 2;
							}
							else if (s[j] == '(')
							{
								depth++;
								j++;
							}
							else if (s[j] == ')')
							{
								depth--;
								j++;
							}
							else
								j++;
						}
						if (depth == 0)
						{
							/* Append prefix before $(( if this is first match */
							if (pos > 0 && outbuf.len == 0)
								vec_push_nstr(&outbuf, s, pos);
							/* Extract arithmetic expression (between (( and )) */
							int expr_start = pos + 3;
							int expr_end = j - 2;
							int expr_len = expr_end - expr_start;
							char *result = arith_expand(state, s + expr_start, expr_len);
							if (result)
							{
								vec_push_nstr(&outbuf, result, ft_strlen(result));
								free(result);
							}
							/* Update scanning */
							s = tok->start + j;
							len = tok->len - j;
							pos = 0;
							changed = true;
							continue;
						}
					}
					/* Check for $( command substitution */
					else if (s[pos] == '$' && pos + 1 < len && s[pos + 1] == '(')
					{
						int depth = 1;
						int j = pos + 2;
						while (j < len && depth > 0)
						{
							if (s[j] == '(')
								depth++;
							else if (s[j] == ')')
								depth--;
							j++;
						}
						if (depth == 0)
						{
							if (pos > 0 && outbuf.len == 0)
								vec_push_nstr(&outbuf, s, pos);
							int inner_start = pos + 2;
							int inner_end = j - 1;
							int inlen = inner_end - inner_start;
							char *inner = malloc(inlen + 1);
							if (inner)
							{
								memcpy(inner, s + inner_start, inlen);
								inner[inlen] = '\0';
								char *subout = capture_subshell_output(state, inner);
								free(inner);
								if (!subout)
									subout = ft_strdup("");
								if (*subout)
									vec_push_nstr(&outbuf, subout, ft_strlen(subout));
								free(subout);
							}
							s = tok->start + j;
							len = tok->len - j;
							pos = 0;
							changed = true;
							continue;
						}
					}
					/* Copy single character */
					char c = s[pos++];
					vec_push(&outbuf, &c);
				}
				if (changed)
				{
					char *newstr = malloc(outbuf.len + 1);
					if (newstr)
					{
						if (outbuf.len)
							memcpy(newstr, outbuf.ctx, outbuf.len);
						newstr[outbuf.len] = '\0';
						if (tok->allocated && tok->start)
							free((char *)tok->start);
						tok->start = newstr;
						tok->len = outbuf.len;
						tok->allocated = true;
					}
					free(outbuf.ctx);
				}
				else
					free(outbuf.ctx);
			}
		}
		expand_cmd_substitutions(state, &((t_ast_node *)node->children.ctx)[i]);
		i++;
	}
}

