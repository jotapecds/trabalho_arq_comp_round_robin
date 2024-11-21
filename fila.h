#include "processo.h"

typedef struct _NO
{
    PCB *processo; 
    struct _NO *prox;
} No;

typedef struct
{
    No *inicio;
    No *fim;
} Fila;

void iniciaFila(Fila *fila);
int filaEstaVazia(Fila *fila);
void addFila(Fila *fila, PCB *processo);
PCB *rmvFila(Fila *fila);
void printFila(Fila *fila);      
void escalona(PCB *processo, Fila *fila);

