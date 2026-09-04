CC      = cc
CFLAGS  = -Wall -Wextra -Werror -pthread
SRC     = 01_create_join.c 02_mutex.c 03_cond.c 04_timedwait.c 05_mini_codexion.c
BIN     = $(SRC:.c=)

all: $(BIN)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(BIN)

re: clean all

.PHONY: all clean re
