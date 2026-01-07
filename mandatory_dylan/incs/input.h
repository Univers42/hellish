/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 22:48:23 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 22:48:23 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INPUT_H
# define INPUT_H

# include "common.h"

typedef enum e_input_method
{
	INPUT_READLINE,
	INPUT_FILE,
	INPUT_ARG,
	INPUT_STDIN_NOTTY
}	t_input_method;

/* Prompt generation moved to input.c */

#endif