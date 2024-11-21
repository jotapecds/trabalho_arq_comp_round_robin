#include "fila.h"
#include <stdlib.h>

void iniciaFila(Fila *fila)
{
    fila->inicio = NULL;
    fila->fim = NULL;
}

void addFila(Fila *fila, PCB *processo)
{
    No *novo = (No *)malloc(sizeof(No)); 

    if (novo == NULL)
    {
        printf("Sem memória disponivel\n");
        exit(1);
    }

    novo->processo = processo;
    novo->prox = NULL;
    novo->processo->status = 0;

    if (filaEstaVazia(fila))
    {                        
        fila->inicio = novo; 
        fila->fim = novo;
    }
    else
    {
        fila->fim->prox = novo; 
        fila->fim = novo;       
    }
}

PCB *rmvFila(Fila *fila)
{
    PCB *primeiro;

    if (filaEstaVazia(fila))
    {
        printf("Fila vazia!!!\n");
        return NULL;
    }
    else
    {
        primeiro = fila->inicio->processo; 
        fila->inicio = fila->inicio->prox; 
    }

    if (filaEstaVazia(fila))
    {
        fila->fim = NULL;
    }

    return primeiro;
}

int filaEstaVazia(Fila *fila)
{
    return (fila->inicio == NULL) ? 1 : 0; 
}

void printFila(Fila *fila)
{
    No* noFila = fila->inicio;

    while (noFila->processo != fila->fim->processo)
    {
        printProcesso(noFila->processo);
        noFila = noFila->prox;
    }
    printProcesso(fila->fim->processo);
}

void printFilaIODisco(Fila disco)
{
    if (filaEstaVazia(&disco))
    {
        printf("Fila de I/O de DISCO está vazia.\n");
    }
    else
    {
        printf("\n >>> Processos na fila de DISCO (I/O) <<< \n\n");
        printFila(&disco);
    }
}

void printFilaBaixaPrioridade(Fila baixa)
{
    if (filaEstaVazia(&baixa))
    {
        printf("Fila de baixa prioridade está vazia.\n");
    }
    else
    {
        printf("\n >>> Processos com BAIXA prioridade <<< \n\n");
        printFila(&baixa);
    }
}

void printFilaIOFita(Fila fita)
{
    if (filaEstaVazia(&fita))
    {
        printf("Fila de de I/O de FITA está vazia.\n");
    }
    else
    {
        printf("\n >>> Processos na fila de FITA (I/O) <<< \n\n");
        printFila(&fita);
    }
}

void printFilaAltaPrioridade(Fila alta)
{
    if (filaEstaVazia(&alta))
    {
        printf("Fila de alta prioridade está vazia.\n");
    }
    else
    {
        printf("\n >>> Processos com ALTA prioridade <<< \n\n");
        printFila(&alta);
    }
}

void printFilaIOImpressora(Fila impressora)
{
    if (filaEstaVazia(&impressora))
    {
        printf("Fila de I/O da IMPRESSORA está vazia.\n");
    }
    else
    {
        printf("\n >>> Processos na fila de IMPRESSORA (I/O) <<< \n\n");
        printFila(&impressora);
    }
}

void printFilaFinalizados(Fila finalizados)
{
    if (filaEstaVazia(&finalizados))
    {
        printf("Fila de baixa prioridade está vazia.\n");
    }
    else
    {
        printf("\n >>> Processos FINALIZADOS <<< \n\n");
        printFila(&finalizados);
    }
}

void printProcesso(PCB* processo)
{
    printf(" > PID: %d\n", processo->PID);
    printf(" ----> Tempo em execução: %d u.t\n", processo->tempo_interno);
    printf(" ----> Status: %d\n", processo->status);

    if (processo->quantidade_ios < 1)
    {
        printf("\n");
        return;
    }

    printf(" ----> Operações de IO:");

    for (int i = 0; i < processo->quantidade_ios; i++)
    {
        printf("\n --------> Tipo Opr. I/O: %d\n", (processo->operacao_io+i)->tipo);
        printf(" --------> Duração: %d u.t\n", (processo->operacao_io+i)->tipo);
    }

    printf("\n");
}

void printInfoProcesso(PCB *processo, int tempo_sistema)
{
    printf("\n=============================================\n");
    printf("\n >>> Unidade de tempo: %d \n", tempo_sistema);
    printf("\n >>> Processo atual:\n");
    printf("\t> PID: %d\n", processo->PID);
    printf("\t> Status: %d\n", processo->status);
    printf("\t> Tempo restante %d\n", processo->tempo_restante);
    printf("\t> Tempo interno %d\n", processo->tempo_interno);
    printf("\n");
}