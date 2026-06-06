/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_files.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* File completion: delegate entirely to readline's built-in filename
   generator.  We just clear the append char (no trailing space after a
   directory completion -- readline appends '/' instead automatically). */

#include "libft.h"
#include <readline/readline.h>

/* Hand off to rl_filename_completion_function, which handles tilde
   expansion, hidden files, and directory slash appending for us. */
char	**complete_files(const char *text, int start, int end)
{
	(void)start;
	(void)end;
	rl_completion_append_character = '\0';
	return (rl_completion_matches(text, rl_filename_completion_function));
}
