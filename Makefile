# **************************************************************************** #
#                                                                              #
#                                                         ::::::::             #
#    Makefile                                           :+:    :+:             #
#                                                      +:+                     #
#    By: tblaudez <tblaudez@student.codam.nl>         +#+                      #
#                                                    +#+                       #
#    Created: 2021/02/09 14:37:19 by tblaudez      #+#    #+#                  #
#    Updated: 2021/03/19 14:23:51 by tblaudez      ########   odam.nl          #
#                                                                              #
# **************************************************************************** #

RED := \e[31m
YELLOW := \e[33m
GREEN := \e[32m
PURPLE := \e[35m
UNDERLINED := \e[4m
RESET := \e[0m

NAME := ft_ping

SRC := $(addprefix src/, libft_functions.c ping.c receive.c send.c setup.c utils.c)
INC := $(addprefix include/, ft_ping.h)
OBJ := $(SRC:%.c=%.o)
CFLAGS := -I include/ -Wall -Wextra -Werror -Ofast
FLAGS := -lm

all: $(NAME)
	@printf "$(PURPLE)Done$(RESET)\n"

$(NAME): $(INC) $(OBJ)
	@printf "\n$(GREEN)Compiling program $(UNDERLINED)%s$(RESET)\n" $(NAME)
	@$(CC) $(CFLAGS) $(FLAGS) $(OBJ) $(LIB) -o $(NAME)

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

