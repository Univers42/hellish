/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   help_data.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/19 20:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "help.h"

/* First half of the help index: navigation, output, variables, jobs.

   Entries carry a group, and `help` with no argument prints them grouped
   rather than as one alphabetical wall. bash sorts sixty entries by first
   letter, which answers "what starts with f" -- a question nobody has. The
   question a newcomer actually has is "what can I do about jobs", and a
   grouped listing answers that one. */
static const t_help	g_help_1[] = {
{"cd", "navigation",
	"cd [-L|-P] [dir] | cd - | cd old new",
	"change directory (honours CDPATH, - goes back)"},
{"pwd", "navigation",
	"pwd [-L|-P]",
	"print the working directory"},
{"pushd", "navigation",
	"pushd [dir | +N | -N] [-n]",
	"push a directory on the stack and cd there"},
{"popd", "navigation",
	"popd [+N | -N] [-n]",
	"pop the directory stack and cd there"},
{"echo", "output",
	"echo [-neE] [arg ...]",
	"write arguments (-n no newline, -e escapes)"},
{"printf", "output",
	"printf [-v var] format [arg ...]",
	"format and print, like printf(3)"},
{"read", "output",
	"read [-r] [-p prompt] [-n n] [-t sec] [name ...]",
	"read one line into variables"},
{"mapfile", "output",
	"mapfile [-t] [-n n] [-O i] [-s n] [-u fd] [array]",
	"read lines of input into an array"},
{"readarray", "output",
	"readarray [-t] [-n n] [-O i] [-s n] [array]",
	"same as mapfile"},
{"umask", "output",
	"umask [-S] [-p] [mode]",
	"show or set the file-creation mask"},
{"export", "variables",
	"export [-p] [-n] [name[=value] ...]",
	"put variables into the environment of commands"},
{"readonly", "variables",
	"readonly [-p] [name[=value] ...]",
	"make variables unassignable and unremovable"},
{"unset", "variables",
	"unset [-f|-v] [name ...]",
	"remove variables or functions"},
{"declare", "variables",
	"declare [-aAfFgiIlnrtux] [-p] [name[=value] ...]",
	"declare variables and give them attributes"},
{"typeset", "variables",
	"typeset [-aAfFgiIlnrtux] [-p] [name[=value]...]",
	"same as declare"},
{"local", "variables",
	"local [name[=value] ...]",
	"declare variables local to a function"},
{"let", "variables",
	"let arg [arg ...]",
	"evaluate arithmetic; status 1 if the last is 0"},
{"getopts", "variables",
	"getopts optstring name [arg ...]",
	"parse option arguments in a loop"},
{"jobs", "jobs",
	"jobs [-l] [-p] [jobspec ...]",
	"list background jobs"},
{"fg", "jobs",
	"fg [jobspec]",
	"bring a job to the foreground"},
{"bg", "jobs",
	"bg [jobspec ...]",
	"resume a stopped job in the background"},
{"kill", "jobs",
	"kill [-s sig|-n num|-sig] pid|job ... | kill -l",
	"send a signal to a process or job"},
{"wait", "jobs",
	"wait [-n] [id ...]",
	"wait for background jobs to finish"},
{NULL, NULL, NULL, NULL}
};

/* The first half of the index; help_find and the listing walk both halves. */
const t_help	*help_index(void)
{
	return (g_help_1);
}
