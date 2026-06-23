/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_api.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"

/* One-shot chat (ai ask / ai do / the pro-tip worker). `state` is unused now --
   context is gathered state-free -- but kept so existing callers don't change.
   Returns the assistant text (xfree it) or NULL. */
char	*ai_chat(t_shell *state, const char *user_msg)
{
	ai_sync_env(state);
	return (ai_request(user_msg, 0));
}

/* Inline command completion for the readline keybinding. Frames the partial
   line as a completion task and asks for a bare command (fence-stripped). */
char	*ai_complete_line(const char *line)
{
	char	*prompt;
	char	*out;

	prompt = ft_strjoin("The user is typing a shell command. Output the single "
			"most likely full command, beginning with EXACTLY this text and "
			"nothing before it; only the command: ", line);
	out = ai_request(prompt, 1);
	xfree(prompt);
	return (out);
}
