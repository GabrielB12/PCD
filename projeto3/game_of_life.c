/*
 * Gabriel Bianchi e Silva
 * Júlia Prates de Sá Carvalho
 * Miguel Silva Taciano
 */

// Argumentos: -O3 -march=native -flto
// Rodar: mpirun --hostfile hostfile -np <THREADS> <arquivo>

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 2048
#define EPOCHS 2000

// Mantém o valor entre 0 e SIZE - 1
#define BOUNDED(x, y) (((x) + y) % y)

// Função `alive_neighbors()` retorna o número de vizinhos vivos
int
alive_neighbors(int **grid, int i, int j)
{
    int alive = 0;

    for (int a = i - 1; a <= i + 1; a++) {
        for (int b = j - 1; b <= j + 1; b++) {
            if (a == i && b == j) {
                continue; // É ele mesmo
            }

            alive += grid[BOUNDED(a, SIZE)][BOUNDED(b, SIZE)];
        }
    }

    return alive;
}

// Função `exchange_neighbours()` realiza uma troca não bloquante
// de linhas necessárias para o cálculo de células vivas
void
exchange_neighbours(int **grid, int rank, int top_rank, int btn_rank, int chunk)
{
    MPI_Status status;
    MPI_Request top_request = MPI_REQUEST_NULL;
    MPI_Request btn_request = MPI_REQUEST_NULL;

    // Manda a primeira linha do processo atual para o processo de cima
    MPI_Isend(
        grid[1], SIZE, MPI_INT, top_rank, 0, MPI_COMM_WORLD, &top_request
    );
    // Manda a última linha do processo atual para o processo de baixo
    MPI_Isend(
        grid[chunk - 2], SIZE, MPI_INT, btn_rank, 0, MPI_COMM_WORLD,
        &btn_request
    );

    // Recebe a linha de cima
    MPI_Irecv(
        grid[0], SIZE, MPI_INT, top_rank, 0, MPI_COMM_WORLD, &top_request
    );
    MPI_Wait(&top_request, &status);

    // Recebe a linha de baixo
    MPI_Irecv(
        grid[chunk - 1], SIZE, MPI_INT, btn_rank, 0, MPI_COMM_WORLD,
        &btn_request
    );
    MPI_Wait(&btn_request, &status);
}

// Função `game_of_life()` é passada para cada processo existente,
// assim o jogo é realizado de maneira concorrente
int
game_of_life(
    int **grid, int **new_grid, int world_size, int rank, int top_rank,
    int btn_rank, int chunk
)
{
    int min_alive = 2;      // Mínimo de vizinhos vivos para continuar vivo
    int overpopulation = 4; // Mínimo de vizinhos vivos para morrer
    int revive = 3;         // Quantidade de vizinhos vivos para reviver

    for (int e = 0; e < EPOCHS; e++) {
        for (int i = 1; i < chunk - 1; i++) {
            exchange_neighbours(grid, rank, top_rank, btn_rank, chunk);

            for (int j = 0; j < SIZE; j++) {
                int alive = alive_neighbors(grid, i, j);

                if (grid[i][j] == 1) {
                    if (alive >= min_alive && alive < overpopulation) {
                        new_grid[i][j] = 1; // Sobrevive
                        continue;
                    }
                }

                if (grid[i][j] == 0 && alive == revive) { // Revive
                    new_grid[i][j] = 1;
                    continue;
                }

                new_grid[i][j] = 0; // Morreu ou continou morto
            }
        }

        // Depois de cada geração, os valores são atualizados
        // do grid temporário para o real
        MPI_Barrier(MPI_COMM_WORLD);
        for (int i = 0; i < chunk; i++) {
            for (int j = 0; j < SIZE; j++) {
                grid[i][j] = new_grid[i][j];
                new_grid[i][j] = 0;
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    // TODO: calcular total vivos
    int total_alive = 0;
    for (int i = 0; i < chunk; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (grid[i][j] == 1) {
                total_alive += 1;
            }
        }
    }

    return total_alive;
}

int
main(int argc, char *argv[])
{
    int **grid;
    int **new_grid;
    int rank, world_size, top_rank, btn_rank;
    int chunk, partial_alive_cells, total_alive_cells;

    /* COMEÇO DO PARALELISMO MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // NOTE: não há problemas para divisões sem resto,
    // para os casos de testes (2048 com threads 2^N) é uma implementação
    // satisfatória
    chunk = SIZE / world_size;

    // Inicializando os tabuleiros de maneira paralela
    grid = malloc((chunk + 2) * sizeof(*grid));
    new_grid = malloc((chunk + 2) * sizeof(*new_grid));
    for (int i = 0; i < (chunk + 2); i++) {
        grid[i] = malloc(SIZE * sizeof(*grid[i]));
        new_grid[i] = malloc(SIZE * sizeof(*new_grid[i]));
    }

    // NOTE: Todos valores iniciais estão no processo de rank 0
    // para até ~64 processos, muito mais do que os valores usados para testes
    if (rank == 0) {
        int line, column;

        // GLIDER
        line = 1;
        column = 1;
        grid[line][column + 1] = 1;
        grid[line + 1][column + 2] = 1;
        grid[line + 2][column] = 1;
        grid[line + 2][column + 1] = 1;
        grid[line + 2][column + 2] = 1;

        // R-pentomino
        line = 10;
        column = 30;
        grid[line][column + 1] = 1;
        grid[line][column + 2] = 1;
        grid[line + 1][column] = 1;
        grid[line + 1][column + 1] = 1;
        grid[line + 2][column + 1] = 1;
    }

    top_rank = BOUNDED(rank - 1, world_size);
    btn_rank = BOUNDED(rank + 1, world_size);

    partial_alive_cells = game_of_life(
        grid, new_grid, world_size, rank, top_rank, btn_rank, chunk
    );

    MPI_Reduce(
        &partial_alive_cells, &total_alive_cells, 1, MPI_INT, MPI_SUM, 0,
        MPI_COMM_WORLD
    );

    // Jogo terminou, esperar todos processos
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) { // Apenas um deve mostrar o resultado
        printf("Células vivas: %d\n", total_alive_cells);
    }

    MPI_Finalize();
    /* FIM DO PARALELISMO MPI */

    // MPI_Wtime();

    return 0;
}
