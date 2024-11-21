#include <stdio.h>
#include <stdlib.h>


/* Configuracoes gerais do Escalonador */
#define MAX_PROCESSOS                     50  
#define QUANTUM                           6                       

/* Define o tempo necessario para cada tipo de I/O */
#define IO_DISCO_TEMPO                    4                
#define IO_FITA_TEMPO                     5
#define IO_IMPRESSORA_TEMPO               2

/* Tipos de I/O */
typedef enum {
    DISCO,
    FITA,
    IMPRESSORA
} IO; 

/* Estrutura que contem as informacoes individuais de cada I/O realizado por um processo */
typedef struct OperacaoIO{
    int inicio; /* inicio do tempo interno do processo em que comeca a operacao de I/O */
    IO tipo; /* tipo da operacao de I/O */
    struct OperacaoIO* prox; /* para encadear as operacoes em uma lista na PCB */
} OperacaoIO;

/* Tipos sobre o Process Control Block */
typedef struct {
    int PID; /* id do processo */
    int instante_de_ativacao;  /*  instante em que iniciou   */
    int fim; /* instante em que terminou */
    int tempo_restante; /* tempo que falta para o processo terminar */
    int tempo_cpu; /* tempo total de cpu, informado no arquivo*/
    int tempo_interno; /* tempo de cpu rodado*/
    int status; /* para verificar se está sendo rodado */
    OperacaoIO* inicio_io;   /* ponteiro para o inicio da lista de operacoes de I/O do processo, se nao tiver sera NULL */
    OperacaoIO* final_io;    /* ponteiro para o final da lista de operacoes de I/O do processo */
    int numOperacoesIO; /* numero total de operacoes de I/O realizadas pelo processo */

} PCB;

/* Definicao da estrutura do no pertencente as fila */
typedef struct _No {
    PCB* processo; /* ponteiro para o PCB */
    struct _No* prox;
} No;

/* Definicao da estrutura da fila */
typedef struct Fila {
    No* inicio; 
    No* fim; 
} Fila;

/* Métodos da fila */
void filaInit(Fila* fila);
void filaInsere(Fila* fila, PCB* processo);
PCB* filaRemove(Fila* fila);
int estaVazia(Fila *fila);

/* Tipos de fila */
Fila espera;
Fila alta;
Fila baixa;
Fila disco;
Fila fita;
Fila impressora; 
Fila finalizados; /* cemitério de processos */

typedef struct _NoResumo {
    int numCiclo;
    int PID;
    struct _NoResumo* ant;
    struct _NoResumo* prox;
} NoResumo;

typedef struct {
    NoResumo* inicio; 
    NoResumo* fim; 
} Resumo;

int numPIDs = 0; /* Quantidade de processos encontrados pelo escalonador */
Resumo resumo;
void addResumo(Resumo* resumo, int PID, int numQuantum); /* salva quem foi executado e quando (em ordem!). Se ninguem foi executado, passar o PID como -1 */

int tempoSistema; /* contador geral de tempo total p/ o turnaround */

/* Métodos do escalonador */

void escalona(PCB* processo, Fila* fila);
void roundRobin();
void LeProcessos(FILE *file); /* le os processos do arquivo */
void iniciaProcessos(); /* move os processos para a fila de alta no seu instante de ativacao */
void displayProcesso(PCB* processo); /* mostra um processo individualmente */
void displayProcessos(Fila *fila); /* mostra todos os processos de uma fila */
void addOperacaoIO(PCB* processo, int inicio, int codigo); /* adiciona uma operacao de I/O especifica no processo */
void IOHandler(PCB* processo); /* Lida com IO, supondo que ha IO */
void fimDeIO(); /* Verifica se houve fim de IO em qualquer uma das filas de IO */
void displayGrafico(Resumo* resumo, int numPIDs);


int main(int argc, char* argv[]) {
    FILE* file;

    if (argc != 2) {
        printf("Uso: %s <nome_do_arquivo>\n", argv[0]);
        return 1;
    }

    file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo");
        return 1;
    }
    
    filaInit(&espera);
    filaInit(&alta);
    filaInit(&baixa);
    filaInit(&disco);
    filaInit(&fita);
    filaInit(&impressora);
    resumo.inicio = NULL;
    resumo.fim = NULL;
    tempoSistema = 1;

    printf("====================================================\n");
    printf("|| Bem-vindo ao Simulador Round Robin com feedback ||\n");
    printf("====================================================\n");
    printf("\n\n\n");
    
    LeProcessos(file);

    printf("=============================================\n");
    printf("||   PROCESSOS INSERIDOS NO ESCALONADOR:    ||\n");
    printf("=============================================\n");
    printf("\n");
    
    displayProcessos(&espera);

    printf("=============================================\n");
    printf("||  O escalonamento dos processos começará! ||\n");
    printf("=============================================\n");
    printf("\n");

    roundRobin();

    return 0; 
}

/* Faz a leitura dos processos no arquivo */
void LeProcessos(FILE *file) {
    int PID, instante_de_ativacao, tempo_cpu, inicio, codigo;
    char caracter;
    PCB* processo;
    int i;
    printf("\n OBS: O escalonador só lê até 50 processos!\n");
    for(i = 0; i < MAX_PROCESSOS; i++){
            while (fscanf(file, "%d %d %d", &PID, &instante_de_ativacao, &tempo_cpu) == 3) {
            processo = (PCB*)malloc(sizeof(PCB));
            processo->instante_de_ativacao = instante_de_ativacao;
            processo->PID = PID;
            processo->tempo_cpu = tempo_cpu;
            while (1) {
                int result = fscanf(file, "%d-%d", &inicio, &codigo);

                if (result == 2) {
                    addOperacaoIO(processo, inicio, codigo);

                    fscanf(file, "%c", &caracter);

                    if (caracter == ' ') {
                        continue;
                    } 
                    if (caracter == '\n') {
                        filaInsere(&espera, processo);
                        break;
                    }
                }
                else {
                    filaInsere(&espera, processo);
                    break;
                }
            }
        }

    }
  
}

void iniciaProcessos() {
    PCB* processo;
    while (espera.fim != NULL && espera.inicio->processo->instante_de_ativacao == tempoSistema) {
        processo = filaRemove(&espera);
        filaInsere(&alta, processo);
    }
}

/* Inicializa uma fila */
void filaInit(Fila* fila) {
    fila->inicio = NULL; 
    fila->fim = NULL;
}

void filaInsere(Fila* fila, PCB* processo) {
    No* novo = (No*)malloc(sizeof(No)); /* cria o novo no */
    
    if (novo == NULL) {
        printf("Sem memória disponivel\n");
        exit(1);
    }

    novo->processo = processo; 
    novo->prox = NULL;
    novo->processo->status = 0;
 
    if (fila->inicio == NULL) { /* coloca na fila */
        fila->inicio = novo; /* caso fila vazia  */
        fila->fim = novo;

    } else {
        /* caso fila nao vazia */
        fila->fim->prox = novo; /* linka na fila */
        fila->fim = novo;       /* atualiza a referencia do fim para o novo */ 

    }
}

PCB* filaRemove(Fila* fila) {
    PCB* primeiro;

    /*  Verifica se ha pelo menos um no */
    if (fila->inicio == NULL) {
        printf("Fila vazia!!!\n");
        return NULL;
    }
    else{
        primeiro = fila->inicio->processo; /* Pega o primeiro processo */
        fila->inicio = fila->inicio->prox; /* Inicio passa a apontar para o próximo */
    }
    
    /* Checa se a fila ficou vazia */ 
    if (fila->inicio == NULL) {
        fila->fim = NULL;
    }

    return primeiro;
}

int estaVazia(Fila *fila) {
    return (fila->inicio == NULL) ? 1 : 0; /* Retorna 1 se estiver vazia e 0 se estiver com pelo menos um processo */
}

void addResumo(Resumo* resumo, int PID, int numQuantum) {
    /* Criar um novo nó */
    NoResumo* novoResumo = (NoResumo*) malloc(sizeof(NoResumo));
    if (novoResumo == NULL) {
        fprintf(stderr, "Erro ao alocar memória para o novo nó.\n");
        exit(EXIT_FAILURE);
    }

    /* Preencher os dados do nó */
    novoResumo->PID = PID;
    novoResumo->numCiclo = numQuantum;  /* Usando o tempo atual como exemplo */
    novoResumo->prox = NULL;

    /* Verificar se a fila está vazia */
    if (resumo->inicio == NULL) {
        novoResumo->ant = NULL;
        resumo->inicio = novoResumo;
        resumo->fim = novoResumo;
    } else {
        /* Encadear */
        novoResumo->ant = resumo->fim;
        resumo->fim->prox = novoResumo;
        resumo->fim = novoResumo;
    }
}

void displayGrafico(Resumo* resumo, int numPIDs) {
    int i,j;
    NoResumo* atual;
    PCB* processo;
    No* ptNo;


    /* Imprimir cabeçalhos de coluna */
    printf("|  u.t  |");
    for (i = 1; i <= numPIDs; i++) {
        printf(" %2d |", i);
    }
    printf("\n");

    /* Imprimir linha separadora */
    printf("+-------+");
   
    for (j = 1; j <= numPIDs; j++) {
        printf("----+");
    }
    printf("\n");

    /* Imprimir tabela de execução */
    atual = resumo->inicio;
    while (atual != NULL) {
        /* Imprimir tempo atual */
        printf("| %5d |", atual->numCiclo);

        /* Imprimir marca para o processo executado */
        for (i = 1; i <= numPIDs; i++) {
            if (atual->PID == i) {
                printf("  # |");
            } else {
                printf("    |");
            }
        }
        printf("\n");

        /* Avançar para o próximo nó na fila */
        atual = atual->prox;
    }

    printf("\n");
    printf("DADOS DO ESCALONAMENTO\n");
    printf("----------------------\n");
       printf("Em ordem de quem terminou primeiro:\n");
    printf("\n");
    
    /* Percorre a lista */
    ptNo = finalizados.inicio;
    while (ptNo != NULL) {
        processo = ptNo->processo;
        printf("PID %2d: Tournaround: %3d, Tempo de CPU: %3d\n", processo->PID, processo->fim - processo->instante_de_ativacao, processo->tempo_cpu);

        ptNo = ptNo->prox;
    }
}

/* Funcao para adicionar um novo no a lista */
void addOperacaoIO(PCB* processo, int inicio, int codigo) {
    OperacaoIO *novoIO = (OperacaoIO *)malloc(sizeof(OperacaoIO)); novoIO->inicio = inicio;
    
    if(processo->inicio_io == NULL){
        processo->inicio_io = novoIO;
        processo->final_io = novoIO;
        novoIO->prox = NULL;
    }
    else{
        processo->final_io->prox = novoIO;
        processo->final_io = novoIO;
        novoIO->prox = NULL; 
    }
    switch(codigo){
        case 0:
            novoIO->tipo = DISCO;
        break;
        case 1:
            novoIO->tipo = FITA;
        break;
        case 2:
            novoIO->tipo = IMPRESSORA;
        break;
    }
}


void displayProcesso(PCB* processo) {
    OperacaoIO* pt = processo->inicio_io;
    printf("PID: %d\n", processo->PID);
    /* printf("Tempo CPU: %d\n", processo->tempo_cpu); */
    /* printf("Tempo restante: %d\n", processo->tempo_restante); */
   
    if(pt == NULL){
        printf("Não tem I/O.\n");
    }
    while (pt != NULL) {
        printf("Inicio I/O: %d, Codigo: %d\n", pt->inicio, pt->tipo); 
        pt = pt->prox; }
    
    printf("\n");
}

void displayProcessos(Fila* fila) {
    if(fila->inicio == NULL){
        printf("Não tem ninguém na fila.\n");
    }
    else{
        No* pt = fila->inicio;
        while (pt->processo != fila->fim->processo) {
            displayProcesso(pt->processo);
            pt = pt->prox;
        }
        displayProcesso(pt->processo);
    }
}

void printInfoProcesso(PCB* processo) {
    printf("\nUnidade de tempo: %d \n", tempoSistema);
    printf("PID: %d\n", processo->PID);
    printf("Status: %d\n", processo->status);
    printf("Tempo restante %d\n",processo->tempo_restante);
    printf("Tempo interno %d\n",processo->tempo_interno);
    printf("\n");
}


void IOHandler(PCB* processo) {
    /* Switch case pra ver qual IO e por no final da fila com processo_atual->io->tipo */
    switch(processo->inicio_io->tipo){
        case DISCO:
            filaInsere(&disco,processo);
            processo->tempo_restante = IO_DISCO_TEMPO; 
        break;

        case FITA:
            filaInsere(&fita,processo);
            processo->tempo_restante = IO_FITA_TEMPO;
        break;

        case IMPRESSORA: 
            filaInsere(&impressora,processo);
            processo->tempo_restante = IO_IMPRESSORA_TEMPO; 
        break;
    }
    processo->inicio_io = processo->inicio_io->prox; /* Roda a fila interna de IO do processo */
}

void fimDeIO() {
    PCB* processo;

    /* Fila DISCO */
    if(disco.inicio != NULL) {
        if (disco.inicio->processo->tempo_restante == 0) {
            processo = filaRemove(&disco);  /* Remove da Fila de IO */
            filaInsere(&baixa, processo);   /* Insere na Fila correta */
        }
        else {
            disco.inicio->processo->tempo_restante--;
        }
    }

    /* Fila FITA */
    if(fita.inicio != NULL) {
        if (fita.inicio->processo->tempo_restante == 0) {
            processo = filaRemove(&fita);  
            filaInsere(&alta, processo);
        }
        else {
            fita.inicio->processo->tempo_restante--;
        }
    }

    /* Fila IMPRESSORA */
    if(impressora.inicio != NULL) {
        if (impressora.inicio->processo->tempo_restante == 0) {
            processo = filaRemove(&impressora);  
            filaInsere(&alta, processo);
        } 
        else {
            impressora.inicio->processo->tempo_restante--;
        }
    }
}

void escalona(PCB* processo, Fila* fila) {
    processo->tempo_restante--;
    processo->tempo_interno++;
    
    /* verifica se ele tem IO e se o IO ta no tempo interno dele de acontencer */
    if(processo->inicio_io != NULL && processo->inicio_io->inicio == processo->tempo_interno){
        processo = filaRemove(fila);
        IOHandler(processo);
    }

    /* verifica se o processo ja acabou por completo */
    if(processo->tempo_interno == processo->tempo_cpu){
        processo = filaRemove(fila);
        processo->fim = tempoSistema;
        filaInsere(&finalizados, processo);
    }

    /* Verifica se o QUANTUM acabou */
    else if(processo->tempo_restante == 0){
        processo = filaRemove(fila);
        filaInsere(&baixa, processo);
    }
    
}

void roundRobin() {
    PCB* processo_atual;

    int timer_processo = 0;
    int timer_disco = 0;
    int timer_fita = 0;
    int timer_impressora = 0;

    /* Enquanto pelo menos uma das filas nao estiver vazia, roda */
    while(!(estaVazia(&alta)) || !(estaVazia(&baixa)) || !(estaVazia(&fita)) || !(estaVazia(&disco)) || !(estaVazia(&impressora)) || !(estaVazia(&espera))){

        iniciaProcessos();

        /* Verifica se ha algum processo na fila de alta prioridade */
        if(!estaVazia(&alta)) {
            processo_atual = alta.inicio->processo;
            if (processo_atual->status == 0) {
                processo_atual->status = 1;
                processo_atual->tempo_restante = QUANTUM;
            }
            escalona(processo_atual, &alta);
            addResumo(&resumo, processo_atual->PID, tempoSistema);
        }

        /* Fila de baixa */
        else if(!estaVazia(&baixa)){
            processo_atual = baixa.inicio->processo;
            if (processo_atual->status == 0) {
                processo_atual->status = 1;
                processo_atual->tempo_restante = QUANTUM;
            }
            escalona(processo_atual, &baixa);
            addResumo(&resumo, processo_atual->PID, tempoSistema);
        } 

        /* Ambas as filas vazias */
        else {
            addResumo(&resumo, -1, tempoSistema);
        }

        timer_processo++;
        timer_disco++;
        timer_fita++;
        timer_impressora++;
        tempoSistema++;
        
        printInfoProcesso(processo_atual);

        printf("===================================================\n");
        printf("||  Lista de processos na fila de ESPERA          ||\n");
        printf("===================================================\n");
        printf("\n");
        displayProcessos(&alta);

        printf("===================================================\n");
        printf("||  Lista de processos na fila de ALTA prioridade  ||\n");
        printf("===================================================\n");
        printf("\n");
        displayProcessos(&alta);

        printf("===================================================\n");
        printf("||  Lista de processos na fila de BAIXA prioridade ||\n");
        printf("===================================================\n");
        printf("\n");
        displayProcessos(&baixa);

        printf("===================================================\n");
        printf("||  Lista de processos na fila de FINALIZADOS     ||\n");
        printf("===================================================\n");
        printf("\n");
        displayProcessos(&finalizados);

        printf("===================================================\n");
        printf("||  Lista de processos na fila de DISCO (I/O)    ||\n");
        printf("===================================================\n");
        printf("\n");
        displayProcessos(&disco);

        printf("===================================================\n");
        printf("||   Lista de processos na fila de FITA (I/O)    ||\n");
        printf("===================================================\n");
        printf("\n");
        displayProcessos(&fita);

        printf("===================================================\n");
        printf("||   Lista de processos na fila de IMPRESSORA (I/O) ||\n");
        printf("===================================================\n");
        printf("\n");
        displayProcessos(&impressora);

        /* Verifica se alguém finalizou o IO */
        fimDeIO();
        
        /* Verifica a quantidade de processos (PIDs precisam ser concedidos em ordem) */
        if (processo_atual->PID > numPIDs) numPIDs = processo_atual->PID;
    }
    printf("\n\nFIM DO ESCALONAMENTO: NÃO há processos a serem escalonados.\n");

    printf("\n\n\n");
    printf("===================================================\n");
    printf("||            GRÁFICO DO ESCALONAMENTO             ||\n");
    printf("===================================================\n");
    printf("\n");
    displayGrafico(&resumo, numPIDs);
}

