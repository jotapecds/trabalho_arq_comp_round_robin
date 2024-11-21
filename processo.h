typedef enum
{
    DISCO,
    FITA,
    IMPRESSORA
} IO;

typedef struct
{
    IO tipo;
    int instante_inicio;
} OperacaoIO;

typedef struct
{
    int PID;                  
    int instante_de_ativacao; 
    int tempo_cpu;            
    int tempo_restante;       
    int tempo_interno;        
    int status;               
    OperacaoIO *operacao_io;  
    int quantidade_ios;
} PCB;

void leProcessos(FILE *file);        
void printProcesso(PCB* processo);
