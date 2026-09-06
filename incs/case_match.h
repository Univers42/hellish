/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   case_match.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* The shell's ONE pattern matcher for whole strings: `case` patterns, the
   right-hand side of `[[ == ]]`, and the extglob groups inside a filename
   glob all come through here.  Keeping it one function is the point -- a
   second matcher would eventually disagree with this one about some corner,
   and a pattern that means two things depending on where it is written is
   the worst kind of bug to read. */

#ifndef CASE_MATCH_H
# define CASE_MATCH_H

# include <stdbool.h>
# include <stddef.h>

bool		case_match(const char *s, const char *p);

/* POSIX classes for the bracket arm (case_match2.c): skip advances past a
   whole [:name:] (false = unterminated, treat '[' as a member); match
   tests one character against it and advances the same way. */
bool		cm_class_skip(const char **q);
bool		cm_class_match(const char *c, size_t n, const char **pp);
const char	*bracket_close(const char *p);

/* Multibyte members (case_match3.c): a range compared by code point --
   bash's default globasciiranges order -- and the classes answered by the
   wide-character tables for a character that is not a single byte. */
bool		cm_in_range(const char *c, size_t n, const char *lo,
				const char *hi);
bool		cm_class_has_w(const char *name, int len, const char *c,
				size_t n);

/* extglob (case_match_ext*.c): xg_start says a group begins here,
   xg_match matches the group PLUS the rest of the pattern. */
bool		xg_start(const char *p);
bool		xg_match(const char *s, const char *p);
const char	*xg_group_end(const char *p);
const char	*xg_alt_end(const char *p);
bool		xg_any_alt(const char *s, size_t cut, const char *alts);

/* Both spellings of a group -- bash's `@(a|b)` and zsh's bare `(a|b)` --
   answered in one place (case_match_ext3.c) so the lexer, the word reparser,
   the filename globber and the matcher cannot disagree about where a group
   starts or ends.  xg_meta is the set a QUOTED segment must escape to stay
   literal; zsh_alt_ahead is the lexer's position-guarded wrapper, because a
   `(` at the start of a word is a subshell and only the lexer has to care. */
const char	*xg_open(const char *p);
bool		xg_meta(char c);
int			xg_alt_group(const char *at);
int			zsh_alt_ahead(const char *start, const char *at);

#endif
