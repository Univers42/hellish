/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_ai.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "ai.h"

int	cmd_ai_setup(t_shell *state, char **av, size_t n);

/* Join argv[0..n-1] with single spaces into a fresh string. */
static char	*join_args(char **av, size_t n)
{
	char	*out;
	char	*tmp;
	size_t	i;

	if (n == 0)
		return (ft_strdup(""));
	out = ft_strdup(av[0]);
	i = 1;
	while (i < n)
	{
		tmp = ft_strjoin(out, " ");
		xfree(out);
		out = ft_strjoin(tmp, av[i++]);
		xfree(tmp);
	}
	return (out);
}

/* `ai status`: toggle state + the provider chain (cloud primary -> local). */
static int	cmd_status(t_shell *state)
{
	char	*url;

	ai_sync_env(state);
	if (state->opt_ai)
		ft_printf("ai: on\n");
	else
		ft_printf("ai: off\n");
	url = getenv("HELLISH_AI_URL");
	if (url && *url)
		ft_printf("primary: %s\n", url);
	else
		ft_printf("primary: none (set HELLISH_AI_URL for cloud, e.g. Groq)\n");
	if (ai_reachable() == 0)
		ft_printf("local:   %s:%d (up)\n", ai_host(), ai_port());
	else
		ft_printf("local:   %s:%d (down -- run: make ai-up)\n",
			ai_host(), ai_port());
	return (0);
}

/* Send `prompt` (consumed) and print the reply. as_cmd wraps it so the model
   returns just a shell command (`ai do`) instead of prose (`ai ask`). Always
   allowed: the opt_ai toggle gates the passive hooks, not explicit asks. */
static int	run_chat(t_shell *state, char *prompt, int as_cmd)
{
	char	*full;
	char	*reply;

	full = prompt;
	if (as_cmd)
	{
		full = ft_strjoin("Output ONLY one shell command, no prose, for: ",
				prompt);
		xfree(prompt);
	}
	reply = ai_chat(state, full);
	xfree(full);
	if (!reply)
		return (ft_eprintf("ai: no response (is the server up?)\n"), 1);
	if (as_cmd)
		ai_strip_fence(reply);
	ft_printf("%s\n", reply);
	xfree(reply);
	return (0);
}

/* `ai model [name]`: set the active model directly, or open the fuzzy picker
   over the server's /v1/models list. */
static int	cmd_model(t_shell *state, char **av, size_t n)
{
	char	**list;
	int		cnt;
	int		i;

	if (n > 0)
	{
		xfree(state->ai_model);
		state->ai_model = ft_strdup(av[0]);
		setenv("HELLISH_AI_MODEL", state->ai_model, 1);
		return (ft_printf("ai: model = %s\n", state->ai_model), 0);
	}
	list = ai_models(&cnt);
	if (!list || cnt == 0)
		return (ai_models_free(list, cnt),
			ft_eprintf("ai: no models (is the server up?)\n"), 1);
	i = ai_select(list, cnt, "select model");
	if (i >= 0)
	{
		xfree(state->ai_model);
		state->ai_model = ft_strdup(list[i]);
		setenv("HELLISH_AI_MODEL", state->ai_model, 1);
		ft_printf("ai: model = %s\n", state->ai_model);
	}
	return (ai_models_free(list, cnt), 0);
}

/* ai [status|on|off|model [name]|ask <text>]. on/off mirror `set -o ai`. */
int	builtin_ai(t_shell *state, t_vec argv)
{
	char	**av;

	av = (char **)argv.ctx;
	if (argv.len < 2 || ft_strcmp(av[1], "status") == 0)
		return (cmd_status(state));
	if (ft_strcmp(av[1], "on") == 0)
		return (state->opt_ai = true, 0);
	if (ft_strcmp(av[1], "off") == 0)
		return (state->opt_ai = false, 0);
	if (ft_strcmp(av[1], "model") == 0)
		return (cmd_model(state, av + 2, argv.len - 2));
	if (ft_strcmp(av[1], "setup") == 0)
		return (cmd_ai_setup(state, av + 2, argv.len - 2));
	if (ft_strcmp(av[1], "ask") == 0)
		return (run_chat(state, join_args(av + 2, argv.len - 2), 0));
	if (ft_strcmp(av[1], "do") == 0)
		return (run_chat(state, join_args(av + 2, argv.len - 2), 1));
	return (run_chat(state, join_args(av + 1, argv.len - 1), 0));
}
