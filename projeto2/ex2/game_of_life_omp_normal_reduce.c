/*
 * Gabriel Bianchi e Silva
 * Júlia Prates de Sá Carvalho
 * Miguel Silva Taciano
 */

// Argumentos: -fopenmp -O3 -march=native -flto

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define THREADS 8
#define SIZE 2048
#define EPOCHS 2000

static int currently_alive;

// Função 'getNeighbors()' retorna o número de vizinhos vivos
int getNeighbors(int **grid, int i, int j) {
    int alive = 0;

    for (int a = i - 1; a <= i + 1; a++) {
        for (int b = j - 1; b <= j + 1; b++) {
            if (a == i && b == j) {
                continue; // É ele mesmo
            }

            alive +=
                grid[((a % SIZE) + SIZE) % SIZE][((b % SIZE) + SIZE) % SIZE];
        }
    }

    return alive;
}

// Função 'gameOfLife()' é passada para cada thread existente,
// assim o jogo é realizado de maneira concorrente
void gameOfLife(int **grid, int **newGrid) {
    int min_alive = 2;
    int overpopulation = 4;
    int revive = 3;
    int threads;
    double inicial, final;

#pragma omp parallel num_threads(THREADS)
    for (int e = 0; e < EPOCHS; e++) {
        threads = omp_get_num_threads();
#pragma omp for collapse(2)
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                int alive = getNeighbors(grid, i, j);

                if (grid[i][j] == 1) {
                    if (alive >= min_alive && alive < overpopulation) {
                        newGrid[i][j] = 1; // Sobrevive
                        continue;
                    }
                }

                if (grid[i][j] == 0 && alive == revive) { // Revive
                    newGrid[i][j] = 1;

                    continue;
                }

                newGrid[i][j] = 0;
            }
        }

        if (e == EPOCHS - 1) { // Última geração
            inicial = omp_get_wtime();

            currently_alive = 0;
#pragma omp for reduction(+ : currently_alive)
            for (int i = 0; i < SIZE; i++) {
                for (int j = 0; j < SIZE; j++) {
                    currently_alive += newGrid[i][j];
                }
            }
            final = omp_get_wtime();
        }

#pragma omp barrier
#pragma omp master
        { printf("Geração %d\n", e + 1); }

#pragma omp for collapse(2)
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                // printf("thread[%d] -> i: %d j: %d\n", id, i, j);
                grid[i][j] = newGrid[i][j];
                newGrid[i][j] = 0;
            }
        }
#pragma omp barrier
    }

    printf("Tempo para soma de células vivas(): %f\n", final - inicial);
    printf("Células vivas: %d\n", currently_alive);
    printf("Threads usados: %d\n", threads);
}

int main() {
    int **grid;
    int **newGrid;
    int lin, col;

    // Inicializando os tabuleiros
    grid = calloc(SIZE, sizeof(*grid));
    newGrid = calloc(SIZE, sizeof(*newGrid));
    for (int i = 0; i < (int)SIZE; i++) {
        grid[i] = calloc(SIZE, sizeof(*grid[i]));
        newGrid[i] = calloc(SIZE, sizeof(*newGrid[i]));
    }

    // GLIDER
    lin = 1;
    col = 1;
    grid[lin][col + 1] = 1;
    grid[lin + 1][col + 2] = 1;
    grid[lin + 2][col] = 1;
    grid[lin + 2][col + 1] = 1;
    grid[lin + 2][col + 2] = 1;

    // R-pentomino
    lin = 10;
    col = 30;
    grid[lin][col + 1] = 1;
    grid[lin][col + 2] = 1;
    grid[lin + 1][col] = 1;
    grid[lin + 1][col + 1] = 1;
    grid[lin + 2][col + 1] = 1;

    // Printanto os vivos e guardando o valor em uma variável global
    currently_alive = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            currently_alive += grid[i][j];
        }
    }
    printf("Condição inicial: %d\n", currently_alive);

    // usar omp
    gameOfLife(grid, newGrid);

    // Free nos 'malloc()'
    for (int i = 0; i < (int)SIZE; i++) {
        free(grid[i]);
        free(newGrid[i]);
    }
    free(grid);
    free(newGrid);

    return 0;
}
