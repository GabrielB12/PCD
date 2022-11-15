/*
 * Gabriel Bianchi e Silva
 * Júlia Prates de Sá Carvalho
 * Miguel Silva Taciano
 */

// Argumentos: -fopenmp

#include <omp.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_ITER 20
#define NUM_THREADS 5 /* N clientes + 1 servidor */

int main(void) {
    int soma = 0;
    int request = 0;
    int respond = 0;
    int iteration = 0;
    int thread_id;
    int local;
    int local_iter = 0;
#pragma omp parallel num_threads(NUM_THREADS) default(none)                    \
    firstprivate(local_iter) private(thread_id, local)                         \
        shared(soma, request, respond, iteration)
    {
        thread_id = omp_get_thread_num();
        if (thread_id == 0) {
            /* SERVIDOR */
            while (iteration < MAX_ITER) {
                while (request == 0) {
                    ; /* ESPERA OCUPADA */
                }

                respond = request;

                while (respond != 0) {
                    ; /* ESPERA OCUPADA */
                }

                request = 0;
            }
        } else {
            /* CLIENTE */
            int remainder = 0;
            if (thread_id == (NUM_THREADS - 1)) { /* Último thread */
                remainder = MAX_ITER % (NUM_THREADS - 1);
            }

            while (local_iter < MAX_ITER / (NUM_THREADS - 1) || remainder) {
                while (respond != thread_id) {
                    request = thread_id;
                }

                /* SEÇÃO CRÍTICA */
                local = soma;
                sleep(rand() % 2);
                soma = local + 1;
                iteration += 1;
                printf("iteração(%d): thread[%d] -> soma (local) = %d; soma "
                       "(global) = %d\n",
                       iteration, thread_id, local + 1, soma);
                local_iter += 1;
                /* SEÇÃO CRÍTICA */

                respond = 0;
            }
        }
    }

    printf("soma (total) = %d\n", soma);

    return 0;
}
