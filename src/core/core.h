/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 00:01:28 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 00:01:28 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Private header for src/core/: wires together init, option-parsing, and
   startup helpers. Nothing outside core/ should include this.
   The duplicate prototypes (init_arg, init_file, init_stdin_notty) are a
   build artefact; C allows repeated identical prototypes, but a cleanup
   pass could remove them. */
#ifndef CORE_H
# define CORE_H

# include "shell.h"
# include "shell.h"
# include "helpers.h"
# include "sh_input.h"
# include "env.h"
# include <string.h>
# include "lexer.h"
# include <fcntl.h>
# include <errno.h>
# include <unistd.h>

/* One command-line parse in flight.  cli_scan walks argv, applies every
   option to `state`, and leaves `i` pointing at the first operand (the -c
   command string, the script file, or past the end).  cmode/inter record
   -c/-i; err is 2 on an unrecognised option (bash's usage-error status). */
typedef struct s_cli
{
	char	**argv;
	int		i;
	bool	cmode;
	bool	inter;
	int		err;
}	t_cli;

void		cli_parse(t_shell *state, char **argv, t_cli *cli);
void		cli_scan(t_shell *state, t_cli *cli);
void		cli_dispatch(t_shell *state, t_cli *cli);
int			cli_opt_word(t_shell *state, t_cli *cli, const char *w);
void		cli_long_word(t_shell *state, t_cli *cli, const char *w);
void		cli_lone_dash(t_shell *state, t_cli *cli);
bool		cli_known_short(char c);
int			cli_apply_long(t_shell *state, char sign, const char *name);
void		cli_apply_short(t_shell *state, char sign, char c);
int			cli_take_o(t_shell *state, t_cli *cli, char sign, int idx);
void		apply_flag_word(t_shell *state, const char *w);
int			set_long_option(t_shell *state, char sign, const char *name);
void		init_arg(t_shell *state, char **argv);
void		init_file(t_shell *state, char **argv);
void		init_stdin_notty(t_shell *state);
void		init_cwd(t_shell *state);
void		ensure_essential_env_vars(t_shell *state);
void		handle_file_open_error(t_shell *state, char **argv);
void		read_file_to_buffer(int fd, t_shell *state);
char		*read_file(const char *path);
void		source_profile(t_shell *state);
void		update_ctx_from_file(t_shell *state, char **argv);
#endif
