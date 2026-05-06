/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 02:36:45 by obahya            #+#    #+#             */
/*   Updated: 2026/05/02 11:22:02 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	main(int ac, char **av)
{
	t_sim	*sim;
	int		error;

	sim = ft_calloc(1, sizeof(t_sim));
	if (!sim)
		return (-1);
	error = parse_args(sim, ac, av);
	if (error == 1)
		printf("Invalid arguments, usage: ./codexion <num_coders> "\
"<time_to_burnout> <time_to_compile> <time_to_debug> "\
"<time_to_refactor> <required_compiles> <dongle_cooldown> "\
"<scheduler_type>\n");
	else if (error == 2)
		printf("Failed to initialize simulation\n");
	else
	{
		error = start_simulation(sim);
		if (error == 3)
			printf("Error: Failed to start simulation\n");
		printf("Simulation ended\n");
	}
	if (error != 1)
		cleanup_simulation(sim);
	free(sim);
	return (error);
}
