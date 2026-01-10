/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   common.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 01:39:25 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 01:39:25 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COMMON_H
# define COMMON_H

/**
 * Common group of libraries we use globally in all the project. it's too
 * simplify the callings and avoid circular dependencies..
 * or worst redundancies due to lack of mental mapping.
 */

# include "libft.h"
# include <dirent.h>
# include <stdbool.h>
# include <stdlib.h>
# include <stdbool.h>
# include <string.h>
# include <stdlib.h>
# include <unistd.h>
# include <sys/types.h>
# include <errno.h>
# include <fcntl.h>
# include <stdio.h>
# include <signal.h>
# include "alias.h"

#endif