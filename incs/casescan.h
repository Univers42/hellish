/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   casescan.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/01 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/01 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Case-aware $(...) span scanning (issue #95). A case pattern's closing
   `)` is unbalanced by design (`a*)` needs no opening paren), so every
   scanner that walks a command substitution looking for its matching `)`
   by counting parens ends the span at the first pattern. This automaton
   is the one shared implementation of the fix: it tracks `case ... esac`
   regions (only in command position, so `echo case` cannot arm it) and
   reports a `)` at the depth where the innermost open `case` started as
   a pattern terminator instead of a closer. Balanced pattern parens
   `(a*)`, extglob `@(a|b)`, body subshells and nested substitutions all
   raised the depth first, so they still close normally.
   Users: the lexer's tokenize_subshell, the word reparser's
   reparse_envvar_paren, the expander's find_cmd_sub_end, span_skip. The
   cmdsub fast path's csf_skip_csub stays naive on purpose: a mis-scan
   there makes eligibility reject and fork, correct by construction.
     The same automaton owns the two other spans inside a substitution
   whose bytes are not shell text (issue #139): a `#` comment, and a
   heredoc body. bash parses $( ) as a script, so `$(# it's<NL>echo x)`
   and `$(cat <<E<NL>a ) 'b<NL>E<NL>)` are fine there; a scanner that only
   knows quotes sees an open quote or an early `)` in both. Keeping them
   here, in the one scanner every user shares, is what keeps the lexer's
   span and the expander's span the same span (casescan2.c). */

#ifndef CASESCAN_H
# define CASESCAN_H

# include <stdbool.h>

/* Unclosed-`case` tracking depth; nesting beyond it falls back to plain
   paren counting (the pre-#95 behaviour). */
# define CASESCAN_MAX 32

/* Heredoc operators whose bodies can be owed at once (`cat <<A <<B`);
   operators past it are not skipped, which is the pre-#139 behaviour. */
# define CASESCAN_HD_MAX 8

/* A `<<word` seen in the span: its body starts at the next newline and
   runs through the line equal to the word (quotes removed, and leading
   tabs ignored for `<<-`). `word` points into the scanned text. */
typedef struct s_cshd
{
	const char	*word;
	int			wlen;
	bool		dash;
}	t_cshd;

typedef struct s_casescan
{
	int		len; /* input length, or -1 for NUL-terminated input */
	int		ncase; /* how many `case` are open */
	int		at[CASESCAN_MAX]; /* paren depth each open case started at */
	bool	cmdpos; /* the next word sits in command position */
	int		nhd; /* heredoc bodies owed at the next newline */
	t_cshd	hd[CASESCAN_HD_MAX];
	int		arith; /* inside (( )) while depth > arith; -1 = not */
}	t_casescan;

void	casescan_init(t_casescan *cs, int len);

/* Feed the UNQUOTED, UNESCAPED character at s[*i]; the caller handles
   quotes and backslash itself (clearing cs->cmdpos when it does).
   Advances *i past what was consumed (one char, a whole keyword, a whole
   comment, a heredoc operator with its word, or the owed heredoc bodies
   after a newline) and returns the paren-depth delta: +1, -1 or 0.
   Every caller starts past the `$(`, so s[*i - 1] is always readable. */
int		casescan_step(t_casescan *cs, const char *s, int *i, int depth);

/* casescan2.c / casescan3.c: the non-shell spans. Each returns true when
   it consumed s[*i] (and what followed); false leaves s[*i] to the
   paren/word steps. `<<` inside (( )) or $(( )) is a shift, never a
   heredoc: a `(` right after a `(` opens that arithmetic span. */
bool	casescan_in(const t_casescan *cs, const char *s, int j);
bool	casescan_comment(t_casescan *cs, const char *s, int *i);
bool	casescan_heredoc_op(t_casescan *cs, const char *s, int *i);
bool	casescan_bodies(t_casescan *cs, const char *s, int *i);
int		casescan_paren(t_casescan *cs, const char *s, int *i, int depth);

#endif
