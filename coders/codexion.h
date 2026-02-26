/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 02:39:16 by obahya            #+#    #+#             */
/*   Updated: 2026/02/26 04:55:01 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <unistd.h>


typedef struct s_dongle {
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	long long		available_at;
	int				is_held;
	void			*queue;
}					t_dongle;

typedef struct s_coder {
	int				id;
	pthread_t		thread_id;
	long long		last_compile_start;
	int				compiles_done;
	t_dongle		*left_dongle;
	t_dongle		*right_dongle;
	struct s_sim	*sim;
}					t_coder;

typedef struct s_sim {
	int				num_coders;
	long long		start_time;
	long long		time_to_burnout;
	long long		time_to_compile;
	long long		time_to_debug;
	long long		time_to_refactor;
	int				required_compiles;
	long long		dongle_cooldown;
	int				scheduler_type;
	int				is_active;
	int				threads_ready;
	pthread_cond_t	start_cond;
	pthread_mutex_t	state_mutex;
	pthread_mutex_t	write_mutex;

	t_dongle		*dongles;
	t_coder			*coders;
}					t_sim;

typedef struct s_node {
	t_coder			*coder;
	struct s_node	*next;
	struct s_node	*prev;
}					t_node;

int	parse_args(int argc, char **argv, t_sim *sim);
void	*coder_routine(void *arg);
void	*monitor_routine(void *arg);
t_node	*dequeue(t_dongle *dongle);
void	enqueue(t_dongle *dongle, t_node *new_node, int scheduler_type);
void	print_action(t_coder *coder, char *action);
long long	get_current_time_ms(void);

#endif
