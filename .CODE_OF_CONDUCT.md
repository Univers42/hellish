# Principles of developers (workflow to respect)
To be efficient we must agree to respect different rules of organization to set
this project the most quickly and reliably possibly. Thoses framework pass by acknowledging different set of rules.

# Before we even code a line
- [ ] Git Workflow
- [ ] Hooks
- [ ] CI (GitHub Actions)
- [ ] Code Style
- [ ] Commit convention
- [ ] branch protection
- [ ] Docs
- [ ] interactive rebase habits

## Commit

From What I've sawn on the internet, most teams follow some variation of these three pillars:
1. Conventional commits				--> standardized structure
2. Messge length + clarity			-->	"50-char subject, 72 car body lines"
3. One logical change per commit	--> "Easier to review + revet" 


**Wrong practice:**
```c
fix stuff
update
final commit ft this time
```

**Template**:

|	type	|		Meaning					|
|	--		|		--						|
|`feat`		|A new feature					|
|`fix`		|A bug fix						|
|`docs`		|Documentation only				|
|`style`	|Formatting, no logic changes	|
|`refactor`	|Code refactor withot bug/feat	|
|`test`		|Adding/modifying tests			|
|`chore`	|Build process or tooling change|


**Example**

```bash
fix(parser): handle heredoc properly
feat(builtin): add cd command
docs(readme): clarify usage section
```

> Finally never speak to past, use `imperative` phrase
like 
`add parser for heredoc`	-->	✅
`added parser for heredoc`	--> ❌

### One logical change per commit
Each commit should represent one idea: [atomic habit](https://www.amazon.fr/Atomic-Habits-Proven-Build-Break/dp/1847941834)

> if project track issues, add references:
```bash
feat(exec): fix path resolution (#24)

#or use footers
Fixes: #24
```

---
## Push
# Hooks development
To configure and keep a maximum control on environment project. I will create different 
hooks so either me or my colleague don't mess up the codebase ot the cloud repo...

Each hooks from the hidden files [git](./.git/hooks), are component that is configured to be triggered at N actions when we want to make some modification to the project. either to push or even to rebase, or to commit, etc.. We also can control on dependencies... this way...

- applypatch
- commit-msg
- fsmonitor-msg
- post-update
- pre-applypatch
- pre-commit
- pre-merge-commit
- pre-push
- pre-rebase
- pre-receive
- prepare-commit-msg
- push-to-checkout
- sendemail-validate
- update

# Features Stack
- Basic
## 2.level : command execution

## 3.level : Built-in commands

## 4.level : pipes & redirection

## 5.level : Cotntrol flow

## 6.level : script execution
