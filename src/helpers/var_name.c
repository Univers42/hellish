/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   var_name.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:25:44 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 14:25:44 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdbool.h>
#include <ctype.h>

/* POSIX says a variable name starts with [a-zA-Z_]. The unsigned char cast
   is required by the C standard: isalpha on a plain char would be UB for
   chars with the high bit set (e.g. UTF-8 bytes). */
bool	is_var_name_p1(char c)
{
	return (isalpha((unsigned char)c) || c == '_');
}

/* Subsequent characters extend to [a-zA-Z0-9_]. Same unsigned cast for the
   same reason. The two-function split (p1 / p2) lets callers cheaply detect
   whether they're looking at a valid identifier start vs continuation without
   duplicating the is-digit check. */
bool	is_var_name_p2(char c)
{
	return (isalnum((unsigned char)c) || c == '_');
}
