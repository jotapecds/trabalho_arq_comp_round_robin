#include <stdio.h>
#include <stdlib.h>


/* Configuracoes gerais do Escalonador */
#define MAX_PROCESSOS                     5  
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
    IO tipo;
    int instante_inicio;
} OperacaoIO;

/* Tipos sobre o Process Control Block */
typedef struct {
    int PID; /* id do processo */
    int instante_de_ativacao;  /*  instante em que iniciou   */
    int tempo_cpu; /* tempo total de cpu, informado no arquivo*/
    int tempo_restante; /* tempo que falta para o processo terminar */
    int tempo_interno; /* tempo de cpu rodado*/
    int status; /* para verificar se está sendo rodado */
    OperacaoIO* operacao_io; /*  */

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

int numPIDs = 0; /* Quantidade de processos encontrados pelo escalonador */
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
    int PID, instante_de_ativacao, tempo_cpu, tipo_io, tempo_io;
    PCB* processo;
    OperacaoIO* operacao;

    while (fscanf(file, "%d %d %d %d %d", &PID, &instante_de_ativacao, &tempo_cpu, &tipo_io, &tempo_io) != EOF) 
    {
        processo = (PCB*) malloc(sizeof(PCB));
        
        if(processo == NULL) {
            printf("Erro de alocação de memória\n");
            exit(-1);
        }

        processo->PID = PID;
        processo->instante_de_ativacao = instante_de_ativacao;
        processo->tempo_cpu = tempo_cpu;

        operacao = (OperacaoIO*) malloc(sizeof(OperacaoIO));

        if(operacao == NULL) {
            printf("Erro de alocação de memória\n");
            exit(-1);
        }

        processo->operacao_io = operacao;
        processo->operacao_io->instante_inicio = tempo_io;

        switch(tipo_io)
        {
            case 0:
                processo->operacao_io->tipo = DISCO;
                break;

            case 1:
                processo->operacao_io->tipo = FITA;
                break;

            case 2:
                processo->operacao_io->tipo = IMPRESSORA;
                break;
        }

        filaInsere(&espera, processo);
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


void displayProcesso(PCB* processo) {
    printf("PID: %d\n", processo->PID);
    /* printf("Tempo CPU: %d\n", processo->tempo_cpu); */
    /* printf("Tempo restante: %d\n", processo->tempo_restante); */
   
    if(processo->operacao_io == NULL)
    {
        printf("Não tem I/O.\n");
    }

    printf("Inicio I/O: %d, Codigo: %d\n", processo->operacao_io->instante_inicio, processo->operacao_io->tipo); 
    
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

void displayPCB(PCB* processo) {
    printf("\nUnidade de tempo: %d \n", tempoSistema);
    printf("PID: %d\n", processo->PID);
    printf("Status: %d\n", processo->status);
    printf("Tempo restante %d\n",processo->tempo_restante);
    printf("Tempo interno %d\n",processo->tempo_interno);
    printf("\n");
}


void IOHandler(PCB* processo) {
    /* Switch case pra ver qual IO e por no final da fila com processo_atual->io->tipo */
    switch(processo->operacao_io->tipo){
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
    if(processo->operacao_io != NULL && processo->operacao_io->instante_inicio == processo->tempo_interno){
        processo = filaRemove(fila);
        IOHandler(processo);
    }

    /* verifica se o processo ja acabou por completo */
    if(processo->tempo_interno == processo->tempo_cpu){
        processo = filaRemove(fila);
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
        }

        /* Fila de baixa */
        else if(!estaVazia(&baixa)){
            processo_atual = baixa.inicio->processo;
            if (processo_atual->status == 0) {
                processo_atual->status = 1;
                processo_atual->tempo_restante = QUANTUM;
            }
            escalona(processo_atual, &baixa);
        } 

        timer_processo++;
        timer_disco++;
        timer_fita++;
        timer_impressora++;
        tempoSistema++;
        
        displayPCB(processo_atual);

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
}
