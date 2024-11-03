#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/*********************************************************
 Inclua o código a seguir no seu programa, sem alterações.
 Dessa forma a saída automaticamente estará no formato esperado 
 pelo sistema de correção automática.
 *********************************************************/

void passa_tempo(int tid, int sala, int decimos)
{
    struct timespec zzz, agora;
    static struct timespec inicio = {0,0};
    int tstamp;

    if ((inicio.tv_sec == 0)&&(inicio.tv_nsec == 0)) {
        clock_gettime(CLOCK_REALTIME,&inicio);
    }

    zzz.tv_sec  = decimos/10;
    zzz.tv_nsec = (decimos%10) * 100L * 1000000L;

    if (sala==0) {
        nanosleep(&zzz,NULL);
        return;
    }

    clock_gettime(CLOCK_REALTIME,&agora);
    tstamp = ( 10 * agora.tv_sec  +  agora.tv_nsec / 100000000L )
            -( 10 * inicio.tv_sec + inicio.tv_nsec / 100000000L );

    // printf("%3d [ %2d @%2d z%4d\n",tstamp,tid,sala,decimos);
    printf("Tempo entrada: %3d [ Thread: %2d @ Sala: %2d z Duracao:%4d\n",tstamp,tid,sala,decimos);

    nanosleep(&zzz,NULL);

    clock_gettime(CLOCK_REALTIME,&agora);
    tstamp = ( 10 * agora.tv_sec  +  agora.tv_nsec / 100000000L )
            -( 10 * inicio.tv_sec + inicio.tv_nsec / 100000000L );

    //printf("%3d ) %2d @%2d\n",tstamp,tid,sala);
    printf("Tempo saida: %3d ) Thread: %2d @ Sala: %2d\n",tstamp,tid,sala);
}
/*********************** FIM DA FUNÇÃO *************************/

struct sala {
    int contagemThreadsEspera;
    int contagemThreads;
    pthread_mutex_t mutex;
    pthread_cond_t existeTrio;
    pthread_cond_t salaVazia;
};

struct args {
    int idThread;
    int numSalasVisitadas;
    int *idSalasTrajeto;
    int *tempoMinSalaDecimosSeg;
    struct sala *salas;
};

void *trajeto_thread(void *args) {
    struct args *arg = (struct args *)args;
    int tid = arg->idThread;

    for (int j = 0; j < arg->numSalasVisitadas; j++) {
        int salaAtual = arg->idSalasTrajeto[j];

        // Espera para formar trio na sala
        struct sala *sala = &arg->salas[salaAtual];
        pthread_mutex_lock(&sala->mutex);

        sala->contagemThreadsEspera++;
        if (sala->contagemThreadsEspera < 3) {
            printf("Thread %d esperando trio na sala %d\n", tid, salaAtual);
            pthread_cond_wait(&sala->existeTrio, &sala->mutex);
        } else {
            printf("Trio formado na sala %d após thread %d entrar.\n", salaAtual, tid);
            sala->contagemThreadsEspera = 0;
            pthread_cond_broadcast(&sala->existeTrio);
        }

        // Entrar na sala
        sala->contagemThreads++;
        pthread_mutex_unlock(&sala->mutex);

        passa_tempo(tid, salaAtual, arg->tempoMinSalaDecimosSeg[j]);

        // Sair da sala
        pthread_mutex_lock(&sala->mutex);
        sala->contagemThreads--;
        if (sala->contagemThreads == 0) {
            pthread_cond_broadcast(&sala->salaVazia);
        }
        pthread_mutex_unlock(&sala->mutex);
    }

    free(arg->idSalasTrajeto);
    free(arg->tempoMinSalaDecimosSeg);
    free(arg);
    return NULL;
}

int main() {
    int numSalas, numThreads;
    scanf("%d %d", &numSalas, &numThreads);

    struct sala salas[numSalas];
    pthread_t threads[numThreads];

    // Inicializar mutexes e variáveis de condição
    for (int i = 0; i < numSalas; i++) {
        salas[i].contagemThreadsEspera = 0;
        salas[i].contagemThreads = 0;
        pthread_mutex_init(&salas[i].mutex, NULL);
        pthread_cond_init(&salas[i].existeTrio, NULL);
        pthread_cond_init(&salas[i].salaVazia, NULL);
    }

    // Criar threads
    for (int i = 0; i < numThreads; i++) {
        int tId, tempoInicial, numSalasVisitadas;
        scanf("%d %d %d", &tId, &tempoInicial, &numSalasVisitadas);

        struct args *arg = malloc(sizeof(struct args));
        arg->idThread = tId;
        arg->numSalasVisitadas = numSalasVisitadas;
        arg->idSalasTrajeto = malloc(numSalasVisitadas * sizeof(int));
        arg->tempoMinSalaDecimosSeg = malloc(numSalasVisitadas * sizeof(int));
        arg->salas = salas;

        for (int j = 0; j < numSalasVisitadas; j++) {
            scanf("%d %d", &arg->idSalasTrajeto[j], &arg->tempoMinSalaDecimosSeg[j]);
        }

        passa_tempo(tId, 0, tempoInicial);

        pthread_create(&threads[i], NULL, trajeto_thread, arg);
    }

    // Aguardar conclusão das threads
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Destruir mutexes e variáveis de condição
    for (int i = 0; i < numSalas; i++) {
        pthread_mutex_destroy(&salas[i].mutex);
        pthread_cond_destroy(&salas[i].existeTrio);
        pthread_cond_destroy(&salas[i].salaVazia);
    }

    return 0;
}