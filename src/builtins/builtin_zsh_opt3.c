/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_opt3.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Every option zsh 5.9 has, by its normalised name (lower case, no
** underscores, the `no` prefix folded away where it is one).
**
** zopt_inert listed the options plugins commonly touch, and anything not
** on it got "setopt: no such option" -- true for a typo, false for the
** hundred-odd real options nobody had typed yet.  An oh-my-zsh lib file
** or a pasted ~/.zshrc sets `unsetopt beep`, `setopt hist_expire_dups_first
** flowcontrol long_list_jobs`, and each line printed an error at every
** shell start although zsh accepts every one of them.  The roster is the
** whole set, generated from `zsh -c 'set -o'` and split in four only for
** the norm's line budget, so the remaining message means what it says:
** zsh would not know the name either.  Behaviour is unchanged -- the
** options this shell implements are still mapped in zopt_apply first;
** everything here is accepted and does nothing. */
static bool	roster1(const char *n)
{
	static const char	*t[] = {
		"aliases", "aliasfuncdef", "allexport", "alwayslastprompt",
		"alwaystoend", "appendcreate", "appendhistory", "autocd",
		"autocontinue", "autolist", "automenu", "autonamedirs",
		"autoparamkeys", "autoparamslash", "autopushd", "autoremoveslash",
		"autoresume", "badpattern", "banghist", "bareglobqual",
		"bashautolist", "bashrematch", "beep", "bgnice", "braceccl",
		"bsdecho", "caseglob", "casematch", "cbases", "cdablevars",
		"cdsilent", "chasedots", "chaselinks", "checkjobs",
		"checkrunningjobs", "combiningchars", "completealiases",
		"completeinword", "continueonerror", "correct", "correctall",
		"cprecedences", "cshjunkiehistory", "cshjunkieloops",
		"cshjunkiequotes", "cshnullcmd", "cshnullglob", "debugbeforecmd",
		"dvorak", "emacs", "equals", "errexit", "errreturn", "evallineno",
		"extendedglob", "extendedhistory", NULL};
	int					i;

	i = -1;
	while (t[++i])
		if (!ft_strcmp(t[i], n))
			return (true);
	return (false);
}

static bool	roster2(const char *n)
{
	static const char	*t[] = {
		"flowcontrol", "forcefloat", "functionargzero", "globalexport",
		"globalrcs", "globassign", "globcomplete", "globdots",
		"globstarshort", "globsubst", "hashcmds", "hashdirs",
		"hashexecutablesonly", "hashlistall", "histallowclobber", "histbeep",
		"histexpiredupsfirst", "histfcntllock", "histfindnodups",
		"histignorealldups", "histignoredups", "histignorespace",
		"histlexwords", "histnofunctions", "histnostore", "histreduceblanks",
		"histsavebycopy", "histsavenodups", "histsubstpattern", "histverify",
		"hup", "ignorebraces", "ignoreclosebraces", "ignoreeof",
		"incappendhistory", "incappendhistorytime", "interactive",
		"interactivecomments", "ksharrays", "kshautoload", "kshglob",
		"kshoptionprint", "kshtypeset", "kshzerosubscript", "listambiguous",
		"listbeep", "listpacked", "listrowsfirst", "listtypes", "localloops",
		"localoptions", "localpatterns", "localtraps", "login",
		"longlistjobs", "magicequalsubst", NULL};
	int					i;

	i = -1;
	while (t[++i])
		if (!ft_strcmp(t[i], n))
			return (true);
	return (false);
}

static bool	roster3(const char *n)
{
	static const char	*t[] = {
		"mailwarning", "markdirs", "menucomplete", "monitor", "multibyte",
		"multifuncdef", "multios", "noaliases", "noalwayslastprompt",
		"noappendhistory", "noautolist", "noautomenu", "noautoparamkeys",
		"noautoparamslash", "noautoremoveslash", "nobadpattern", "nobanghist",
		"nobareglobqual", "nobeep", "nobgnice", "nocaseglob", "nocasematch",
		"nocheckjobs", "nocheckrunningjobs", "noclobber", "nodebugbeforecmd",
		"noequals", "noevallineno", "noexec", "noflowcontrol",
		"nofunctionargzero", "noglob", "noglobalexport", "noglobalrcs",
		"nohashcmds", "nohashdirs", "nohashlistall", "nohistbeep",
		"nohistsavebycopy", "nohup", "nolistambiguous", "nolistbeep",
		"nolisttypes", "nomatch", "nomultibyte", "nomultifuncdef",
		"nomultios", "nonomatch", "nonotify", "nopromptcr", "nopromptpercent",
		"nopromptsp", "norcs", "noshortloops", "notify", "nounset", NULL};
	int					i;

	i = -1;
	while (t[++i])
		if (!ft_strcmp(t[i], n))
			return (true);
	return (false);
}

static bool	roster4(const char *n)
{
	static const char	*t[] = {
		"nullglob", "numericglobsort", "octalzeroes", "overstrike",
		"pathdirs", "pathscript", "pipefail", "posixaliases", "posixargzero",
		"posixbuiltins", "posixcd", "posixidentifiers", "posixjobs",
		"posixstrings", "posixtraps", "printeightbit", "printexitvalue",
		"privileged", "promptbang", "promptcr", "promptpercent", "promptsp",
		"promptsubst", "pushdignoredups", "pushdminus", "pushdsilent",
		"pushdtohome", "rcexpandparam", "rcquotes", "rcs", "recexact",
		"rematchpcre", "restricted", "rmstarsilent", "rmstarwait",
		"sharehistory", "shfileexpansion", "shglob", "shinstdin", "shnullcmd",
		"shoptionletters", "shortloops", "shwordsplit", "singlecommand",
		"singlelinezle", "sourcetrace", "sunkeyboardhack", "transientrprompt",
		"trapsasync", "typesetsilent", "verbose", "vi", "warncreateglobal",
		"warnnestedvar", "xtrace", "zle", NULL};
	int					i;

	i = -1;
	while (t[++i])
		if (!ft_strcmp(t[i], n))
			return (true);
	return (false);
}

bool	zopt_roster(const char *n)
{
	return (roster1(n) || roster2(n) || roster3(n) || roster4(n));
}
