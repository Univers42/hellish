/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   common.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 17:05:04 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 17:05:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COMMON_H
# define COMMON_H

// standard libraries
# include <unistd.h>
# include <stdio.h>
# include <ctype.h>
# include <string.h>
# include <stdlib.h>
# include <stdbool.h>
# include <stddef.h>
# include <dirent.h>
# include <errno.h>

// Customized libraries
# include "dsa.h"
# include "utils.h"
# include "libft/libft.h"
# include "sigs.h"
# include "ft_printf/ft_printf.h"


typedef t_vec	t_string;		// a single dynamic string
typedef t_vec	t_obj_str;		// elements objects of string
typedef t_vec	t_strings;		// array of C-string pointers (references)
typedef t_vec	t_env;
typedef t_vec	t_ints;

/**
 * For now exit state but can have other purposes
 */
typedef enum s_state
{
	/* success */
	ST_OK = 0,
	/* base failure code */
	ST_BASE_ERR = 1,
	ST_WAIT,
	ST_INIT,
	ST_MORE_INPUT,
	ST_FATAL_ERR
}	t_state;


#endif