/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sh.h                                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/19 15:04:51 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/19 15:22:49 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SH_H
# define SH_H
#include <unistd.h>

//Those functions has to be maybe placed into the libarena implementation
// it's more related to mmemory management, for strings or data structures
typedef union u_smt
{
	int		i;
	char	*cp;
	double	d;
}	t_smt;

// return the size of union - 1. This value is for allocation granurality
static inline size_t	shell_size()
{
	return (sizeof(t_smt) - 1);
}

/**
 * it appears that grabstackstr() will barf such alignments
 * because stalloc() will return a string allocated in a new stackblock
 */
static inline size_t	shell_align(size_t nbytes)
{
	return (nbytes + SHELL_SIZE & ~SHELL_SIZE)
}

#endif