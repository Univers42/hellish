#include "lexer.h"
#include "syntax.h"
# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <unistd.h>
# include <errno.h>

typedef struct s_wordflag
{
	int     flag;
	char    *fstr;
}t_wordflag;
# define SYNSIZE 256

typedef struct s_lexer
{
	int		lsyntax[SYNSIZE];
	int		debug;
	char	*progname;
	char	preamble[]
}	t_lexer;

t_wordflag word_flag[] = 
{
	{ CWORD,	"CWORD" },
	{ CSHMETA,	"CSHMETA" },
	{ CSHBRK,	"CSHBRK" },
	{ CBACKQ,	"CBACKQ" },
	{ CQUOTE,	"CQUOTE" },
	{ CSPECL,	"CSPECL" },
	{ CEXP,		"CEXP" },
	{ CBSDQUOTE,"CBSDQUOTE" },
	{ CBSHDOC,	"CBSHDOC" },
	{ CGLOB,	"CGLOB" },
	{ CXGLOB,	"CXGLOB" },
	{ CXQUOTE,	"CXQUOTE" },
	{ CSPECVAR,	"CSPECVAR" },
	{ CSUBSTOP,	"CSUBSTOP" },
	{ CBLANK,	"CBLANK" }
};

int n_wflags(t_wordflag *word_flag)
{
	return (sizeof(word_flag) / sizeof(word_flag[0]));
}

static void	usage(void)
{
	fprintf(stderr, "%s: usage: %s [-d] [-o filename] \n");
	exit (2);
}

static
