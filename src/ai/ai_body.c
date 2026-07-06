/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai_body.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ai.h"

/* Append the JSON-escaped form of `val` (no surrounding quotes) to buf. */
static void	app_esc(char *buf, size_t cap, const char *val)
{
	char	*e;

	e = ai_json_escape(val);
	ft_strlcat(buf, e, cap);
	xfree(e);
}

/* Decoding params, tuned per task. A completion is one command line: cap it
   small, stop at the first newline, and decode greedily (temperature 0) --
   fewer tokens is directly faster on a CPU backend and more precise. A chat
   answer keeps room to explain, but stays bounded: on a CPU backend every
   generated token is a CPU burst that competes with the user's real work.
   (Anthropic spells stop "stop_sequences".) */
static void	app_params(char *b, size_t cap, int provider, int as_cmd)
{
	if (!as_cmd)
	{
		ft_strlcat(b, ",\"max_tokens\":256", cap);
		return ;
	}
	ft_strlcat(b, ",\"max_tokens\":64,\"temperature\":0", cap);
	if (provider == AI_ANTHROPIC)
		ft_strlcat(b, ",\"stop_sequences\":[\"\\n\"]", cap);
	else
		ft_strlcat(b, ",\"stop\":[\"\\n\"]", cap);
}

/* OpenAI-compatible chat body: a system message then the user message. The
   model is injected only when set (so a local llama-server uses whatever it
   loaded); every interpolated value is escaped, including the model. */
static char	*body_openai(const char *model, int as_cmd, const char *user)
{
	char	*b;
	size_t	cap;

	cap = 16384;
	b = xmalloc(cap);
	ft_strlcpy(b, "{\"messages\":[{\"role\":\"system\",\"content\":\"", cap);
	app_esc(b, cap, ai_sys_prompt(as_cmd));
	ft_strlcat(b, "\"},{\"role\":\"user\",\"content\":\"", cap);
	app_esc(b, cap, user);
	ft_strlcat(b, "\"}]", cap);
	if (model && *model)
	{
		ft_strlcat(b, ",\"model\":\"", cap);
		app_esc(b, cap, model);
		ft_strlcat(b, "\"", cap);
	}
	app_params(b, cap, AI_OPENAI, as_cmd);
	ft_strlcat(b, ",\"stream\":false}", cap);
	return (b);
}

/* Anthropic Messages body: model + required max_tokens (via app_params), a
   top-level system field, and the user message. Falls back to a small default
   model so the request still validates when HELLISH_AI_MODEL is unset. */
static char	*body_anthropic(const char *model, int as_cmd, const char *user)
{
	char	*b;
	size_t	cap;

	cap = 16384;
	b = xmalloc(cap);
	ft_strlcpy(b, "{\"model\":\"", cap);
	if (model && *model)
		app_esc(b, cap, model);
	else
		ft_strlcat(b, "claude-3-5-haiku-latest", cap);
	ft_strlcat(b, "\"", cap);
	app_params(b, cap, AI_ANTHROPIC, as_cmd);
	ft_strlcat(b, ",\"system\":\"", cap);
	app_esc(b, cap, ai_sys_prompt(as_cmd));
	ft_strlcat(b, "\",\"messages\":[{\"role\":\"user\",\"content\":\"", cap);
	app_esc(b, cap, user);
	ft_strlcat(b, "\"}]}", cap);
	return (b);
}

/* Build the request body in the shape the provider expects, with the system
   prompt and decoding params tuned to the task (as_cmd = completion). */
char	*ai_body(int provider, const char *model, int as_cmd, const char *user)
{
	if (provider == AI_ANTHROPIC)
		return (body_anthropic(model, as_cmd, user));
	return (body_openai(model, as_cmd, user));
}
