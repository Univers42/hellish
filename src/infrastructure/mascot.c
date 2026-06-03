/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mascot.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* The mascot's eyes for this frame: mostly open (•ω•), an occasional blink
   (-ω-) and wink (•ω-), and a worried squint (>ω<) after a failed command. */
static const char	*mascot_eyes(size_t frame, int status)
{
	if (status != 0)
		return (">\xcf\x89<");
	if (frame % 33 < 2)
		return ("-\xcf\x89-");
	if (frame % 41 == 20)
		return ("\xe2\x80\xa2\xcf\x89-");
	return ("\xe2\x80\xa2\xcf\x89\xe2\x80\xa2");
}

/* Append the little blinking devil to the prompt; returns its visible width
   (a steady 6 columns: "(xωx) ") so the caller can keep the line padded. */
int	push_mascot(t_string *ret, size_t frame, int status)
{
	if (status != 0)
		vec_push_ansi(ret, "\033[1;38;5;203m");
	else
		vec_push_ansi(ret, "\033[38;5;208m");
	vec_push_str(ret, "(");
	vec_push_str(ret, (char *)mascot_eyes(frame, status));
	vec_push_str(ret, ")");
	vec_push_ansi(ret, "\033[0m");
	vec_push_str(ret, " ");
	return (6);
}
