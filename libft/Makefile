# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/04/16 08:44:44 by akunimot          #+#    #+#              #
#    Updated: 2025/03/27 00:30:24 by akunimot         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME    = libft.a
CC      = cc
RM      = rm -f
CFLAGS  = -Wall -Wextra -Werror

SRCS      = lib/ft_isalpha.c \
            lib/ft_isascii.c \
            lib/ft_isdigit.c \
            lib/ft_isalnum.c \
            lib/ft_isprint.c \
            lib/ft_strlen.c \
            lib/ft_memset.c \
            lib/ft_bzero.c \
            lib/ft_memcpy.c \
            lib/ft_memmove.c \
            lib/ft_strlcpy.c \
            lib/ft_strlcat.c \
            lib/ft_toupper.c \
            lib/ft_tolower.c \
            lib/ft_strchr.c \
            lib/ft_strrchr.c \
            lib/ft_strcmp.c \
            lib/ft_strncmp.c \
            lib/ft_memchr.c \
            lib/ft_memcmp.c \
            lib/ft_strnstr.c \
            lib/ft_atoi.c \
            lib/ft_calloc.c \
            lib/ft_strdup.c \
            lib/ft_substr.c \
            lib/ft_strjoin.c \
            lib/ft_putchar_fd.c \
            lib/ft_putstr_fd.c \
            lib/ft_putendl_fd.c \
            lib/ft_strmapi.c \
            lib/ft_putnbr_fd.c \
            lib/ft_split.c \
            lib/ft_strtrim.c \
            lib/ft_itoa.c \
            lib/ft_striteri.c \
            lib/ft_lstadd_back.c \
            lib/ft_lstadd_front.c \
            lib/ft_lstclear.c \
            lib/ft_lstdelone.c \
            lib/ft_lstiter.c \
            lib/ft_lstlast.c \
            lib/ft_lstmap.c \
            lib/ft_lstnew.c \
            lib/ft_lstsize.c \
            lib/ft_lstdelone.c \
            lib/ft_atol.c \
            lib/ft_strcpy.c \

SRCPF   =   printf/ft_printf.c \
			printf/ft_printf_csp.c \
			printf/ft_printf_diu.c \
			printf/ft_printf_x.c \

SRCGNL  =   gnl/get_next_line.c \
            gnl/get_next_line_utils.c \

OBJS    = $(SRCS:.c=.o)
OBJPF   = $(SRCPF:.c=.o)
OBJGNL  = $(SRCGNL:.c=.o)

all:            $(NAME)

$(NAME): $(OBJS) $(OBJPF) $(OBJGNL) libft.h
	        ar -crs $(NAME) $(OBJS) $(OBJPF) $(OBJGNL)
	        $(RM) $(OBJS) $(OBJPF) $(OBJGNL)

%.o:            %.c
				$(CC) -c $(CFLAGS)  $^ -o $@

clean:
				$(RM) $(OBJS) $(OBJPF) $(OBJGNL)

fclean:			clean
				$(RM) $(NAME)

re:				fclean all

.PHONY:			all clean fclean re bonus