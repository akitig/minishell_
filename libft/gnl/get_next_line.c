/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunimot <akitig24@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/03 14:07:40 by akunimot          #+#    #+#             */
/*   Updated: 2025/01/26 16:32:52 by akunimot         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../libft.h"

char	*bond_buffer(char *save_buffer, char *read_buffer)
{
	char	*join_buffer;

	join_buffer = ft_gnl_strjoin(save_buffer, read_buffer);
	free(save_buffer);
	if (!join_buffer)
		return (NULL);
	return (join_buffer);
}

void	make_next_buffer_from_file(int fd, char **buffer)
{
	ssize_t	read_byte;
	char	*read_buffer;

	if (!*buffer)
		*buffer = ft_calloc(1, 1);
	read_buffer = ft_calloc(BUFFER_SIZE + 1, sizeof(char));
	if (!read_buffer)
	{
		free(*buffer);
		*buffer = NULL;
		return ;
	}
	read_byte = read(fd, read_buffer, BUFFER_SIZE);
	while (read_byte > 0)
	{
		read_buffer[read_byte] = '\0';
		*buffer = bond_buffer(*buffer, read_buffer);
		if (ft_strchr(read_buffer, '\n'))
			break ;
		read_byte = read(fd, read_buffer, BUFFER_SIZE);
	}
	free(read_buffer);
}

char	*make_new_line(char *buffer)
{
	char	*line;
	int		i;

	i = 0;
	if (!buffer[i])
		return (NULL);
	while (buffer[i] && buffer[i] != '\n')
		i++;
	line = ft_calloc(i + 2, sizeof(char));
	i = 0;
	while (buffer[i] && buffer[i] != '\n')
	{
		line[i] = buffer[i];
		i++;
	}
	if (buffer[i] && buffer[i] == '\n')
		line[i++] = '\n';
	return (line);
}

char	*save_buffer_for_next(char *buffer)
{
	int		i;
	int		next_i;
	char	*next_buffer;

	i = 0;
	while (buffer[i] && buffer[i] != '\n')
		i++;
	if (!buffer[i])
	{
		free(buffer);
		return (NULL);
	}
	next_buffer = ft_calloc((ft_strlen(buffer) - i + 1), sizeof(char));
	next_i = 0;
	i++;
	while (buffer[i])
		next_buffer[next_i++] = buffer[i++];
	free(buffer);
	return (next_buffer);
}

char	*get_next_line(int fd)
{
	char		*line;
	static char	*buffer;

	if (fd < 0 || BUFFER_SIZE <= 0 || read(fd, 0, 0) < 0)
	{
		free(buffer);
		buffer = NULL;
		return (NULL);
	}
	make_next_buffer_from_file(fd, &buffer);
	if (!buffer)
		return (NULL);
	line = make_new_line(buffer);
	buffer = save_buffer_for_next(buffer);
	if (!line)
		free(buffer);
	return (line);
}
