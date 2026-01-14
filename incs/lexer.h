/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lexer.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 02:20:12 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 02:20:12 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LEXER_H
# define LEXER_H

struct s_shell; // <-- add this forward declaration

# include "libft.h"
# include "token.h"

typedef struct s_deque_tt
{
	t_deque	deqtok;	// void *buff ==> t_token 
	char		looking_for;
}	t_deque_tt;

typedef struct s_op_map
{
	char	*str;
	t_tt	t;
}	t_op_map;



char		*tokenizer(char *str, t_deque_tt *ret);
int			advance_dquoted(char **str);
int			advance_squoted(char **str);
void		free_all_state(struct s_shell *state);
void	print_tokens(t_deque_tt *tokens);
char	*tt_to_str(t_tt tt);
char	*tt_to_str_p2(t_tt tt);
bool	is_special_char(char c);
bool	is_space(char c);


#endif