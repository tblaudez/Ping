# **************************************************************************** #
#                                                                              #
#                                                         ::::::::             #
#    Makefile                                           :+:    :+:             #
#                                                      +:+                     #
#    By: tblaudez <tblaudez@student.codam.nl>         +#+                      #
#                                                    +#+                       #
#    Created: 2021/02/09 14:37:19 by tblaudez      #+#    #+#                  #
#    Updated: 2021/02/12 12:46:27 by tblaudez      ########   odam.nl          #
#                                                                              #
# **************************************************************************** #

RED := \e[31m
YELLOW := \e[33m
GREEN := \e[32m
PURPLE := \e[35m
UNDERLINED := \e[4m
RESET := \e[0m

NAME := ft_ping

SRC := $(addprefix src/, ft_ping.c)
INC := $(addprefix include/, ft_ping.h)
OBJ := $(SRC:%.c=%.o)

CFLAGS := -I include/ -Wall -Wextra -g3 #-Werror -std=c99 -pedantic

all: $(NAME)
	@printf "$(PURPLE)Done$(RESET)\n"

$(NAME): $(INC) $(OBJ)
	@printf "\n$(GREEN)Compiling program $(UNDERLINED)%s$(RESET)\n" $(NAME)
	@$(CC) $(CFLAGS) $(OBJ) $(LIB) -o $(NAME)

%.o: %.c
	@printf "$(YELLOW)Compiling source file %-50s$(RESET)\r" $<
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@printf "$(RED)Cleaning object files$(RESET)\n" "$(NAME)"
	@rm -f $(OBJ)

fclean: clean
	@printf "$(RED)Deleting program $(UNDERLINED)%s$(RESET)\n" "$(NAME)"
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

