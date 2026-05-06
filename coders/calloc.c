/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   calloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/02 11:16:38 by obahya            #+#    #+#             */
/*   Updated: 2026/05/02 11:26:06 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdlib.h>
#include <string.h>

void	*ft_calloc(size_t nmemb, size_t lsize)
{
	void	*result;
	size_t	size;

	size = lsize * nmemb;
	if (nmemb && lsize != (size / nmemb))
		return (NULL);
	result = malloc(size);
	if (result != NULL)
		memset(result, 0, size);
	return (result);
}
