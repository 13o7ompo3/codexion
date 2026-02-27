/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 02:36:45 by obahya            #+#    #+#             */
/*   Updated: 2026/02/27 05:01:44 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void *monitor_routine(void *arg)
{
	t_sim *sim = (t_sim *)arg;
	int i;
	int all_compiled;
	pthread_mutex_lock(&sim->state_mutex);
	while (sim->threads_ready == 0)
		pthread_cond_wait(&sim->start_cond, &sim->state_mutex);
	pthread_mutex_unlock(&sim->state_mutex);
	while (1)
	{
		i = 0;
		all_compiled = 1;
		while (i < sim->num_coders)
		{
			pthread_mutex_lock(&sim->state_mutex);
			if ((get_current_time_ms() - sim->coders[i].last_compile_start) > sim->time_to_burnout)
			{
				sim->is_active = 0;
				pthread_cond_broadcast(&sim->arbiter_cond);
				pthread_mutex_unlock(&sim->state_mutex);
				print_action(&sim->coders[i], "burned out");
				return (NULL);
			}
			if (sim->required_compiles == -1 || sim->coders[i].compiles_done < sim->required_compiles)
				all_compiled = 0;
			
			pthread_mutex_unlock(&sim->state_mutex);
			i++;
		}
		if (sim->required_compiles != -1 && all_compiled == 1)
		{
			pthread_mutex_lock(&sim->state_mutex);
			sim->is_active = 0;
			pthread_cond_broadcast(&sim->arbiter_cond);
			pthread_mutex_unlock(&sim->state_mutex);
			return (NULL);
		}
		usleep(1000); 
	}
	return (NULL);
}
int start_simulation(t_sim *sim)
{
	int         i;
	pthread_t   monitor_thread;
	i = 0;
	while (i < sim->num_coders)
	{
		if (pthread_create(&sim->coders[i].thread_id, NULL, &coder_routine, &sim->coders[i]) != 0)
			return (1);
		i++;
	}
	if (pthread_create(&monitor_thread, NULL, &monitor_routine, sim) != 0)
		return (1);
	pthread_mutex_lock(&sim->state_mutex);
	sim->start_time = get_current_time_ms();
	i = 0;
	while (i < sim->num_coders)
	{
		sim->coders[i].last_compile_start = sim->start_time;
		i++;
	}
	sim->threads_ready = 1;
	pthread_cond_broadcast(&sim->start_cond);
	pthread_mutex_unlock(&sim->state_mutex);
	pthread_join(monitor_thread, NULL);
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_join(sim->coders[i].thread_id, NULL);
		i++;
	}
	return (0);
}
void cleanup_simulation(t_sim *sim)
{
	int i;
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_mutex_destroy(&sim->dongles[i].mutex);
		i++;
	}
	while (sim->queue != NULL)
	{
		t_node *leaked_node = (t_node *)sim->queue;
		remove_node(sim, leaked_node);
		free(leaked_node);
	} 
	pthread_cond_destroy(&sim->arbiter_cond);
	pthread_cond_destroy(&sim->start_cond);
	pthread_mutex_destroy(&sim->state_mutex);
	pthread_mutex_destroy(&sim->write_mutex);
	
	free(sim->coders);
	free(sim->dongles);
}

int	main(int argc, char **argv)
{
	t_sim	*sim;
	int		error;

	sim = malloc(sizeof(t_sim));
	if (!sim)
		return (2);
	error = parse_args(argc, argv, sim);
	if (error == 1)
		printf("Invalid arguments, usage: ./codexion <num_coders> "
			"<time_to_burnout> <time_to_compile> <time_to_debug> "
			"<time_to_refactor> <required_compiles> <dongle_cooldown> "
			"<scheduler_type> %d\n", argc);
	else if (error == 2)
		printf("Failed to initialize simulation\n");
	else
	{
		error = start_simulation(sim);
		if (error == 1)
			printf("Error: Failed to start simulation\n");
		printf("Simulation ended\n");
	}
	cleanup_simulation(sim);
	free(sim);
	return (error);
}
