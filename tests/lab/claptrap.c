/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   claptrap.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 14:26:07 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 17:55:29 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../vendor/libft/libft.h"

/*
 Simple test for claptrap:
 - some immediate prints via printf
 - several claptrap() calls (they should be buffered)
 - final claptrap(CLAP_PRINT, "") flushes and frees the buffer
*/
int	main(void)
{
	printf("Start: immediate output\n");
	claptrap(CLAP_INFO, "info: initialization step %d", 1);
	claptrap(CLAP_WARNING, "warn: low disk space (%d%%)", 12);
	claptrap(CLAP_SUCCESS, "ok: module %s loaded", "auth");
	printf("Mid: still immediate\n");
	claptrap(CLAP_PRINT, "");
	printf("End: after flush\n");
	return (0);
}
