/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   self_exe2.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 20:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 20:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Split from self_exe.c only to stay inside the 42 norm's five-functions
   rule: norminette counts both #ifdef branches of self_exe_fill, so the
   file was at six. Nothing here is independent of self_exe.c -- it reads
   the same resolution cell. */

#include "sys.h"

/* True when the image this process is running was replaced or removed after
   it started -- the shell in memory is no longer the binary at its own path.
   `update` uses it to say "restart" instead of offering to re-download a
   release the disk already has. */
int	self_exe_replaced(void)
{
	self_exe_path();
	return (*self_exe_state() == 2);
}
