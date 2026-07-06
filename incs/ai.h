/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.h                                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef AI_H
# define AI_H

# include "shell.h"
# include "libft.h"

/* 256 KB cap on a reply (max_tokens keeps responses far smaller). */
# define AI_MAX_REPLY 262144

/* Backing size for a single auth-header string (scheme + API key). */
# define AI_AUTH_CAP 600

/* Backend wire protocols. LOCAL/OPENAI share the OpenAI chat-completions shape;
   ANTHROPIC is the Messages API (top-level system, x-api-key, "text" replies).
   Chosen by ai_provider() from HELLISH_AI_PROVIDER or the endpoint URL. */
enum e_ai_provider
{
	AI_LOCAL = 0,
	AI_OPENAI,
	AI_ANTHROPIC
};

/* Which backend a URL targets, the JSON reply key it uses, and the task-tuned
   system prompt (as_cmd=1 -> bare-command completion, else assistant). */
int		ai_provider(const char *url);
char	*ai_resp_key(int provider);
char	*ai_sys_prompt(int as_cmd);

/* Build the request body for a provider: system prompt, decoding params, and
   token budget picked by task (as_cmd = inline completion), model injected. */
char	*ai_body(int provider, const char *model, int as_cmd,
			const char *user);

/* Fill hdrs[] (NULL-terminated, room for 2 + NULL) with the auth headers the
   endpoint needs -- Bearer for OpenAI-compatible, x-api-key + version for
   Anthropic. `auth` (>= AI_AUTH_CAP) backs the key header string. */
void	ai_auth_headers(const char *url, const char *key, char *auth,
			char **hdrs);

/* Config, read from the environment on demand (no struct kept on t_shell):
   HELLISH_AI_HOST (default 127.0.0.1), HELLISH_AI_PORT (8080),
   HELLISH_AI_TIMEOUT_MS (20000). Returned host pointer is borrowed. */
char	*ai_host(void);
int		ai_port(void);
int		ai_timeout_ms(void);

/* Mirror the HELLISH_AI_* vars from hellish's env store into the process
   environ so getenv() (and the forked readline child) see exported config. */
void	ai_sync_env(t_shell *state);

/* Per interactive REPL turn: sync the AI env (interactive only) + refresh the
   background pro-tip. Called once per loop turn from repl_shell. */
void	ai_prompt_prep(t_shell *state);

/* TCP connect to host:port with send/recv timeouts. -1 on failure. */
int		ai_connect(const char *host, int port, int timeout_ms);
int		ai_send_all(int fd, const char *buf, size_t len);
char	*ai_read_all(int fd);

/* HTTP/1.1 POST/GET to path; returns the response body (xfree it) or NULL. */
char	*ai_post(const char *path, const char *body, int timeout_ms);
char	*ai_get(const char *path, int timeout_ms);
char	*ai_build_request(const char *path, const char *body);

/* Minimal JSON helpers: escape a value, extract a string value by key. */
char	*ai_json_escape(const char *s);
char	*ai_json_get_str(const char *buf, const char *key);

/* One-shot chat against llama.cpp's OpenAI-compatible endpoint. Returns the
   assistant text (xfree it) or NULL. ai_reachable: 0 if the server answers. */
char	*ai_chat(t_shell *state, const char *user_msg);
int		ai_reachable(void);

/* The core: prepend rich shell context to instruction, then try the provider
   chain (cloud $HELLISH_AI_URL primary -> local fallback). Strips a markdown
   fence when as_cmd. Returns the reply (xfree it) or NULL. */
char	*ai_request(const char *instruction, int as_cmd);

/* Shell-context preamble (os, cwd, last status, git, recent commands, dir).
   as_cmd gets a lite cut -- fewer prompt tokens, faster inference. xfree it. */
char	*ai_context_for(int as_cmd);

/* POST a JSON body to a cloud endpoint via curl (TLS + optional Bearer key).
   Returns the response body (xfree it) or NULL. */
char	*ai_curl(const char *url, const char *key, const char *body, int tmo);

/* Complete a partial command line (state-free; for the readline keybinding).
   Returns the suggested command (xfree it) or NULL. */
char	*ai_complete_line(const char *line);

/* Unwrap a ```-fenced markdown code block to the bare command, in place. */
void	ai_strip_fence(char *s);

/* GET /v1/models and collect the model ids. Returns a fresh char*[] of fresh
   strings (free each + the array) and writes the count; NULL/0 on failure. */
char	**ai_models(int *count);
void	ai_models_free(char **list, int n);

/* Fuzzy picker: arrow keys + type-to-filter over names[0..n). Returns the
   chosen index into names (or -1 on cancel / no TTY). */
int		ai_select(char **names, int n, const char *title);

/* Async pro-tip: ai_tip_spawn refreshes a cached tip in a detached worker
   (throttled, interactive-only); ai_tip_read returns the cached line (xfree
   it) or NULL. The prompt only ever reads -- it never calls the network. */
char	*ai_tip_read(void);
void	ai_tip_spawn(t_shell *state);

/* 1 when the machine is already busy (loadavg vs cores): background AI work
   defers so an inference burst never fights the user's real commands. */
int		ai_load_high(void);

#endif
