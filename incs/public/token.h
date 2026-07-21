/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   token.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 00:04:48 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 00:14:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Token types and token struct -- the currency between the lexer and parser.
   Tokens are NOT copied from the input string; t_token holds a (start, len)
   slice.  The `allocated` flag marks the rare cases where the lexer had to
   allocate a new string (e.g. after backslash processing) so free_node
   knows which ones to free and which to leave alone. */

#ifndef TOKEN_H
# define TOKEN_H

# include "libft.h"

/* Exhaustive list of terminal token types the lexer can produce.
   TT_WORD is the catch-all for unquoted words; TT_SQWORD/TT_DQWORD for
   single/double-quoted strings; TT_ENVVAR/$TT_DQENVVAR for $VAR expansions
   still needing expansion at parse time. */
typedef enum e_tt
{
	TT_END,
	TT_WORD,
	TT_REDIRECT_LEFT,
	TT_REDIRECT_RIGHT,
	TT_APPEND,
	TT_PIPE,
	TT_BRACE_LEFT,
	TT_BRACE_RIGHT,
	TT_OR,
	TT_AND,
	TT_SEMICOLON,
	TT_HEREDOC,
	TT_NEWLINE,
	TT_SQWORD,
	TT_DQWORD,
	TT_ENVVAR,
	TT_DQENVVAR,
	TT_AMPERSAND,
	TT_ARITH_START,
	TT_PROC_SUB_IN,
	TT_PROC_SUB_OUT,
	TT_DUP_OUT,
	TT_DUP_IN,
	TT_READWRITE,
	TT_CLOBBER,
	TT_IF,
	TT_THEN,
	TT_ELIF,
	TT_ELSE,
	TT_FI,
	TT_WHILE,
	TT_UNTIL,
	TT_FOR,
	TT_DO,
	TT_DONE,
	TT_CASE,
	TT_ESAC,
	TT_IN,
	TT_LBRACE,
	TT_RBRACE,
	TT_BANG,
	TT_DSEMI
}	t_tt;

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

/* Memoized arithmetic lex, attached to a pure-$((...)) word token so a loop
   re-evaluates without re-lexing/re-parsing. Defined in arith.h; owned by the
   token, freed in free_node, never shared across clones. */
typedef struct s_arith_cache	t_arith_cache;

/* The live token struct used in the AST and passed between lexer/parser.
   start+len is a (non-owning) slice into the input unless allocated=true.
   split_eligible means field splitting is allowed on this token (cleared
   for quoted words and assignment values).
   Layout matters: this struct is embedded in every AST node and every
   deque slot — hundreds of thousands of copies on a big parse. Storing
   the type in a byte (t_tt values fit comfortably) and packing the flags
   beside it shrinks the struct 40 -> 32 bytes; nobody takes the address
   of tt (verified), and enum values round-trip through the byte
   unchanged everywhere it is read or compared. */
typedef struct s_token
{
	unsigned char	tt; /* token type (a t_tt value, byte-packed) */
	bool			allocated; /* true if start is heap-allocated */
	bool			split_eligible; /* false for quoted / assigned values */
	int				len; /* byte length of the token text */
	char			*start; /* points into the input string */
	t_token_old		*full_word; /* back-ref to original word (optional) */
	t_arith_cache	*arith_cache; /* memoised arith parse (owned, or NULL) */
}	t_token;

static inline t_token	create_token(char *start, int len, t_tt token_type)
{
	return ((t_token)
		{
			.tt = token_type,
			.start = start,
			.len = len,
			.allocated = false,
			.full_word = NULL
		});
}

static inline t_token	create_tok4(char *start, int len,
							t_tt token_type, bool allocated)
{
	return ((t_token)
		{
			.tt = token_type,
			.start = start,
			.len = len,
			.allocated = allocated,
			.full_word = NULL
		});
}

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