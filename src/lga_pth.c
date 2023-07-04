#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "lga_base.h"
#include "lga_pth.h"
#include "pthread_barrier.h"

struct SHARED_DATA {
    byte *grid_1;
    byte *grid_2;
    int grid_size;
} SHARED;

typedef struct {
    int thread_id;
    int batch_size;
} thread_args_t;

pthread_barrier_t barrier;

static byte get_next_cell(int i, int j, byte *grid_in, int grid_size) {
    byte next_cell = EMPTY;

    for (int dir = 0; dir < NUM_DIRECTIONS; dir++) {
        int rev_dir = (dir + NUM_DIRECTIONS/2) % NUM_DIRECTIONS;
        byte rev_dir_mask = 0x01 << rev_dir;

        int di = directions[i%2][dir][0];
        int dj = directions[i%2][dir][1];
        int n_i = i + di;
        int n_j = j + dj;

        if (inbounds(n_i, n_j, grid_size)) {
            if (grid_in[ind2d(n_i,n_j)] == WALL) {
                next_cell |= from_wall_collision(i, j, grid_in, grid_size, dir);
            }
            else if (grid_in[ind2d(n_i, n_j)] & rev_dir_mask) {
                next_cell |= rev_dir_mask;
            }
        }
    }

    return check_particles_collision(next_cell);
}

static void update(byte *grid_in, byte *grid_out, int grid_size,
                   int start, int end) {
    for (int i = start; i < end; i++) {
        for (int j = 0; j < grid_size; j++) {
            if (grid_in[ind2d(i,j)] == WALL)
                grid_out[ind2d(i,j)] = WALL;
            else
                grid_out[ind2d(i,j)] = get_next_cell(i, j, grid_in, grid_size);
        }
    }
}

static void *update_thread(void *args) {
    thread_args_t *t_args = (thread_args_t *) args;
    int start = t_args->thread_id * t_args->batch_size;
    int end = (t_args->thread_id + 1) * t_args->batch_size;

    byte *grid_1 = SHARED.grid_1;
    byte *grid_2 = SHARED.grid_2;
    int grid_size = SHARED.grid_size;

    if (end > grid_size)
        end = grid_size;

    update(grid_1, grid_2, grid_size, start, end);
    pthread_barrier_wait(&barrier);
    update(grid_2, grid_1, grid_size, start, end);

    pthread_exit(NULL);
}

void simulate_pth(byte *grid_1, byte *grid_2, int grid_size, int num_threads) {
    pthread_t threads[num_threads];
    pthread_attr_t attr;
    thread_args_t *t_args = malloc(sizeof(thread_args_t) * num_threads);
    int batch_size = grid_size / num_threads,
        leftover = grid_size % num_threads,
        rc;

    num_threads += !!leftover;

    SHARED.grid_1 = grid_1;
    SHARED.grid_2 = grid_2;
    SHARED.grid_size = grid_size;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (int i = 0; i < ITERATIONS/2; i++) {
        pthread_barrier_init(&barrier, NULL, num_threads);
        for (int t = 0; t < num_threads; t++) {
            t_args[t].thread_id = t;
            t_args[t].batch_size = batch_size;

            rc = pthread_create(&threads[t], &attr, update_thread, (void *) &t_args[t]);
            if (rc) {
                printf("ERROR; return code from pthread_create() is %d\n", rc);
                exit(-1);
            }
        }

        for (int t = 0; t < num_threads; t++) {
            rc = pthread_join(threads[t], NULL);
            if (rc) {
                printf("ERROR; return code from pthread_join() is %d\n", rc);
                exit(-1);
            }
        }
    }

    pthread_attr_destroy(&attr);
    pthread_barrier_destroy(&barrier);
}
