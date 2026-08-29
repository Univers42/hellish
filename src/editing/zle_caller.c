/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle_caller.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 15:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 15:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "zle.h"

/* The shell state the readline child's widgets run against.
**
** get_more_input_readline forks, and the child has no t_shell argument -- it
** never needed one, because readline only ever read a LINE. A widget is a
** shell FUNCTION, so it does. The caller parks the state here on its way
** into the fork and the child reads it back.
**
** A pointer and not a copy: the child is a fork, so the object is already at
** the same address in its own address space. Copying would hand the widget a
** second t_shell that nothing else in the child uses. */
t_shell	**zle_caller_cell(void)
{
	static t_shell	*st;

	return (&st);
}

t_shell	*zle_caller(void)
{
	return (*zle_caller_cell());
}
