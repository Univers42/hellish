/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   trap_handler_string.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 01:57:34 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/08 06:17:06 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"

// #ifdef DEBUG
// /* Return a printable representation of the trap handler for SIG. */
// static char *
// trap_handler_string (int sig)
// {
//   if (trap_list[sig] == (char *)DEFAULT_SIG)
//     return "DEFAULT_SIG";
//   else if (trap_list[sig] == (char *)IGNORE_SIG)
//     return "IGNORE_SIG";
//   else if (trap_list[sig] == (char *)IMPOSSIBLE_TRAP_HANDLER)
//     return "IMPOSSIBLE_TRAP_HANDLER";
//   else if (trap_list[sig])
//     return trap_list[sig];
//   else
//     return "NULL";
// }
// #endif