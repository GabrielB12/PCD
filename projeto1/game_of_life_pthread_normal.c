/*
 * Gabriel Bianchi e Silva
 * Júlia Prates de Sá Carvalho
 * Miguel Silva Taciano
 */

// Argumentos: -fopenmp -pthread -O3 -march=native -flto

#include <omp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define THREADS 8
#define SIZE 2048
#define EPOCHS 2000

typedef struct {
    int id;
    int start;
    int end;
    int **grid;
    int **newGrid;
} GOLArgs;

typedef struct {
    pthread_t id;
} Thread;

static pthread_barrier_t barrier;
static pthread_mutex_t alive_mutex = PTHREAD_MUTEX_INITIALIZER;
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
void *gameOfLife(void *argumentos) {
    GOLArgs *args = (GOLArgs *)argumentos;
    int min_alive = 2;
    int overpopulation = 4;
    int revive = 3;

    for (int e = 0; e < EPOCHS; e++) {
        for (int i = args->start; i < args->end; i++) {
            for (int j = 0; j < SIZE; j++) {
                int alive = getNeighbors(args->grid, i, j);

                if (args->grid[i][j] == 1) {
                    if (alive >= min_alive && alive < overpopulation) {
                        args->newGrid[i][j] = 1; // Sobrevive
                        continue;
                    }
                }

                if (args->grid[i][j] == 0 && alive == revive) { // Revive
                    args->newGrid[i][j] = 1;

                    // Sessão crítica
                    pthread_mutex_lock(&alive_mutex);
                    currently_alive += 1;
                    pthread_mutex_unlock(&alive_mutex);
                    continue;
                }

                args->newGrid[i][j] = 0;

                if (args->grid[i][j] == 1) { // Morreu
                    // Sessão crítica
                    pthread_mutex_lock(&alive_mutex);
                    currently_alive -= 1;
                    pthread_mutex_unlock(&alive_mutex);
                }
            }
        }

        // TODO: printar 50x50
        pthread_barrier_wait(&barrier);
        if (args->id == 1) {
            printf("Geração %d: %d\n", e + 1, currently_alive);
        }

        for (int i = args->start; i < args->end; i++) {
            for (int j = 0; j < SIZE; j++) {
                // printf("thread[%d] -> i: %d j: %d\n", args->id, i, j);
                args->grid[i][j] = args->newGrid[i][j];
                args->newGrid[i][j] = 0;
            }
        }
        pthread_barrier_wait(&barrier);
    }

    return NULL;
}

int main() {
    int **grid;
    int **newGrid;
    int lin, col, chunk;
    Thread threads[THREADS];
    GOLArgs args[THREADS];

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

    // Inicializando os argumentos
    chunk = SIZE / THREADS;
    for (int i = 0; i < THREADS; i++) {
        args[i].id = i + 1;
        args[i].start = i * chunk;
        args[i].end = (i + 1) * chunk;

        args[i].grid = grid;
        args[i].newGrid = newGrid;
    }

    // Criando as threads para o game of life
    pthread_barrier_init(&barrier, NULL, THREADS);
    double inicial = omp_get_wtime();
    for (int i = 0; i < THREADS; i++) {
        pthread_create(&threads[i].id, NULL, gameOfLife, (void *)&args[i]);
    }

    // Dando join nas threads do game of life
    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i].id, NULL);
    }
    double final = omp_get_wtime();
    pthread_barrier_destroy(&barrier);
    printf("Tempo game_of_life(): %.3f\n", final - inicial);
    printf("Threads usados: %d\n", THREADS);

    // Free nos 'malloc()'
    for (int i = 0; i < (int)SIZE; i++) {
        free(grid[i]);
        free(newGrid[i]);
    }
    free(grid);
    free(newGrid);

    return 0;
}
