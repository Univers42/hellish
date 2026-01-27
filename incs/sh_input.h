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

// The header name cannot be change due to conflict name with libft
#ifndef SH_INPUT_H
# define SH_INPUT_H

# include "alias.h"
# include <stdbool.h>
# include <stddef.h>

/* forward declarations */
typedef struct s_shell		t_shell;
typedef struct s_deque_tok	t_deque_tok;

/* input method enum (unique guard to avoid accidental macro collision) */
# ifndef INCS_INPUT_ENUM_DEFINED
#  define INCS_INPUT_ENUM_DEFINED

typedef enum e_metinp
{
	INP_RL,
	INP_FILE,
	INP_ARG,
	INP_NOTTY,
}	t_metinp;
# endif

bool	ends_with_bs_nl(t_string s);
void	extend_bs(t_shell *state);
int		get_more_tokens(t_shell *state, char **prompt, t_deque_tok *tt);
void	parse_and_execute_input(t_shell *state);

#endif