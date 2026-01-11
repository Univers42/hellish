/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:16:41 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:16:41 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifdef INPUT_H
# define INPUT_H

# include "libft.h"
# include "shell.h"
# include "../lexer/lexer.h"

/* avoid redefining enum if already provided by incs/shell.h */
# ifndef INPUT_ENUM_DEFINED
#  define INPUT_ENUM_DEFINED
typedef enum e_input_method
{
	INP_READLINE,
	INP_FILE,
	INP_ARG,
	INP_STDIN_NOTTY,
}	t_input_method;
# endif

bool	ends_with_bs_nl(t_string s);
void	extend_bs(t_shell *state);
void	get_more_tokens(t_shell *state, char **prompt, t_deque_tt *tt);
void	parse_and_execute_input(t_shell *state);

# endif