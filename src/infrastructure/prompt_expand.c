/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_expand.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/26 00:30:00 by marvin            #+#    #+#             */
/*   Updated: 2026/09/26 00:30:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "sh_input.h"
#include <stdlib.h>

/* ${var@P}: the value of var rendered as a prompt string, bash's way to
   ask the shell what a prompt looks like (issue #134, section 1).

   It is the PS1 rule and nothing else: the one prompt_normal applies to
   PS1 -- zsh_to_ps1 in the mode the dialect bit selects, then
   ps1_render. So `${PS1@P}` is what the prompt shows, including what it
   does NOT do: a variable's value is not scanned again for escapes, in
   either language (V='100%done' stays 100%done, V='\w' stays \w), which
   is also bash's order. print -P is zsh's and does scan again, as zsh's
   does; a preview of a PS1 theme built on it drew prompts that never
   appear.

   ps1_render, not ps1_animated: the animation cells belong to the live
   prompt, and an @P inside a PS1 or a precmd hook must not rebuild them
   from another string.

   \[ and \] become readline's zero-width markers, \001 and \002. Without
   a line editor bash drops them, so they are stripped here unless the
   read is interactive -- or HELLISH_DBG_PROMPT_MARKS asks for them, the
   window print -P gives the width tests too. Returns a new string. */
char	*prompt_expand_p(t_shell *state, const char *fmt)
{
	t_string	conv;
	t_string	out;
	char		*s;
	size_t		i;
	size_t		j;

	conv = zsh_to_ps1(state, fmt, zsh_mode(state));
	out = ps1_render(state, (char *)conv.ctx);
	xfree(conv.ctx);
	s = (char *)out.ctx;
	if (state->metinp == INP_RL || getenv("HELLISH_DBG_PROMPT_MARKS"))
		return (s);
	i = 0;
	j = 0;
	while (s[i])
	{
		if (s[i] != '\001' && s[i] != '\002')
			s[j++] = s[i];
		i++;
	}
	s[j] = '\0';
	return (s);
}
