#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "lga_base.h"
#include "lga_pth.h"

struct SHARED_DATA {
    byte *grid_1;
    byte *grid_2;
    int grid_size;
} SHARED;

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

static void update(byte *grid_in, byte *grid_out, int grid_size) {
    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            if (grid_in[ind2d(i,j)] == WALL)
                grid_out[ind2d(i,j)] = WALL;
            else
                grid_out[ind2d(i,j)] = get_next_cell(i, j, grid_in, grid_size);
        }
    }
}

static void *update_thread(void *args) {
    byte *grid_1 = SHARED.grid_1;
    byte *grid_2 = SHARED.grid_2;
    int grid_size = SHARED.grid_size;

    update(grid_1, grid_2, grid_size);
    update(grid_2, grid_1, grid_size);

    pthread_exit(NULL);
}

void simulate_pth(byte *grid_1, byte *grid_2, int grid_size, int num_threads) {
    pthread_t thread;
    pthread_attr_t attr;
    int rc;

    SHARED.grid_1 = grid_1;
    SHARED.grid_2 = grid_2;
    SHARED.grid_size = grid_size;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (int i = 0; i < ITERATIONS/2; i++) {
        rc = pthread_create(&thread, &attr, update_thread, NULL);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }

        rc = pthread_join(thread, NULL);
        if (rc) {
            printf("ERROR; return code from pthread_join() is %d\n", rc);
            exit(-1);
        }
    }

    pthread_attr_destroy(&attr);
}
