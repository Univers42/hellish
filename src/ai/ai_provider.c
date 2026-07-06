/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_provider.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"
#include <stdlib.h>

/* Explicit override via HELLISH_AI_PROVIDER (local|openai|anthropic|claude). */
static int	provider_env(const char *p)
{
	if (!ft_strcmp(p, "anthropic") || !ft_strcmp(p, "claude"))
		return (AI_ANTHROPIC);
	if (!ft_strcmp(p, "local"))
		return (AI_LOCAL);
	return (AI_OPENAI);
}

/* Pick the backend protocol. HELLISH_AI_PROVIDER wins; otherwise infer from the
   endpoint: an api.anthropic.com URL is the Messages API, any other URL is
   OpenAI-compatible, and no URL means the local llama server. */
int	ai_provider(const char *url)
{
	char	*p;

	p = getenv("HELLISH_AI_PROVIDER");
	if (p && *p)
		return (provider_env(p));
	if (!url || !*url)
		return (AI_LOCAL);
	if (ft_strnstr(url, "anthropic.com", ft_strlen(url)))
		return (AI_ANTHROPIC);
	return (AI_OPENAI);
}

/* The JSON key holding the assistant text: Anthropic returns content blocks
   ("text"), OpenAI-compatible returns a message "content" string. */
char	*ai_resp_key(int provider)
{
	if (provider == AI_ANTHROPIC)
		return ("text");
	return ("content");
}

/* Task-tuned system prompt. Completion (as_cmd) must return a bare runnable
   command continuation; the assistant mode explains and advises. Both are told
   to lean on the shell context the request carries. */
char	*ai_sys_prompt(int as_cmd)
{
	if (as_cmd)
		return ("You are a command-line completion engine for a POSIX shell. "
			"Using the shell context and the partial command, reply with ONLY "
			"the single most likely complete command line as raw text: no "
			"explanation, no markdown, no backticks, no quoting. Your reply "
			"MUST begin with the user's exact partial text.");
	return ("You are hellish, an expert POSIX shell assistant in the user's "
		"terminal. Use the shell context (cwd, git branch, recent commands, "
		"files) to infer intent. Answer concisely and correctly; when asked "
		"for a command, output just the command, no prose.");
}

/* Fill hdrs[] with the endpoint's auth headers. Anthropic authenticates with
   x-api-key plus a pinned API version; OpenAI-compatible endpoints use a Bearer
   token. No key => no auth header (a local server needs none). */
void	ai_auth_headers(const char *url, const char *key, char *auth,
		char **hdrs)
{
	hdrs[0] = NULL;
	hdrs[1] = NULL;
	hdrs[2] = NULL;
	if (!key || !*key)
		return ;
	if (ai_provider(url) == AI_ANTHROPIC)
	{
		ft_strlcpy(auth, "x-api-key: ", AI_AUTH_CAP);
		ft_strlcat(auth, key, AI_AUTH_CAP);
		hdrs[0] = auth;
		hdrs[1] = "anthropic-version: 2023-06-01";
		return ;
	}
	ft_strlcpy(auth, "Authorization: Bearer ", AI_AUTH_CAP);
	ft_strlcat(auth, key, AI_AUTH_CAP);
	hdrs[0] = auth;
}
