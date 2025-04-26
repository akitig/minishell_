/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_lstnew.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/04/23 13:42:41 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:31:03 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

t_list	*ft_lstnew(void *content)
{
	t_list	*ans;

	ans = (t_list *)malloc(sizeof(t_list));
	if (ans == NULL)
		return (NULL);
	ans->content = content;
	ans->next = NULL;
	return (ans);
}

/*
#include <stdio.h>
#include <string.h>

int	main(void)
{
	char	test11[] = "12345";
	t_list	*ans;
		t_list	*p;

	puts("----ft_lstnew------");
//	char	test21[] = "abcde";
//	char	test31[] = "123\n";
	ans = ft_lstnew(test11);
	while ()
	{
		p = ans.next;
		printf("%s",ans.next);
}
*/
