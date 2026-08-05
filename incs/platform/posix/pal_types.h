/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pal_types.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PAL_TYPES_H
# define PAL_TYPES_H

/* Platform process-tracking state embedded in t_shell.  POSIX needs
   none: children are addressed by pid alone and the kernel keeps the
   zombie until waitpid.  The win32 sibling stores the pid -> HANDLE
   registry here (holding the handle pins a Windows pid against reuse). */
typedef struct s_pal_procs
{
	int	unused;
}	t_pal_procs;

#endif
