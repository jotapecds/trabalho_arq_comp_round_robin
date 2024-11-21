#include <stdio.h>
#include <stdlib.h>
#include "fila.c"

#define MAX_PROCESSOS 5
#define QUANTUM 2

#define IO_DISCO_TEMPO 3
#define IO_FITA_TEMPO 5
#define IO_IMPRESSORA_TEMPO 7

/* Métodos do escalonador */
void roundRobin();
void IOHandler(PCB *processo, OperacaoIO *OperacaoIO); 
void fimDeIO();      
                                 
Fila alta;
Fila baixa;
Fila disco;
Fila fita;
Fila impressora;
Fila finalizados; 

int tempo_sistema; 

int main(int argc, char *argv[])
{
    FILE *file;

    if (argc != 2)
    {
        printf("Uso: %s <nome_do_arquivo>\n", argv[0]);
        return 1;
    }

    file = fopen(argv[1], "r");
    if (file == NULL)
    {
        printf("Erro ao abrir o arquivo");
        return 1;
    }

    iniciaFila(&alta);
    iniciaFila(&baixa);
    iniciaFila(&disco);
    iniciaFila(&fita);
    iniciaFila(&impressora);

    leProcessos(file);

    printf("\n");
    printf(" --- PROCESSOS INSERIDOS NO ESCALONADOR --- \n");
    printf("\n");

    printFila(&alta);

    tempo_sistema = 0;

    printf("\n");
    printf(" --- INICIO DO SIMULADOR --- \n");
    printf("\n");

    roundRobin();

    return 0;
}

/* Faz a leitura dos processos no arquivo */
void leProcessos(FILE *file)
{
    PCB *processo;
    OperacaoIO *operacao;
    char info_processo[100];

    while (fgets(info_processo, sizeof(info_processo), file) != NULL)
    {
        /* Se o processo não tem IO*/

        processo = (PCB *)malloc(sizeof(PCB));

        if (processo == NULL)
        {
            printf("Erro de alocação de memória\n");
            exit(-1);
        }

        processo->PID = atoi(&info_processo[0]);
        processo->instante_de_ativacao = atoi(&info_processo[2]);
        processo->tempo_cpu = atoi(&info_processo[4]);
        processo->quantidade_ios = atoi(&info_processo[6]);

        if (processo->quantidade_ios != 0)
        {
            operacao = (OperacaoIO *)malloc(sizeof(OperacaoIO) * processo->quantidade_ios);
            processo->operacao_io = operacao;
            for (int i = 0, j = 8; i < processo->quantidade_ios; i++, j += 4)
            {
                if ((operacao + i) == NULL)
                {
                    printf("Erro de alocação de memória\n");
                    exit(-1);
                }

                (processo->operacao_io + i)->tipo = atoi(&info_processo[j]);
                (processo->operacao_io + i)->instante_inicio = atoi(&info_processo[j + 2]);

                switch (atoi(&info_processo[j]))
                {
                case 0:
                    (processo->operacao_io + i)->tipo = DISCO;
                    break;

                case 1:
                    (processo->operacao_io + i)->tipo = FITA;
                    break;

                case 2:
                    (processo->operacao_io + i)->tipo = IMPRESSORA;
                    break;

                default:
                    (processo->operacao_io + i)->tipo = IMPRESSORA;
                }
            }
        }

        addFila(&alta, processo);
    }
}

void IOHandler(PCB *processo, OperacaoIO *operacaoIO)
{
    switch (operacaoIO->tipo)
    {
    case DISCO:
        addFila(&disco, processo);
        processo->tempo_restante = IO_DISCO_TEMPO;
        break;

    case FITA:
        addFila(&fita, processo);
        processo->tempo_restante = IO_FITA_TEMPO;
        break;

    case IMPRESSORA:
        addFila(&impressora, processo);
        processo->tempo_restante = IO_IMPRESSORA_TEMPO;
        break;
    }
}

void fimDeIO()
{
    PCB *processo;

    if (disco.inicio != NULL)
    {
        if (disco.inicio->processo->tempo_restante == 0)
        {
            processo = rmvFila(&disco); 
            addFila(&baixa, processo); 
        }
        else
        {
            disco.inicio->processo->tempo_restante--;
        }
    }

    if (fita.inicio != NULL)
    {
        if (fita.inicio->processo->tempo_restante == 0)
        {
            processo = rmvFila(&fita);
            addFila(&alta, processo);
        }
        else
        {
            fita.inicio->processo->tempo_restante--;
        }
    }

    if (impressora.inicio != NULL)
    {
        if (impressora.inicio->processo->tempo_restante == 0)
        {
            processo = rmvFila(&impressora);
            addFila(&alta, processo);
        }
        else
        {
            impressora.inicio->processo->tempo_restante--;
        }
    }
}

void escalona(PCB *processo, Fila *fila)
{
    processo->tempo_restante--;
    processo->tempo_interno++;

    for (int i = 0; i < processo->quantidade_ios; i++)
    {
        if ((processo->operacao_io + i) != NULL && (processo->operacao_io + i)->instante_inicio == processo->tempo_interno)
        {
            processo = rmvFila(fila);
            IOHandler(processo, (processo->operacao_io+i));
        }
    }

    if (processo->tempo_interno == processo->tempo_cpu)
    {
        processo = rmvFila(fila);
        addFila(&finalizados, processo);
    }

    else if (processo->tempo_restante == 0)
    {
        processo = rmvFila(fila);
        addFila(&baixa, processo);
    }
}

void roundRobin()
{
    PCB *processo_atual;

    while (!(filaEstaVazia(&alta)) || !(filaEstaVazia(&baixa)) || !(filaEstaVazia(&fita)) || !(filaEstaVazia(&disco)) || !(filaEstaVazia(&impressora)))
    {
        if (!filaEstaVazia(&alta))
        {
            processo_atual = alta.inicio->processo;
            if (processo_atual->status == 0)
            {
                processo_atual->status = 1;
                processo_atual->tempo_restante = QUANTUM;
            }
            escalona(processo_atual, &alta);
        }

        else if (!filaEstaVazia(&baixa))
        {
            processo_atual = baixa.inicio->processo;
            if (processo_atual->status == 0)
            {
                processo_atual->status = 1;
                processo_atual->tempo_restante = QUANTUM;
            }
            escalona(processo_atual, &baixa);
        }

        tempo_sistema++;

        printInfoProcesso(processo_atual, tempo_sistema);
        printFilaAltaPrioridade(alta);
        printFilaBaixaPrioridade(baixa);
        printFilaIODisco(disco);
        printFilaIOFita(fita);
        printFilaIOImpressora(impressora);
        printFilaFinalizados(finalizados);

        fimDeIO();
    }
    printf("\n --- FIM DO SIMULADOR: Todos os processos foram finalizados --- \n\n");
}
