# If you change SRCS, always run 'make fclean' before 'make' or 'make test'

CC			= 		cc
CFLAGS		= 		-Wall -Wextra -Werror -pedantic -Wshadow -std=c99 -I.
DEPFLAGS	= 		-MMD -MP
NAME_LIB	= 		var
NAME		= 		$(NAME_LIB).a
SRCS		=  		var_cmp.c

OBJ_DIR		=		obj
TEST_DIR	=		unit_test

TEST_SRCS	=		$(TEST_DIR)/test.c
TEST_OBJS	=		$(addprefix $(OBJ_DIR)/, $(TEST_SRCS:%.c=%.o))
TEST_BIN	=		test_$(NAME_LIB)

OBJS		=		$(addprefix $(OBJ_DIR)/, $(SRCS:%.c=%.o))
DPDC		=		$(addprefix $(OBJ_DIR)/, $(SRCS:%.c=%.d))
RM			:=		rm -rf
AR			:=		ar rcs

all: $(NAME)

$(NAME):$(OBJS)
	$(AR) $@ $^

$(OBJ_DIR)/%.o : %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/$(TEST_DIR):
	mkdir -p $(OBJ_DIR)/$(TEST_DIR)

# Build test object
$(OBJ_DIR)/$(TEST_DIR)/%.o : $(TEST_DIR)/%.c | $(OBJ_DIR)/$(TEST_DIR)
	$(CC) $(CFLAGS) -I. -c $< -o $@

# Test target
test: $(NAME) $(TEST_OBJS)
	$(CC) $(CFLAGS) $(TEST_OBJS) $(OBJS) -o $(TEST_BIN)
	@echo "Run ./$(TEST_BIN) to execute tests."

clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME) $(TEST_BIN)

re: fclean all

.PHONY: all clean fclean re test

-include $(DPDC)