#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXFILA 8
#define MAXNUM 64
#define NUM_PROD 1
#define NUM_CONS 1

int fila[MAXFILA];
int fim_da_fila = 0;
int inicio_da_fila = 0;
int qtd_na_fila = 0;
int produtor_terminado = 0;
int consumidor_terminado = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fila_nao_cheia = PTHREAD_COND_INITIALIZER;
pthread_cond_t fila_nao_vazia = PTHREAD_COND_INITIALIZER;

void *produtor(void *arg);
void *consumidor(void *arg);

int main(int argc, char *argv[])
{
    pthread_t prod[NUM_PROD];
    pthread_t cons[NUM_CONS];

    printf("Iniciando produtor e consumidor...\n");

    int t;
    for (t = 1; t < NUM_PROD + 1; t++)
        pthread_create(&prod[t - 1], NULL, produtor, (void *)t);
    for (t = 1; t < NUM_CONS + 1; t++)
        pthread_create(&cons[t - 1], NULL, consumidor, (void *)t);

    for (t = 0; t < NUM_PROD; t++)
        pthread_join(prod[t], NULL);
    for (t = 0; t < NUM_CONS; t++)
        pthread_join(cons[t], NULL);

    printf("Trabalho concluído.\n");
    pthread_exit(NULL);
}

void *produtor(void *num)
{
    int i;
    for (i = 0; i < MAXNUM; i++)
    {
        pthread_mutex_lock(&mutex);

        while (qtd_na_fila == MAXFILA)
        {
            if (consumidor_terminado == NUM_CONS)
            {
                printf("Produtor %d abortando: fila cheia e consumidores terminaram.\n", (int)num);
                pthread_mutex_unlock(&mutex);
                pthread_exit(NULL);
            }
            pthread_cond_wait(&fila_nao_cheia, &mutex);
        }

        int item = (rand() % 1000) + 1;
        fila[fim_da_fila] = item;
        fim_da_fila = (fim_da_fila + 1) % MAXFILA;
        qtd_na_fila++;

        printf("Produtor %d produziu: %d (itens na fila: %d)\n", (int)num, item, qtd_na_fila);

        pthread_cond_signal(&fila_nao_vazia);
        pthread_mutex_unlock(&mutex);

        sleep(1);
    }

    printf("Produtor %d terminado.\n", (int)num);

    pthread_mutex_lock(&mutex);
    produtor_terminado++;
    pthread_cond_broadcast(&fila_nao_vazia);
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

void *consumidor(void *num)
{
    int i = 0;

    while (i < MAXNUM)
    {
        pthread_mutex_lock(&mutex);

        while (qtd_na_fila == 0 && !(produtor_terminado == NUM_PROD))
        {
            pthread_cond_wait(&fila_nao_vazia, &mutex);
        }

        if (qtd_na_fila == 0 && (produtor_terminado == NUM_PROD))
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        int item = fila[inicio_da_fila];
        inicio_da_fila = (inicio_da_fila + 1) % MAXFILA;
        qtd_na_fila--;
        i++;

        printf("Consumidor %d consumiu: %d (itens na fila: %d)\n", (int)num, item, qtd_na_fila);

        pthread_cond_signal(&fila_nao_cheia);
        pthread_mutex_unlock(&mutex);

        sleep(2);
    }

    printf("Consumidor %d terminou. Total consumido: %d\n", (int)num, i);

    pthread_mutex_lock(&mutex);
    consumidor_terminado++;
    pthread_cond_broadcast(&fila_nao_cheia);
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}