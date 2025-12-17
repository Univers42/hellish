# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/11/07 17:01:26 by dlesieur          #+#    #+#              #
#    Updated: 2025/12/17 02:27:13 by dlesieur         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

include incs/libft/mk/global_conf.mk
include incs/libft/mk/functions.mk

# Generic project Makefile — discovers sources in srcs, builds objects in .objs/, and includes dependency files
NAME			:=	shell
LIBFT			:=	libft.a

# Linker flags
LDFLAGS			:=	-lreadline -pthread -lm

# Submodule / dependency management
SUBMODULE_DIR	:= incs/libft
SUBMODULE_REPO	:= git@github.com:Univers42/libft.git
SUBMODULE_LIB	:= $(SUBMODULE_DIR)/libft.a
REMOTE_HOME		:= $(shell git remote -v | awk 'NR==3 {print $$1}')
CURRENT_BRANCH	:= $(shell git branch --show-current)
REMOTE_CAMPUS	:= $(shell git remote -v | awk 'NR==2 {print $$1}')
MSG				?=
BRANCH			?=

all: $(NAME)

$(SUBMODULE_LIB):
	echo "[DEPS] ensuring libft.a..."
	@if [ -f "$(SUBMODULE_LIB)" ]; then																									\
		echo "[deps] libft.a found: $(SUBMODULE_LIB)";																					\
	else																																\
		if [ -d "$(SUBMODULE_DIR)" ]; then																								\
			if [ -f "$(SUBMODULE_DIR)/Makefile" ]; then																					\
				echo "[deps] building libft in $(SUBMODULE_DIR)";																		\
				$(MAKE) -C $(SUBMODULE_DIR) -j$(nproc) all || true;																		\
			else																														\
				echo "[deps] libft directory exists but no Makefile — cannot build automatically";										\
			fi;																															\
		else																															\
			echo "[deps] libft not present — trying to init submodule";																	\
			if [ -d "$(SUBMODULE_DIR)/.git" ] && ! git -C "$(SUBMODULE_DIR)" rev-parse --is-inside-work-tree >/dev/null 2>&1; then		\
				echo "[ERROR] libft submodule appears broken. Please run:";																\
				echo "  rm -rf $(SUBMODULE_DIR) && git submodule update --init --recursive";											\
				exit 1;																													\
			fi;																															\
			git submodule update --init --recursive $(SUBMODULE_DIR) ||																	\
			git submodule add $(SUBMODULE_REPO) $(SUBMODULE_DIR) || true;																\
			if [ -f "$(SUBMODULE_DIR)/Makefile" ]; then																					\
				echo "[deps] building libft in $(SUBMODULE_DIR)";																		\
				$(MAKE) -C $(SUBMODULE_DIR) all -j$(nproc) || true;																		\
			fi;																															\
		fi;																																\
	fi

$(NAME): $(SUBMODULE_LIB) $(OBJS)
	$(call print_status,$(GREEN),LINK,$@)
	@echo "[link] linking $(NAME)"
	$(CC) $(CFLAGS) -I./incs -I./incs/libft/include -o $@ $(OBJS) $(SUBMODULE_LIB) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@) $(DEPDIR)/$(dir $*)
	$(call print_status,$(CYAN),COMPILING,$<)
	@$(CC) $(CFLAGS) $(OPTFLAGS) -I./incs -I./incs/libft/include  -c $< -o $@ $(PREPROCFLAGS) $(DEPDIR)/$*.d -I$(SUBMODULE_DIR)/include -I./incs

update: configure
	$(call log_info, updating submodule to latest remote version...)
	git submodule update --init --recursive --remote --merge $(SUBMODULE_DIR) || exit 1
	git pull

configure:
	@# Make shell scripts executable and run install-hooks from .scripts or scripts (prefer .scripts)
	@bash -lc '\
	for d in .scripts scripts; do \
	if [ -d "$$d" ]; then \
		find "$$d" -type f -name "*.sh" -exec chmod +x {} + 2>/dev/null || true; \
		if [ -x "$$d/install-hooks.sh" ]; then \
			"$$d/install-hooks.sh" >/dev/null 2>&1 && printf "%s\n" "$(BRIGHT_CYAN)$(BOLD)$(LOG_PREFIX)$(RESET) [$(STATE_COLOR_OK)$(BOLD)OK$(RESET)] : Project hooks configured successfully!"; \
		else \
			printf "%s\n" "$(BRIGHT_CYAN)$(BOLD)$(LOG_PREFIX)$(RESET) [$(STATE_COLOR_WARN)$(BOLD)WARN$(RESET)] : %s/install-hooks.sh not found or not executable." "$$d"; \
		fi; \
		exit 0; \
	fi; \
	done; \
	printf "%s\n" "$(BRIGHT_CYAN)$(BOLD)$(LOG_PREFIX)$(RESET) [$(STATE_COLOR_WARN)$(BOLD)WARN$(RESET)] : no .scripts or scripts directory found."'

push_home: ffclean
	@# require MSG to be provided to avoid accidental empty commits
	@if [ -z "$(MSG)" ]; then																				\
		printf "ERROR: pass a commit message with MSG=\"your message\"\n";									\
		exit 1;																								\
	fi
	@git add .
	@git commit -m "$(MSG)"
	@git push $(REMOTE_HOME) $(CURRENT_BRANCH)

push_campus: ffclean
	@if [ -z "$(MSG)" ]; then																				\
		printf "ERROR: pass a commit message with MSG=\"your message\"\n";									\
		exit 1;																								\
	fi
	@git add .
	@git commit -m "$(MSG)"
	@git push $(REMOTE_CAMPUS) --all




clean:
	@rm -rf $(OBJDIR) $(DEPDIR)

fclean: clean
	@$(RM) $(NAME)
	$(call log_ok, $(NAME) and \ -> Cleaned up Do make if you want to recompile')

re: fclean all
ffclean: fclean
	$(call print_status,$(RED),FFCLEAN,Removing all gen files)
	@if [ -d "$(SUBMODULE_DIR)" ]; then					\
		$(MAKE) -C $(SUBMODULE_DIR) fclean || true;		\
	fi
	$(call log_ok, the libft.a was erased succesfully)
	$(call log_note, use $(GREEN)make$(RESET) to start the project)
fre: ffclean all
rm_branch:
	git branch -D $(BRANCH)
	git push origin --delete $(BRANCH)


start_feat:
	@if [ -z "$(BRANCH)" ]; then																	\
		echo "Error: BRANCH is required. Usage: make start_feat BRANCH=my-feature";					\
		exit 1;																						\
	fi;																								\
	echo "[git-flow] Creating feature branch feat/$(BRANCH) from develop...";						\
	git add .
	git commit
	git checkout -b feat/$(BRANCH);																	\
	echo "[git-flow] Feature branch feat/$(BRANCH) created and checked out."

start_fix:
	@if [ -z "$(BRANCH)" ]; then																	\
		echo "Error: BRANCH is required. Usage: make start_fix BRANCH=my-fix";						\
		exit 1;																						\
	fi;																								\
	echo "[git-flow] Creating fix branch fix/$(BRANCH) from develop...";							\
	git checkout develop;																			\
	git pull origin develop --ff-only;																\
	git checkout -b fix/$(BRANCH);																	\
	echo "[git-flow] Fix branch fix/$(BRANCH) created and checked out."

start_release:
	@if [ -z "$(VERSION)" ]; then																	\
		echo "Error: VERSION is required. Usage: make start_release VERSION=1.0.0";					\
		exit 1;																						\
	fi;																								\
	echo "[git-flow] Creating release branch release/$(VERSION) from develop...";					\
	git checkout develop;																			\
	git pull origin develop --ff-only;																\
	git checkout -b release/$(VERSION);																\
	echo "[git-flow] Release branch release/$(VERSION) created and checked out."


publish:
	@export GIT_PUBLISH=1;																			\
	BR=$$(git branch --show-current);																\
	MSG_VAL='$(subst ','\'',$(MSG))';																\
	if [ -z "$$BR" ]; then echo "fatal: branch name required"; exit 128; fi;						\
	if [ -z "$$MSG_VAL" ]; then echo "fatal: provide commit message via MSG=\"...\""; exit 2; fi;	\
	echo "[publish] current branch: $$BR";															\
	git add .;																						\
	if ! git diff --cached --quiet; then															\
		git commit -m "$$MSG_VAL";																	\
	else																							\
		echo "[publish] nothing to commit";															\
	fi;																								\
	git push origin "$$BR";																			\
	echo "[publish] switching to develop";															\
	git checkout develop;																			\
	git pull --ff-only;																				\
	echo "[publish] merging $$BR -> develop";														\
	git merge --no-ff "$$BR";																		\
	git push origin develop;																		\
	echo "[publish] deleting branch $$BR locally and on origin";									\
	git branch -D "$$BR";																			\
	git push origin --delete "$$BR" || true;														\
	echo "[publish] done (on branch: develop)"

help:
	$(call log_title,MINISHELL BUILD SYSTEM)
	$(call log_section,Build Targets)
	$(call log_definition,make / make all,Build the minishell binary (default))
	$(call log_definition,make re,Clean and rebuild from scratch)
	$(call log_definition,make fre,Full clean includes libft and rebuild)
	
	$(call log_section,Clean Targets)
	$(call log_definition,make clean,Remove object files and archives)
	$(call log_definition,make fclean,Remove binary and object files)
	$(call log_definition,make ffclean,Full clean also cleans libft submodule)
	
	$(call log_section,Build Configuration)
	$(call log_definition,make help-only,Build with FEATURE_HELP_ONLY)
	$(call log_definition,make debug-only,Build with FEATURE_DEBUG_ONLY)
	$(call log_definition,make all-features,Build with all features enabled)
	$(call log_definition,make minimal,Build minimal core no optional features)
	
	$(call log_section,Testing Targets)
	$(call log_definition,make test-lexer,Run lexer tests)
	$(call log_definition,make test-lexer-verbose,Run lexer tests with verbose output)
	$(call log_description,Options: ID=L123 GREP=pattern LOG=path)
	$(call log_definition,make test-lexer-raw,Run raw lexer tests prints to stdout)
	
	$(call log_section,Git and Push Targets)
	$(call log_definition,make push_home,Push to home remote requires MSG)
	$(call log_definition,make push_campus,Push to campus remote requires MSG)
	$(call log_definition,make publish,Publish feature commit merge to develop delete)
	$(call log_description,Options: MSG=your-commit-message)
	$(call log_description,Options: MSG=your-commit-message)
	$(call log_description,Options: MSG=your-commit-message)
	$(call log_description,Options: MSG=your-commit-message)
	$(call log_description,Options: MSG=your-commit-message)
	$(call log_definition,make finish,Finish feature commit merge to develop delete)
	$(call log_definition,make start_feat,Create feature branch feat/BRANCH)
	$(call log_definition,make start_fix,Create fix branch fix/BRANCH)
	$(call log_definition,make start_release,Create release branch release/VERSION)
	
	$(call log_section,Maintenance Targets)
	$(call log_definition,make configure,Set up git hooks and scripts)
	$(call log_definition,make update,Update libft submodule to latest)
	
	$(call log_section,Examples)
	$(call log_definition,make,Build the project)
	$(call log_definition,make re,Clean and rebuild)
	$(call log_definition,make publish MSG=...,Publish with commit message)
	$(call log_definition,make test-lexer-verbose ID=L1,Run specific lexer test)
	@echo ""
	$(call log_ok,For more info check the Makefile itself)
	@echo ""


.PHONY: all update configure push_home push_campus clean fclean re ffclean fre rm_branch start_feat start_fix start_release publich help
