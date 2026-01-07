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

/* standard libraries */
# include <unistd.h>
# include <stdio.h>
# include <ctype.h>
# include <string.h>
# include <stdlib.h>
# include <stdbool.h>
# include <stddef.h>
# include <dirent.h>
# include <errno.h>

/* For now exit state but can have other purposes */
typedef enum s_state
{
	/* success */
	ST_OK = 0,
	/* base failure code */
	ST_BASE_ERR = 1,
	ST_SCANNING,
	ST_WAIT,
	ST_INIT,
	ST_MORE_INPUT,
	ST_FATAL_ERR
}	t_state;

/* Customized libraries: utils must come first so helpers/types are available */
# include "utils.h"
# include "ft_printf/ft_printf.h"
# include "dsa.h"
# include "sigs.h"



#endif