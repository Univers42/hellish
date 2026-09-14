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

/* Subject and pattern as SLICES -- a cursor and the byte after the last one
   -- rather than two NUL-terminated strings.

   Every caller that asks about a piece of something had to cut that piece
   out first: an extglob alternative is the pattern between two `|`, a
   repetition asks "does the group match the first `cut` bytes", and
   ${v//p/r} asks that of every prefix at every position.  Each of those was
   an ft_strndup, so one ${v//p/r} over a value of n bytes copied O(n^3)
   bytes before it could answer, and bash-preexec -- whose install runs one
   such substitution over PROMPT_COMMAND -- took 7.7 seconds at 122
   characters where bash takes none.  A slice costs nothing to take, so the
   copies are simply gone, and with them the reason the matcher was
   quadratic in memory as well as in time. */
typedef struct s_cmp
{
	const char	*s;
	const char	*se;
	const char	*p;
	const char	*pe;
}	t_cmp;

bool		case_match(const char *s, const char *p);
bool		case_match_n(const char *s, size_t slen, const char *p,
				size_t plen);
bool		cm_run(t_cmp m);

/* POSIX classes for the bracket arm (case_match2.c): skip advances past a
   whole [:name:] (false = unterminated, treat '[' as a member); match
   tests one character against it and advances the same way. */
bool		cm_class_skip(const char **q, const char *qe);
bool		cm_class_match(const char *c, size_t n, const char **pp,
				const char *pe);
const char	*bracket_close(const char *p, const char *pe);

/* Multibyte members (case_match3.c): a range compared by code point --
   bash's default globasciiranges order -- and the classes answered by the
   wide-character tables for a character that is not a single byte. */
bool		cm_in_range(const char *c, size_t n, const char *lo,
				const char *hi);
bool		cm_class_has_w(const char *name, int len, const char *c,
				size_t n);

/* extglob (case_match_ext*.c): xg_start says a group begins here,
   xg_match matches the group PLUS the rest of the pattern. */
bool		xg_start(const char *p, const char *pe);
bool		xg_match(t_cmp m);
const char	*xg_group_end(const char *p, const char *pe, bool brk);
const char	*xg_alt_end(const char *p, const char *pe, bool brk);
bool		xg_any_alt(t_cmp m, size_t cut);

/* Both spellings of a group -- bash's `@(a|b)` and zsh's bare `(a|b)` --
   answered in one place (case_match_ext3.c) so the lexer, the word reparser,
   the filename globber and the matcher cannot disagree about where a group
   starts or ends.  xg_meta is the set a QUOTED segment must escape to stay
   literal; zsh_alt_ahead is the lexer's position-guarded wrapper, because a
   `(` at the start of a word is a subshell and only the lexer has to care. */
const char	*xg_open(const char *p);
bool		xg_meta(char c);
int			xg_alt_group_n(const char *at, const char *pe, bool brk);
int			xg_alt_group(const char *at);
int			zsh_alt_ahead(const char *start, const char *at);

#endif
