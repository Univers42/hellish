/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 15:01:32 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 15:02:31 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"

int str_match(char *pattern, char *string, int flags)
{
    if (string == 0 || pattern == 0)
        return (STRMATCH_NO);
    return (xstr_match(pattern, string, flags));
}