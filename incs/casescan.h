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
   reparse_envvar_paren, the expander's find_cmd_sub_end. The cmdsub fast
   path's csf_skip_csub stays naive on purpose: a mis-scan there makes
   eligibility reject and fork, which is correct by construction. */

#ifndef CASESCAN_H
# define CASESCAN_H

# include <stdbool.h>

/* Unclosed-`case` tracking depth; nesting beyond it falls back to plain
   paren counting (the pre-#95 behaviour). */
# define CASESCAN_MAX 32

typedef struct s_casescan
{
	int		len; /* input length, or -1 for NUL-terminated input */
	int		ncase; /* how many `case` are open */
	int		at[CASESCAN_MAX]; /* paren depth each open case started at */
	bool	cmdpos; /* the next word sits in command position */
}	t_casescan;

void	casescan_init(t_casescan *cs, int len);

/* Feed the UNQUOTED, UNESCAPED character at s[*i]; the caller handles
   quotes and backslash itself (clearing cs->cmdpos when it does).
   Advances *i past what was consumed (one char, or a whole keyword) and
   returns the paren-depth delta to apply: +1, -1 or 0. */
int		casescan_step(t_casescan *cs, const char *s, int *i, int depth);

#endif
