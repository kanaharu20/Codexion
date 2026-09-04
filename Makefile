NAME		= codexion

SRC_DIR		= src
INCL_DIR	= incl
OBJ_DIR		= obj

SRC			= main.c codexion.c dongle.c coder.c shared.c \
			  heap.c dongle_cooldown.c dongle_acquire.c dongle_release.c \
			  log.c coder_routine.c
OBJ			= $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))

CC			= cc
CFLAGS		= -Wall -Wextra -Werror -pthread
INCLUDES	= -I$(INCL_DIR)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INCL_DIR)/header.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
