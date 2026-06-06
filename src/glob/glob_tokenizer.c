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

/* Pack the tokenizer state into a t_tokenizer_ctx. Passing ctx by value to
   each handle_* function means each handler gets a consistent snapshot; the
   shared `i` pointer is the only mutable state. */
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

/* Break a glob pattern string into a t_vec_glob of typed tokens. Each token
   is one of: G_SLASH (path separator), G_ASTERISK (* wildcard), G_QUESTION
   (? wildcard), G_BRACKET ([..] class), or G_LITERAL (plain text). When
   `quoted` is true every special character is treated as a literal -- this
   is how double-quoted words with $(...) get their wildcards suppressed.
   G_SLASH tokens are always emitted regardless of quoting because slashes
   are never special in POSIX glob patterns; they're structural separators
   for the directory-walk logic in match_dir. */
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
