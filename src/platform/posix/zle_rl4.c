/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle_rl4.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "zle.h"
#include "helpers.h"
#include <readline/readline.h>
#include <unistd.h>

/* `zle -M` -- the message zsh prints BELOW the line being edited (#77 item
** 5), and the one option of the family readline can honour honestly.
**
** zsh keeps a message area under the prompt and clears it on the next
** keystroke. readline has no such area, so the message is printed on its own
** line and the prompt is redrawn under it: the text lands where the user
** expects to read it, and scrolls away with the rest of the session instead
** of being erased. That is a real difference and it is the harmless
** direction -- a message that stays visible one keystroke too long, rather
** than a message that never appears.
**
** rl_on_new_line() is what tells readline the cursor moved out from under
** it; without it the redisplay repaints over the message it was asked to
** show, which looks exactly like the message never having been printed.
*/
void	zle_do_message(const char *msg)
{
	write_to_file("\n", STDOUT_FILENO);
	if (msg)
		write_to_file((char *)msg, STDOUT_FILENO);
	write_to_file("\n", STDOUT_FILENO);
	rl_on_new_line();
	rl_redisplay();
}
