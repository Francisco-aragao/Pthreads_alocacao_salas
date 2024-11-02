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

    printf("%3d [ %2d @%2d z%4d\n",tstamp,tid,sala,decimos);

    nanosleep(&zzz,NULL);

    clock_gettime(CLOCK_REALTIME,&agora);
    tstamp = ( 10 * agora.tv_sec  +  agora.tv_nsec / 100000000L )
            -( 10 * inicio.tv_sec + inicio.tv_nsec / 100000000L );

    printf("%3d ) %2d @%2d\n",tstamp,tid,sala);
}
/*********************** FIM DA FUNÇÃO *************************/

struct sala {
    int id;
    int contagemThreadsEspera;
    int contagemThreads; 
};

struct args {
    pthread_mutex_t mutex;
    pthread_cond_t existeTrio;
    pthread_cond_t salaVazia;
    struct sala salas;
};

void *entraProximaSala(void *args) {

    struct args *arguments;

    arguments = (struct args *) args;
    
    pthread_mutex_lock(&arguments->mutex);

    // espero formação do trio
    if (arguments->salas.contagemThreadsEspera != 3) {
        arguments->salas.contagemThreadsEspera++;
        pthread_cond_wait(&arguments->existeTrio, &arguments->mutex);
    } else { // tenho 3 threads esperando
        arguments->salas.contagemThreadsEspera = 0;
        pthread_cond_broadcast(&arguments->existeTrio);
    }

    // espero sala estar vazia pra eu entrar
    while(arguments->salas.contagemThreads != 0) {
        pthread_cond_wait(&arguments->salaVazia, &arguments->mutex);
    }

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

    // TA DANDO RESULTADO ERRADO, CONFERIR O QUE FUNCAO PASSA TEMPO FAZ E CONFERIR CODIGO ABAIXO PRA ENTENDER TROCAS DE SALAS
    
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

            // se estou na ultima sala não preciso ir pra proxima
            if ( j != numSalasVisitadas[i] - 1) {
                pthread_create(&threads[i], NULL, entraProximaSala, (void *) &args);
            }

            args.salas = salas[salaAnterior]; // passo informacao da sala que estou saindo

            // se estou na primeira sala não preciso sair da anterior
            if (j != 0) {
                pthread_create(&threads[i], NULL, saiSalaAnterior, (void *) &args);
            }

            passa_tempo(tId[i], idSalasTrajeto[i][j], tempoMinSalaDecimosSeg[i][j]);
 		
	    // OLHAR ONDE USAR O FIM DAS THERADS, E TAMBÉM COMO DESTRUIR MUTEX E VAR DE CONDICAO
        }
    }

    return 0;
}
