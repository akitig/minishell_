# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/03/15 15:32:16 by akunimot          #+#    #+#              #
#    Updated: 2025/05/09 14:21:11 by akunimot         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME         = minishell
CC           = cc
CFLAGS       = -Wall -Wextra -Werror -g

# ---------- libft --------------
LIBFT_DIR    = ./libft
LIBFT_NAME   = libft.a
LIBFT        = $(LIBFT_DIR)/$(LIBFT_NAME)

# ---------- srcs/objs ----------
SRC_DIR      = ./srcs
OBJ_DIR      = ./objs

SRCS         = $(SRC_DIR)/main.c \


OBJS         = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

READLINE     = -lreadline

# ==============================================================================

all: $(LIBFT) $(NAME)

# ---------- Minishell ----------
$(NAME): $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) $(LIBFT) $(READLINE) -o $(NAME)
	@echo "minishell compiled."

# ---------- libft --------------
$(LIBFT):
	@echo "Making libft..."
	@$(MAKE) -C $(LIBFT_DIR)

# ---------- make objs ----------
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -I./includes -I$(LIBFT_DIR) -c $< -o $@

# ---------- clean --------------
clean:
	@$(MAKE) -C $(LIBFT_DIR) clean
	rm -rf $(OBJ_DIR)

fclean: clean
	@$(MAKE) -C $(LIBFT_DIR) fclean
	rm -f $(NAME)

norm:
#	norminette minishell.h
	norminette $(SRCS)

# ---------- Minishell ----------
debug:
	@$(CC) -g -O0 $(CFLAGS) $(SRCS) $(LIBFT) $(READLINE) -o $(NAME)
	@echo "minishell debug compiled."

test:
	make
	bash test.sh

re:	fclean	all

.PHONY:all	clean	fclean	re