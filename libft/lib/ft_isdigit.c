/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_isdigit.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/15 20:59:05 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:30:30 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

int	ft_isdigit(int c)
{
	return ('0' <= c && c <= '9');
}

/*
#include <ctype.h>
#include <stdio.h>

int	main(void)
{
	int a = 'a';
	printf("lib:%d own:%d\n", isdigit(a), ft_isdigit(a));

	a = '1';
	printf("lib:%d own:%d\n", isdigit(a), ft_isdigit(a));

	return (0);
}
*/