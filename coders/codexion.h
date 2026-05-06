/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: obahya <obahya@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 02:39:16 by obahya            #+#    #+#             */
/*   Updated: 2026/05/05 18:57:27 by obahya           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <sys/time.h>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>

// # define f_malloc(x, ...) NULL

typedef struct s_coder	t_coder;

typedef struct s_dongle
{
	int				id;
	long long		available_at;
	int				in_use;
	int				reserved;
	char			__cache_padding[44]; /* Forces struct to 64 bytes */
}	t_dongle;

typedef struct s_node
{
	t_coder			*coder;
	struct s_node	*next;
	struct s_node	*prev;
}	t_node;

typedef struct s_coder
{
	int				id;
	int				compiles_done;
	long long		last_compile_start;
	int				is_finished;
	int				owns_hardware;
	long long		deadline;
	long long		wake_up_time;
	int				is_sleeping;

	t_dongle		*left_dongle;
	t_dongle		*right_dongle;

	pthread_cond_t	queue_cond;
	pthread_cond_t	sleep_cond;

	struct s_sim	*sim;
	struct s_node	node;
	pthread_t		thread_id;
	pthread_mutex_t	coder_mutex;
	char			__cache_padding[64]; /* Prevents coder cache collision */
}	t_coder;

/* THE HEAP (Priority Queue) */
typedef struct s_heap
{
	t_coder			**array;
	int				size;
	int				capacity;
	int				(*compare)(t_coder *a, t_coder *b);
}	t_heap;

typedef struct s_sim
{
	int				num_coders;
	int				time_to_burnout;
	int				time_to_compile;
	int				time_to_debug;
	int				time_to_refactor;
	int				required_compiles;
	int				dongle_cooldown;
	int				scheduler_type;

	int				is_active;
	int				threads_ready;
	long long		start_time;

	t_coder			*coders;
	t_dongle		*dongles;
	t_node			*queue;
	t_heap			*sleep_heap;

	pthread_mutex_t	state_mutex;
	pthread_mutex_t	write_mutex;
	pthread_cond_t	start_cond;

	pthread_mutex_t	sleep_mutex;
	pthread_cond_t	sleep_room_cond;
	pthread_mutex_t	queue_mutex;
	pthread_cond_t	waiter_cond;

	pthread_t		timer_thread;
	pthread_t		waiter_thread;
	pthread_t		monitor_thread;

	int				main_init_step;
	int				coders_init_num;
	int				init_last_coder;
}	t_sim;

/* Initialization & Cleanup (init.c / main.c) */
int			parse_args(t_sim *sim, int ac, char **av);
int			init_simulation(t_sim *sim);
int			start_simulation(t_sim *sim);
void		cleanup_simulation(t_sim *sim);

/* The Life Cycle (coder.c) */
void		*coder_routine(void *arg);
int			take_both_dongles(t_coder *coder);
void		release_both_dongles(t_coder *coder);
int			is_sim_active(t_sim *sim);

/* Time & Precision (time.c) */
long long	get_current_time_ms(void);
// void		precise_sleep(long long time_in_ms, t_sim *sim);

/* Monitoring & Safety (monitor.c) */
void		*monitor_routine(void *arg);
void		pull_the_fire_alarm(t_sim *sim);
void		print_action(t_coder *coder, char *action);
int			is_burnout(t_coder *coder, long long now);

/* Waiter */
void		*waiter_routine(void *arg);
int			process_queue_node(t_sim *sim, t_node *current, long long now,
				long long *min_wait);

/* Heap (heap.c) */
void		heap_insert(t_heap *heap, t_coder *coder);
void		heap_remove_at(t_heap *heap, int idx);
t_heap		*init_heap(int capacity, int scheduler_type);
void		free_heap(t_heap *heap);
void		heapify_up(t_heap *heap, int idx);
void		heapify_down(t_heap *heap, int idx);
int			compare_fifo(t_coder *a, t_coder *b);
int			compare_edf(t_coder *a, t_coder *b);
int			compare_sleep(t_coder *a, t_coder *b);

/* Queue (queue.c) */
t_node		*append_node(t_node *head, t_node *new_node,
				int (*compare)(t_coder *, t_coder *));
t_node		*remove_node(t_node **head, t_node *node);

/* Sleep Room (sleep_room.c) */
void		*sleep_room_routine(void *arg);
void		request_sleep(t_coder *coder, long long duration_ms);
void		set_timespec_timeout(struct timespec *ts, long long delay_in_ms);

void		wake_up_coders(t_sim *sim);
void		print_compiling_sequence(t_coder *coder);

/* Memory Allocation (calloc.c) */
void		*ft_calloc(size_t nmemb, size_t lsize);

#endif
