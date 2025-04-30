/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strrchr.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/16 05:55:16 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:32:20 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

char	*ft_strrchr(const char *s, int c)
{
	char	*ans;
	size_t	i;

	ans = NULL;
	i = 0;
	while (1)
	{
		if (s[i] == (char)c)
			ans = (char *)(s + i);
		if (s[i] == '\0')
			break ;
		i++;
	}
	return (ans);
}

/*
#include "stdio.h"

   #include <sys/mman.h>

void	*electric_alloc(size_t size)
{
	long	page_size;
	void	*ptr;

	page_size = sysconf(_SC_PAGESIZE);
	ptr = mmap(NULL, page_size * 2, PROT_READ | PROT_WRITE,
			MAP_ANON | MAP_PRIVATE, -1, 0);
	memset(ptr, 'Z', page_size * 2);
	mprotect(ptr + page_size, page_size, PROT_NONE);
	return (ptr + page_size - size);
}

int	main(void)
{
	char	*str1;

	// your strrchr crash because it read too many bytes or attempt to write on s !
        str1 = malloc(INT_MAX-100);   char *str2 = malloc(INT_MAX-100);
		memset(str1, 'a', INT_MAX-110);
		memset(str2, 'a', INT_MAX-110);
		printf("%s\n", ft_strrchr(str1, 'z'));
		printf("%s\n", strrchr(str2,  'z'));
		printf("%d\n", strcmp(str1, str2));
}
*/