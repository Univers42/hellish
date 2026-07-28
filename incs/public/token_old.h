/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   token_old.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* The legacy t_token_old back-reference type and its constructors, split
   out of token.h for the 42-norm five-function file limit. token.h
   includes this among its top includes, so every existing include site
   keeps seeing the full token API unchanged. Do not include directly —
   go through token.h. */

#ifndef TOKEN_OLD_H
# define TOKEN_OLD_H

# include "libft.h"

/* Compact back-reference to the original full word before the lexer split
   it into sub-tokens.  Used by the expander to reconstruct the original
   text for ${v} word forms that span multiple sub-tokens.
   Field order packs the struct to 16 bytes (pointer, int, two flag bytes)
   instead of the 24 the old bool-first layout padded out to. */
typedef struct s_token_old
{
	char	*start; /* pointer to start of original word in input */
	int		len; /* byte length of the original word */
	bool	present; /* true if this back-reference is valid */
	bool	allocated; /* true if start is heap (must be freed) */
}	t_token_old;

static inline t_token_old	create_token_old(char *start, int len, bool present)
{
	return ((t_token_old)
		{
			.start = start,
			.len = len,
			.present = present
		});
}

static inline t_token_old	init_token_old(void)
{
	return ((t_token_old){.present = false, .start = NULL, .len = 0});
}

#endif
