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
# include <stdint.h>
# include "token_old.h"

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
	TT_DSEMI,
	TT_HERESTRING,
	TT_COPROC,
	TT_PROC_SUB_FILE
}	t_tt;

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

/* Lean token stored in the RAW token deque (t_deque_tok). full_word and
   arith_cache are always NULL there, and `start` is stored as a 32-bit OFFSET
   from the deque's tokenize base (t_deque_tok.base) rather than an 8-byte
   pointer -- halving the slot again, 16B -> 8B: a 50k-line parse holds ~310k
   tokens, so this is ~2.5MB less RSS. It works because deque tokens are ALWAYS
   pure slices into the base string: the lexer never sets `allocated` (only the
   expander does, on AST tokens -- verified), so base+off is exact and it lifts
   to false. tt/len/split_eligible are bitfields so direct reads (`p->tt`,
   `p->len`) still work untouched; only `start` had to move behind ltok2tok().
   len is 24-bit (a single token > 16MB would overflow -- no real lexeme is
   that long). This is NO LONGER a prefix of t_token, so the tokenizer converts
   on push via tok2ltok(). */
typedef struct s_ltoken
{
	uint32_t	off;
	uint32_t	len : 24;
	uint32_t	tt : 7;
	uint32_t	split_eligible : 1;
}	t_ltoken;

/* Lift a deque token to a full AST token, rebuilding the absolute start
   pointer from base+off. off==0 also covers the NULL-start sentinel
   (TT_END): base+0 is a 0-length slice nothing dereferences. */
static inline t_token	ltok2tok(t_ltoken l, char *base)
{
	return ((t_token){.tt = (unsigned char)l.tt, .allocated = false,
		.split_eligible = l.split_eligible, .len = (int)l.len,
		.start = base + l.off, .full_word = NULL});
}

/* Pack a freshly-lexed token into a deque slot: start becomes an offset from
   base. A NULL start (the TT_END sentinel) stores off 0. */
static inline t_ltoken	tok2ltok(t_token t, char *base)
{
	t_ltoken	l;

	if (t.start)
		l.off = (uint32_t)(t.start - base);
	else
		l.off = 0;
	l.len = (uint32_t)t.len;
	l.tt = t.tt;
	l.split_eligible = t.split_eligible;
	return (l);
}

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

#endif