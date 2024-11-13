/* Francisco Teixeira Rocha Aragão
2021031726
Fundamentos de sistemas paralelos e distribuidos */


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

// Estrutura para salvar inforamções para controle de acesso a cada sala
struct sala {
    int contagemThreadsEspera;
    int contagemThreads;
    pthread_mutex_t mutex;
    pthread_cond_t existeTrio;
    pthread_cond_t salaVazia;
};

// Estrutura para salvar argumentos para usar em cada thread
// Essa estrutura foi usada pra facilitar o procesos de passagem de argumentos para cada thread (recebem void *)
struct args {
    int idThread;
    int tempoInicial;
    int numSalasVisitadas;
    int *idSalasTrajeto;
    int *tempoMinSalaDecimosSeg;
    struct sala *salas;
};

// funcao para realizar o trajeto de cada uma das threads
/*
Funcionamento:
    - Passa tempo inicial de cada thread antes de iniciar o trajeto
    - Threads esperam sala ficar vazia para formar trio
    - Threads esperam formação de trio
    - Threads entram na sala
    - Passa tempo na sala
    - Threads saem da sala

Sincronização:
    - A sincronização ocorre com uso de um mutex, além de variaveis de condição e variáveis de contagem
    - Antes de entrar e sair, travo o mutex
    - Quando threads entram e saem das salas, atualizo o valor das variaveis de contagem que estão presentes em cada uma das salas
    - Dependendo do valor da contagem, coordeno as condições e movimentações nas salas
    - Para entrar na sala:
        - sala deve estar vazia, ou seja, todas as threads sairam (contagemThreads == 0)
        - deve existir trio de threads esperando para entrar na sala (contagemThreadsEspera == 3)
    - Quando sala está vazia, sinalizo condição para threads poderem esperar (salaVazia)
    - Quando trio é formado a condição é sinalizada e as threads entram (existeTrio)
    - Ao sair da sala:
        - diminuo contagem de threads na sala
        - se não existir mais threads na sala sinalizo condição para sala ficar vazia (salaVazia)
*/

void *trajeto_thread(void *args) {
    struct args *arg = (struct args *)args;
    int tid = arg->idThread;

    // Passa o tempo inicial antes de entrar na primeira sala
    passa_tempo(tid, 0, arg->tempoInicial);

    for (int j = 0; j < arg->numSalasVisitadas; j++) {
        int salaAtual = arg->idSalasTrajeto[j];

        struct sala *sala = &arg->salas[salaAtual];  
        pthread_mutex_lock(&sala->mutex);

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
            sala->contagemThreads = 3;
        }
        // como as duas condições para entrar na sala foram satisfeitas -> possível entrar na sala

        
        // libero mutex para permitir operações em paralelo
        // caso contrário -> thread entra e sai da sala em sequência já que possui mutex
        pthread_mutex_unlock(&sala->mutex); 

        passa_tempo(tid, salaAtual, arg->tempoMinSalaDecimosSeg[j]);

        // iniciando processo pra sair da sala
        pthread_mutex_lock(&sala->mutex);
        
        sala->contagemThreads--; // diminuo contagem de threads na sala
        
        if (sala->contagemThreads == 0) { // se não existir mais threads na sala sinalizo condição
            pthread_cond_broadcast(&sala->salaVazia);
        }

        pthread_mutex_unlock(&sala->mutex);
    }
    
    // liberando memoria alocada quando thread finaliza o seu trajeto de salas
    free(arg->idSalasTrajeto);
    free(arg->tempoMinSalaDecimosSeg);
    free(arg);
    
    return NULL; //threads retornam NULL para finalizar e serem agrupadas no join
}

int main() {

    // Criando variáveis para armazenar informações e threads
    int numSalas, numThreads;
    // Iniciando recebimento dos parametros
    scanf("%d %d", &numSalas, &numThreads);
    
    struct sala salas[numSalas + 1];
    pthread_t threads[numThreads];

    // Inicializando estruturas de controle de cada sala: mutex, variáveis de condição e variáveis de contagem
    for (int i = 0; i < numSalas + 1; i++) { // uso +1 para facilitar acesso aos indices das salas que se inicia em 1.
        salas[i].contagemThreadsEspera = 0;
        salas[i].contagemThreads = 0;
        pthread_mutex_init(&salas[i].mutex, NULL);
        pthread_cond_init(&salas[i].existeTrio, NULL);
        pthread_cond_init(&salas[i].salaVazia, NULL);
    }

    // Estrutura para salvar argumentos para cada thread
    struct args *argsArray[numThreads];

    // Receber restante dos parametros e armazenar
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
        struct args *arg = argsArray[i];
        pthread_create(&threads[i], NULL, trajeto_thread, arg); // Create thread
    }


    // Aguardar conclusão das threads
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
