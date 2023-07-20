#include "lga_base.h"
#include "lga_seq.h"

// Atualiza o LGA a partir do estado em grid_in,
// escrevendo o resultado em grid_out
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

// Simula ITERATIONS iteracoes do LGA de forma sequencial,
// a partir do estado inicial lido em initialize_grids
void simulate_seq(byte *grid_1, byte *grid_2, int grid_size) {
    // Alterna entre grid_1 e grid_2 como grid "prinicipal"
    // para evitar copias desnecessarias de um para o outro
    for (int i = 0; i < ITERATIONS/2; i++) {
        update(grid_1, grid_2, grid_size);
        update(grid_2, grid_1, grid_size);
    }
}
