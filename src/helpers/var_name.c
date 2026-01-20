/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   var_name.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 14:25:44 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 14:25:44 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdbool.h>
#include <ctype.h>

// Returns true if c is a valid first character for a shell variable name
bool	is_var_name_p1(char c)
{
	return (isalpha((unsigned char)c) || c == '_');
}

// Returns true if c is a valid subsequent character for a shell variable name
bool	is_var_name_p2(char c)
{
	return (isalnum((unsigned char)c) || c == '_');
}
