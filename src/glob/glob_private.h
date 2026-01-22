/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_private.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 10:18:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/22 10:42:09 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef GLOB_PRIVATE_H
#define GLOB_PRIVATE_H

#include "glob.h"
#include "shell.h"
#include "lexer.h"
#include "expander.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

/* struct describing POSIX classes */
struct s_classes
{
	const char *pattern;
	int plen;
	const char *chars;
};

/* single-definition: actual array provided in glob_classes.c */
extern const struct s_classes classes[];

#endif
