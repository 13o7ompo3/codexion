/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 02:39:16 by obahya            #+#    #+#             */
/*   Updated: 2026/02/27 03:21:09 by obahya           ###   ########.fr       */
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
	long long		available_at;
	int				is_held;
}					t_dongle;

typedef struct s_coder {
	int				id;
	pthread_t		thread_id;
	long long		last_compile_start;
	int				compiles_done;
	t_dongle		*left_dongle;
	t_dongle		*right_dongle;
	struct s_coder	*next;
	struct s_coder	*prev;
	struct s_sim	*sim;
	pthread_cond_t	wakeup_cond;
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
	t_coder			*queue;
	pthread_cond_t	start_cond;
	pthread_mutex_t	state_mutex;
	pthread_mutex_t	write_mutex;

	t_dongle		*dongles;
	t_coder			*coders;
}					t_sim;


int	parse_args(int argc, char **argv, t_sim *sim);
void	*coder_routine(void *arg);
void	*monitor_routine(void *arg);
void	print_action(t_coder *coder, char *action);
void	print_compiling_sequence(t_coder *coder);
long long	get_current_time_ms(void);
int	take_both_dongles(t_coder *coder);
void	release_both_dongles(t_coder *coder);
void	remove_coder(t_sim *sim, t_coder *coder);
void	enqueue(t_sim *dongle, t_coder *new_coder, int scheduler_type);
void	wake_up_coders(t_sim *sim);
long long	get_deadline(t_coder *coder);

#endif
