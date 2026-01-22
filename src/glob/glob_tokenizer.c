/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_tokenizer.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/15 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

// Helper: initialize tokenizer context
static t_tokenizer_ctx	init_tokenizer_ctx(const char *pattern, int *i,
								int len, bool quoted)
{
	t_tokenizer_ctx	ctx;

	ctx.pattern = pattern;
	ctx.i = i;
	ctx.len = len;
	ctx.quoted = quoted;
	return (ctx);
}

/*
** Main tokenization function
** quoted = true means glob special chars should be treated as literals
*/
t_vec_glob	glob_tokenize(const char *pattern, int len, bool quoted)
{
	t_vec_glob		ret;
	int				i;
	t_tokenizer_ctx	ctx;

	i = 0;
	vec_init(&ret);
	ret.elem_size = sizeof(t_glob);
	ctx = init_tokenizer_ctx(pattern, &i, len, quoted);
	ctx.ret = &ret;
	while (i < len)
	{
		if (pattern[i] == '/')
			handle_slash_token(ctx);
		else if (!quoted && pattern[i] == '*')
			handle_asterisk_token(ctx);
		else if (!quoted && pattern[i] == '?')
			handle_question_token(ctx);
		else if (!quoted && pattern[i] == '[')
			handle_bracket_token(ctx);
		else
			handle_literal_token(ctx);
	}
	return (ret);
}
