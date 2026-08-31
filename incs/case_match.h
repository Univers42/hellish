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

/* extglob (case_match_ext*.c): xg_start says a group begins here,
   xg_match matches the group PLUS the rest of the pattern. */
bool		xg_start(const char *p);
bool		xg_match(const char *s, const char *p);
const char	*xg_group_end(const char *p);
bool		xg_any_alt(const char *s, size_t cut, const char *alts);

#endif
