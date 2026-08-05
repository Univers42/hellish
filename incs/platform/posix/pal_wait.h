/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pal_wait.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PAL_WAIT_H
# define PAL_WAIT_H

/* POSIX: the platform wait-status macros ARE <sys/wait.h>.  The win32
   sibling of this header defines WIFEXITED/WEXITSTATUS/WIFSIGNALED/
   WTERMSIG over the POSIX-encoded int its pal_wait* shims synthesize,
   so shared code decodes child statuses identically on both platforms. */
# include <sys/wait.h>

#endif
