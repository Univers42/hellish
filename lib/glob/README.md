# Glob lib
This lib contain directories and files that specify globbing and pattern matching subsystem. 

> globbing referes to filename pattern matching using wildcards like `*`, `?`, and bracket expresions [abc]

# Features
- Standard globbing: Wildcars `*`, `?`), bracket expressions
- Extended globbing: ksh-like patterns @(foo|bar), !foo, +(foo), etc..
- Multibyte/Unicode support: Handles wilde character
- POSIX compliance: supports POSIX bracket expresssion like [[:alpha:]], [.newline.],[=a=]
- Case folding: Optional case-insensitive matching.
- Directory-specific matching: Flags to match only directories, mark directories, etc..
- Recursive globiing: support for `**` to match files recursively

# HOW IT WORKS
The globbing engine scans directories, matches filenames against patterns using the matching engin and return lists
of matches

# Why it's a lib ?
it implements filename pattern matching (globbing) using wildcard and extended patterns, which is  acommon need in shell
and file utilisies.
- The code is modular and portable, so it can be reused in any other porjets of (shells, scripting langues, file managers)
that need globbing at the end of the day. 

> It's adaptability make it adaptable and completeness make it useful for other porjects beyond shell !c

# COLLSYMS.h
- [x] collsym
- [x] posixcoll

# glob.c

- [ ]glob_pattern
- [ ]extglob_skipname
- [ ]skipname
- [ ]udequote_pathname
- [ ]glob test_dir
- [ ]finddirs
- [ ]glob_vector
- [ ]glob_dir_to_array
- []glob_filename

# glob_loop.c
- [ ]Internal glob pattern p

# gm_loop.c
- [ ]EXTGLOB_PATTERN_P
- [ ]MATCH_PATTER_CHAR
- [ ]MATCHLEN
- [ ]
- [ ]
- [ ]
- [ ]

# gmisc.c
- [ ]glob_dirscsan


# sm_loop.c
- [ ]FCT
- [ ]GMATCH
- [ ]SLASH_PATHNAME
- [ ]PARSE_SUBBRACET
- [ ]BRACKMATCH
- [ ]PATSCAN
- [ ]STRCOMPARE
- [ ]EXTMATCH
- [ ]

# smatch.c
- [ ]fnmatch_fallback
- [ ]fnmatch
- [ ]charcmp
- [ ]rangecmp
- [ ]collseqcmp
- [ ]collsym
- [ ]char_class
- [ ]is_valid_cclass
- [ ]cclass_test
- [ ]is_cclass
- [ ] FOLD
- [ ] ISDIRSEP
- [ ] PATHSEP
- [ ] PDOT_OT_DOTDOT
- [ ] SDOT_OR_DOTDOT
- [ ] FNMATCH_EQUIV_FALLBACK
- [ ] fnmatch_fallback_wc
- [ ] charcmp_wc
- [ ] rangecmp_wc
- [ ] collseqcmp_wc
- [ ] collwcsym
- [ ] is_wcclass
- [ ] posix_cclass_only
## map of fns
```c
#define FCT			internal_wstrmatch
#define GMATCH			gmatch_wc
#define COLLSYM			collwcsym
#define PARSE_SUBBRACKET	parse_subbracket_wc
#define BRACKMATCH		brackmatch_wc
#define PATSCAN			glob_patscan_wc
#define STRCOMPARE		wscompare
#define EXTMATCH		extmatch_wc
#define DEQUOTE_PATHNAME	wcdequote_pathname
#define STRUCT			wcsmat_struct
#define STRCHR(S, C)		wcschr((S), (C))
#define MEMCHR(S, C, N)		wmemchr((S), (C), (N))
#define STRCOLL(S1, S2)		wcscoll((S1), (S2))
#define STRLEN(S)		wcslen(S)
#define STRCMP(S1, S2)		wcscmp((S1), (S2))
#define RANGECMP(C1, C2, F)	rangecmp_wc((C1), (C2), (F))
#define COLLEQUIV(C1, C2)	collseqcmp_wc((C1), (C2))
#define CTYPE_T			enum char_class
#define IS_CCLASS(C, S)		is_wcclass((C), (S))
#include "sm_loop.c"
```
- [ ] xstrmatch

# strmatch.c
- [ ] strmatch
- [ ] wcstrmatch

# xmbstrtowcs.h
- [ ] xmbsrtowcs
- [ ] xdumpstowcs2
- [ ] xdupmbstowcs
- [ ] xwcsrtombs
