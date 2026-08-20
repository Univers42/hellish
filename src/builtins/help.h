/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   help.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/19 20:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#ifndef HELP_H
# define HELP_H

# include "libft.h"

/* One entry in the help index.  `group` drives the grouped listing, and is
   also what makes the listing skimmable: bash prints one alphabetical wall
   of 60 entries, which tells you nothing about which of them you want. */
typedef struct s_help
{
	const char	*name;
	const char	*group;
	const char	*synopsis;
	const char	*summary;
}	t_help;

/* The whole index, NULL-terminated.  Split across two files only to keep
   each one a readable length; help_index() stitches nothing together, the
   second half is simply appended by the linker-visible accessor. */
const t_help	*help_index(void);
const t_help	*help_index2(void);

/* Look up one topic by exact name.  NULL when there is no such topic. */
const t_help	*help_find(const char *name);

/* Print the grouped listing (no arguments) and one topic's detail. */
void			help_print_list(void);
void			help_print_one(const t_help *e, int synopsis_only);

#endif
