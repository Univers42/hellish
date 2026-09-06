/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mbchar.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MBCHAR_H
# define MBCHAR_H

# include <stddef.h>

/* Multibyte-aware string arithmetic (src/helpers/mbchar.c): characters
   in a multibyte locale, bytes in the C locale, an undecodable byte
   counting as one character -- bash's rule for ${#v}, ${v:off:len}, `?`
   in a pattern and read -n. */
size_t	mb_len(const char *s, size_t max);
size_t	mb_len0(const char *s);
size_t	mb_count(const char *s, size_t n);
size_t	mb_skip(const char *s, size_t n, size_t nth);
size_t	mb_conv(const char *s, size_t n, char op, char *out);
size_t	mb_back(const char *s, size_t i);

#endif
