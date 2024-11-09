#include <stdio.h>
#include <stdlib.h> // evitar warning implicit declaration of function ‘malloc’
#include <time.h>
#include <pthread.h>

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
    int id;
    int contagemThreadsEspera;
    int contagemThreads; 
};

struct args {
    int idThread;
    pthread_mutex_t mutex;
    pthread_cond_t existeTrio;
    pthread_cond_t salaVazia;
    struct sala salas;
};

void *entraProximaSala(void *args) {

    struct args *arguments;

    arguments = (struct args *) args;
    
    pthread_mutex_lock(&arguments->mutex);

    //printf("Sala %d possui espera %d\n", arguments->salas.id, arguments->salas.contagemThreadsEspera);

    // espero formação do trio
    if (arguments->salas.contagemThreadsEspera != 3) {
        arguments->salas.contagemThreadsEspera++;
        printf("Th %d esperando. Contagem %d\n", arguments->idThread, arguments->salas.contagemThreadsEspera);
        pthread_cond_wait(&arguments->existeTrio, &arguments->mutex);
        printf("Sala formou trio, entrando...\n");
    } else { // tenho 3 threads esperando
        arguments->salas.contagemThreadsEspera = 0;
        pthread_cond_broadcast(&arguments->existeTrio);
    }


    // espero sala estar vazia pra eu entrar
    while(arguments->salas.contagemThreads != 0) {
        pthread_cond_wait(&arguments->salaVazia, &arguments->mutex);
    }

    printf("Entrando na sala th %d\n", arguments->salas.id);

    arguments->salas.contagemThreads = 3;

    pthread_mutex_unlock(&arguments->mutex);

    return NULL;
}

void *saiSalaAnterior(void *args) {

    struct args *arguments;

    arguments = (struct args *) args;

    pthread_mutex_lock(&arguments->mutex);
    
    arguments->salas.contagemThreads --;

    if (arguments->salas.contagemThreads == 0) {
        pthread_cond_broadcast(&arguments->salaVazia);
    }

    pthread_mutex_unlock(&arguments->mutex);

    return NULL;
}



int main () {

    // Inicio e recebimento dos parametros 

    pthread_mutex_t mutex;
    pthread_cond_t existeTrio;
    pthread_cond_t salaVazia;

    int numSalas;
    int numThreads;

    scanf("%d %d", &numSalas, &numThreads);

    int tId[numThreads];
    int tempoInicialDecimosSeg[numThreads];
    int numSalasVisitadas[numThreads];

    int *idSalasTrajeto[numThreads];
    int *tempoMinSalaDecimosSeg[numThreads];

    for (int i = 0; i < numThreads; i++) {
        scanf("%d %d %d", &tId[i], &tempoInicialDecimosSeg[i], &numSalasVisitadas[i]);

        int salasVisitadas = numSalasVisitadas[i];

        idSalasTrajeto[i] = (int *) malloc(salasVisitadas * sizeof(int));
        tempoMinSalaDecimosSeg[i] = (int *) malloc(salasVisitadas * sizeof(int));        

        for (int j = 0; j < salasVisitadas; j++) {
            scanf("%d %d", &idSalasTrajeto[i][j], &tempoMinSalaDecimosSeg[i][j]);
        }
    }
    // Imprimir os parametros recebidos
    /* printf("\nnumSalas: %d\n", numSalas);
    printf("numThreads: %d\n", numThreads);
    for (int i = 0; i < numThreads; i++) {
        printf("tId[%d]: %d\n", i, tId[i]);
        printf("tempoInicialDecimosSeg[%d]: %d\n", i, tempoInicialDecimosSeg[i]);
        printf("numSalasVisitadas[%d]: %d\n", i, numSalasVisitadas[i]);
        for (int j = 0; j < numSalasVisitadas[i]; j++) {
            printf("idSalasTrajeto[%d][%d]: %d\n", i, j, idSalasTrajeto[i][j]);
            printf("tempoMinSalaDecimosSeg[%d][%d]: %d\n", i, j, tempoMinSalaDecimosSeg[i][j]);
        }
    }
    printf("\n"); */

    // inicializo struct salas, threads, mutex e condicao

    struct sala salas[numSalas];

    for (int i = 0; i < numSalas; i++) {
        salas[i].id = i;
        salas[i].contagemThreadsEspera = 0;
        salas[i].contagemThreads = 0;
    }

    pthread_t threads[numThreads];

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&existeTrio, NULL);
    pthread_cond_init(&salaVazia, NULL);

    struct args args;
    
    for (int i = 0; i < numThreads; i++) {
        
        // passa tempo inicial antes de iniciar o trajeto
        passa_tempo(tId[i], 0, tempoInicialDecimosSeg[i]);
        
        // percorro salas
        for (int j = 0; j < numSalasVisitadas[i]; j++) {
            
            int salaAnterior = 0;
            if (j != 0) {
                salaAnterior = idSalasTrajeto[i][j-1];
            }

            // organizando argumentos para passar para as threads
            args.mutex = mutex;
            args.existeTrio = existeTrio;
            args.salaVazia = salaVazia;
            args.salas = salas[idSalasTrajeto[i][j]]; // passo informacao da sala que irei acessar 
            args.idThread = tId[i];

            // se estou na ultima sala não preciso ir pra proxima
            if ( j != numSalasVisitadas[i] - 1) {
                printf("\nThread %d tenta entrar na sala %d\n", tId[i], idSalasTrajeto[i][j]);
                pthread_create(&threads[i], NULL, entraProximaSala, (void *) &args);
            }

            args.salas = salas[salaAnterior]; // passo informacao da sala que estou saindo

            // se estou na primeira sala não preciso sair da anterior
            if (j != 0) {
                printf("Thread %d tenta sair da sala %d\n", tId[i], salaAnterior);
                pthread_create(&threads[i], NULL, saiSalaAnterior, (void *) &args);
            }

            passa_tempo(tId[i], idSalasTrajeto[i][j], tempoMinSalaDecimosSeg[i][j]);
 		
	    // OLHAR ONDE USAR O FIM DAS THERADS, E TAMBÉM COMO DESTRUIR MUTEX E VAR DE CONDICAO
        }
    }

    return 0;
}
