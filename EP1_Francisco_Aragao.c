/* 
Francisco Teixeira Rocha Aragão
2021031726
Fundamentos de sistemas paralelos e distribuidos 

Referências usadas:
- https://www.geeksforgeeks.org/mutex-lock-for-linux-thread-synchronization/
- https://stackoverflow.com/questions/1105745/pthread-mutex-assertion-error
- Função 'passa_tempo' fornecida no enunciado do trabalho
- https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html
*/


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
    //printf("------ Passa tempo recebeu da th %d sala %d tempo %d\n",tid,sala,decimos);
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

// estrutura para salvar inforamções para controle de acesso a cada sala
struct sala {
    int contagemThreadsEspera;
    int contagemThreads;
    pthread_mutex_t mutex;
    pthread_cond_t existeTrio;
    pthread_cond_t salaVazia;
};

// estrutura para salvar argumentos para usar em cada thread
//essa estrutura foi usada pra facilitar o procesos de passagem de argumentos para cada thread (recebem void *)
struct args {
    int idThread;
    int tempoInicial;
    int numSalasVisitadas;
    int *idSalasTrajeto;
    int *tempoMinSalaDecimosSeg;
    struct sala *salas;
};

/*
Funcionamento:
    - Passa tempo inicial de cada thread antes de iniciar o trajeto
    - Threads esperam sala ficar vazia para formar trio
    - Threads esperam formação de trio
    - Threads entram na sala
        - Threads liberam sala visitada anteriormente (se houver)
    - Passa tempo na sala
    - Se for o fim do trajeto
        - Threads liberam a última sala visitada
    - Libera memória alocada

Sincronização:
    - A sincronização ocorre com uso de um mutex para cada sala, além de variaveis de condição e variáveis de contagem
    - Antes de entrar e sair, mutex é travado e destravado
    - Quando threads entram e saem das salas, atualzo o valor das variaveis de contagem que estão presentes em cada uma das salas
    - Dependendo do valor da contagem, coordeno as condições e movimentações nas salas
    - Para entrar na sala:
        - sala deve estar vazia, ou seja, todas as threads sairam (contagemThreads == 0)
        - deve existir trio de threads esperando para entrar na sala (contagemThreadsEspera == 3)
    - Quando sala está vazia, sinalizo condição para threads poderem esperar (salaVazia)
    - Quando trio é formado a condição é sinalizada e as threads entram (existeTrio)
    - Ao entrar na sala, as threads liberam a posição ocupada na sala anterior:
        - diminuo contagem de threads na sala anterior
        - se não existir mais threads na sala anterior sinalizo condição para sala ficar vazia (salaVazia)
*/

// função pra entrar na sala caso for possível
void entra_sala(struct sala *sala) {

    // espera sala ficar vazia para poder formar trio
    // uso 'while' pois pode ocorrer da condição ser sinalizada e ao realizar a verificação novamente a sala não estar mais vazia
    while(sala->contagemThreads != 0) {
        pthread_cond_wait(&sala->salaVazia, &sala->mutex);
    }

    // espera formação de trio
    // uso 'if' nesse caso ao inves do 'while' pois agora preciso tratar caso de existir ou não existir o trio
    sala->contagemThreadsEspera++;
    
    if (sala->contagemThreadsEspera < 3) {
        pthread_cond_wait(&sala->existeTrio, &sala->mutex);
    } else {
        sala->contagemThreadsEspera = 0;
        pthread_cond_broadcast(&sala->existeTrio);
    }

    // como threads formaram trio na sala vazia, aumento a contagem de threads na sala atual
    sala->contagemThreads++;

}

// função pra sair da sala caso existir uma sala anterior
void sai_sala_anterior(int salaAnterior, struct args *arg) {
    if (salaAnterior != -1) {
        struct sala *salaAnt = &arg->salas[salaAnterior];
        pthread_mutex_lock(&salaAnt->mutex);
        salaAnt->contagemThreads--;

        // se é a ultima thread na sala, então a condição de sala vazia é sinalzada
        if (salaAnt->contagemThreads == 0) {
            pthread_cond_broadcast(&salaAnt->salaVazia);
        }
        pthread_mutex_unlock(&salaAnt->mutex);
    }

}

// funcao para realizar o trajeto de cada uma das threads
void *trajeto_thread(void *args) {
    struct args *arg = (struct args *)args;
    int tid = arg->idThread;
    int salaAnterior = -1; // controle da sala anterior (nenhuma no início)

    // Passa o tempo inicial antes de entrar na primeira sala
    passa_tempo(tid, 0, arg->tempoInicial);

    for (int j = 0; j < arg->numSalasVisitadas; j++) {
        int salaAtual = arg->idSalasTrajeto[j];

        struct sala *sala = &arg->salas[salaAtual];

        pthread_mutex_lock(&sala->mutex);

        entra_sala(sala);
        
        pthread_mutex_unlock(&sala->mutex);

        // libera a sala anterior (se existir)
        sai_sala_anterior(salaAnterior, arg);

        salaAnterior = salaAtual;

        // agora que entrei na sala e sai da anterior -> passo tempo na sala atual
        passa_tempo(tid, salaAtual, arg->tempoMinSalaDecimosSeg[j]);
    }
    // fim do trajeto

    // libera a última sala visitada
    sai_sala_anterior(salaAnterior, arg);

    // libero memória alocada
    free(arg->idSalasTrajeto);
    free(arg->tempoMinSalaDecimosSeg);
    free(arg);

    return NULL;
}

int main() {

    
    int numSalas, numThreads;
    
    scanf("%d %d", &numSalas, &numThreads);
    
    // estrutura pra armazenar informações das salas
    struct sala salas[numSalas + 1];
    pthread_t threads[numThreads];

    // inicializando estruturas de controle de cada sala: mutex, variáveis de condição e variáveis de contagem
    // uso +1 para facilitar acesso aos indices das salas que se inicia em 1.
    // tive alguns problemas com acesso a indices ( SALA - 1 ) então resolvi criar uma posição a mais
    for (int i = 0; i < numSalas + 1; i++) { 
        salas[i].contagemThreadsEspera = 0;
        salas[i].contagemThreads = 0;
        pthread_mutex_init(&salas[i].mutex, NULL);
        pthread_cond_init(&salas[i].existeTrio, NULL);
        pthread_cond_init(&salas[i].salaVazia, NULL);
    }

    // estrutura para salvar argumentos para cada thread
    struct args *argsArray[numThreads];

    //recebendo restante dos parametros
    for (int i = 0; i < numThreads; i++) {
        int tId, tempoInicial, numSalasVisitadas;
        scanf("%d %d %d", &tId, &tempoInicial, &numSalasVisitadas);

        struct args *arg = malloc(sizeof(struct args));
        arg->idThread = tId;
        arg->numSalasVisitadas = numSalasVisitadas;
        arg->idSalasTrajeto = malloc(numSalasVisitadas * sizeof(int));
        arg->tempoMinSalaDecimosSeg = malloc(numSalasVisitadas * sizeof(int));
        arg->salas = salas;
        arg->tempoInicial = tempoInicial;

        for (int j = 0; j < numSalasVisitadas; j++) {
            scanf("%d %d", &arg->idSalasTrajeto[j], &arg->tempoMinSalaDecimosSeg[j]);
        }

        argsArray[i] = arg; //salvo cada respectivo argumetno em um vetor para usar posteriormente
        
    }

    // criando threads e passando parametros de cada trajeto
    for (int i = 0; i < numThreads; i++) {
        pthread_create(&threads[i], NULL, trajeto_thread, argsArray[i]); 
    }


    //aguardar conclusão das threads
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }   

    // liberar mutex e variáveis de condição
    for (int i = 0; i < numSalas; i++) {
        pthread_mutex_destroy(&salas[i].mutex);
        pthread_cond_destroy(&salas[i].existeTrio);
        pthread_cond_destroy(&salas[i].salaVazia);
    }

    return 0;
}
