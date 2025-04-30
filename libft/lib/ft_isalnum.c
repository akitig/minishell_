/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_isalnum.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/15 21:04:27 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:30:21 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

int	ft_isalnum(int c)
{
	return (ft_isalpha(c) || ft_isdigit(c));
}

/*
#include <ctype.h>
#include <stdio.h>

int	main(void)
{
	int	a;

	a = 'a';
	printf("lib:%d own:%d\n", isalnum(a), ft_isalnum(a));
	a = '1';
	printf("lib:%d own:%d\n", isalnum(a), ft_isalnum(a));
	return (0);
}
*/
