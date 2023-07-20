#include <omp.h>
#include "lga_base.h"
#include "lga_omp.h"

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

static void update_omp(byte *grid_in, byte *grid_out, int grid_size,
                       int num_threads, int chunk_size) {
    int start, end;
    #pragma omp parallel for schedule(static) num_threads(num_threads) private(start, end)
    for (int i = 0; i < num_threads; i++) {
        start = i * chunk_size;
        end = start + chunk_size;
        if (end > grid_size)
            end = grid_size;
        update(grid_in, grid_out, grid_size, start, end);
    }
}

void simulate_omp(byte *grid_1, byte *grid_2, int grid_size, int num_threads) {
    int chunk_size = grid_size / num_threads;
    int leftover = grid_size % num_threads;

    num_threads += !!leftover;

    for (int i = 0; i < ITERATIONS/2; i++) {
        update_omp(grid_1, grid_2, grid_size, num_threads, chunk_size);
        update_omp(grid_2, grid_1, grid_size, num_threads, chunk_size);
    }
}
