/*
*=====================================================================================
*---------------------------------------SERVIDOR--------------------------------------
*=====================================================================================
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
// #include <stdio_ext.h>

/*
 * Inicialização do semáforo para controlar o acesso
 * a struct list
 */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Definição da struct online_list, para guardar
 * todos os clientes que estão online no momento.
 */
typedef struct
{
	char number[20];
	char ip[80];
	char port[10];
    int flag;
} online_list;

/*
 * Instancia de um vetor para poder guardar até 10 usuarios onlines 
 */
online_list list[10];

void *atendeCliente(void* param);

int main (int argc, char *argv[]) {
    unsigned short port;       /* Variavel para recuperar a porta passada como parametro.*/
    struct sockaddr_in client; /* Struct para instanciar os dados de conexão do cliente.*/
    struct sockaddr_in server; /* Struct para instanciar os dados de conexão do Servidor.*/ 
    int s;                     /* Socket para aceitar conexões.*/
	int ns;                    /* Socket conectado ao cliente.*/
	int client_len;            /* Variavel para recuperar o tamanho da struct sockaddr_in do cliente.*/
    int *param;                /* Ponteiro para passar o socket criado em cada nova conexão.*/
    pthread_t thread_id;       /* Variavel que contem o id da thread criada.*/

    /*
     * O primeiro argumento (argv[1]) é a porta
     * onde o servidor aguardará por conexões.
     */
	if (argc != 2)
	{
		fprintf(stderr, "Use: %s porta\n", argv[0]);
		exit(1);
	}

    //printf("port: %u\n", port);
    port = (unsigned short)atoi(argv[1]);

    /*
     * Cria um socket TCP (stream) para aguardar conex�es
     */
	if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror("\nERRO ao atribuir o socket\n");
		exit(2);
	}

    /*
     * Define a qual endereco IP e porta o servidor estará ligado.
     * IP = INADDDR_ANY -> faz com que o servidor se ligue em todos
     * os enderecos IP.
     */
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = INADDR_ANY;

    /*
     * Instancia um Socket para receber novas conexões
     * com as configurações feita acima.
     */
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("\nERRO ao dar bind na porta\n");
		exit(3);
	}

    /*
     * Prepara o socket para aguardar por conexões e
     * cria uma fila de conexões pendentes.
     */
	if (listen(s, 1) != 0)
	{
		perror("\nERRO ao dar listen\n");
		exit(4);
	}

    // PRINT para DEBUG.
    printf("Servidor ON!\n");

    do
	{
		client_len = sizeof(client); /* Atribui o tamanho da struct sockaddr_in.*/

        /*
         * Aceita uma nova conexão, e cria um novo socket para atender a mesma.
         */
		if ((ns = accept(s, (struct sockaddr *)&client, (unsigned int *)&client_len)) == -1)
		{
			perror("Accept()\n");
			exit(5);
		}

		param = (int *)(long)ns; /* Passa o novo socket criado para a variavel param.*/

		/* Cria uma nova thread para atender esse novo cliente
		 * passando como parametro a função atendeCliente que
         * realizará o atendimento, e o param que contem o socket
         * aberto para essa nova conexão.
         */
		pthread_create(&thread_id, NULL, atendeCliente, (void *)param);

        /*
         * Realiza um detach na thread criada, para permitir
         * que seja desalocada assim que terminar sua execução.
         */
		pthread_detach(thread_id);

	} while (1);
}

void *atendeCliente(void *ns)
{
    int i;              /* Variavel de controle para loop.*/
    int bytes_recv;     /* Variavel para verificar quantos bytes foram recebidos no recv().*/
    char recvbuf[1000]; /* Vetor para receber os bytes enviados pelo cliente.*/
	char sendbuf[1000]; /* Vetor para enviar os bytes de resposta para o cliente.*/
	char *number;       /* Ponteiro para recuperar o number do cliente no pacote.*/
	char *ip;           /* Ponteiro para recuperar o ip do cliente no pacote.*/
	char *port;         /* Ponteiro para recuperar o port do cliente no pacote.*/

    char auxNumber[100];
    char auxIp[100];
    char auxPort[100];

	long int tid = (long int)pthread_self(); // Recupera o id da Thread.

    /*
     * Aponta todos os ponteiros para NULL, para evitar lixo de memoria.
     */
	number = NULL;
	ip = NULL;
    port = NULL;

    /*
     * Loop para recebimento de pacotes do cliente nessa conexão.
     */
	do
	{
		/* 
         * Recebe uma mensagem do cliente atraves do novo socket conectado 
         * passado como parametro.
         */
		bytes_recv = recv((long int)ns, recvbuf, sizeof(online_list), 0);
        
        /*
         * Verifica se não ocorreu nenhum erro no pacote recebido.
         */
		if (bytes_recv != -1 && bytes_recv != 0)
		{
            // Limpa o BUFFER de resposta.
            strcpy(sendbuf, "");

            // PRINT para DEBUG
            printf("\n=====================================================================\n");
			printf("Mensagem recebida do cliente: [%s]\n", recvbuf);

            /*
             * Verifica se o cliente deseja se cadastrar na lista de onlines.
             */
			if(recvbuf[0] == 'C')
            {  
                // Pula o primero BYTE.
                number = strtok(recvbuf, "-");

                // Recupera o number, ip e port enviado no pacote.
                number = strtok(NULL, "-");
                ip = strtok(NULL, "-");
                port = strtok(NULL, "-");

                strcpy(auxNumber, number);
                strcpy(auxIp, ip);
                strcpy(auxPort, port);

                // PRINT para DEBUG
                printf("NEW client number: [%s]\n", number);
                printf("NEW client ip: [%s]\n", ip);
                printf("NEW client port: [%s]\n", port);

                // Trava o MUTEX.
                pthread_mutex_lock (&mutex);

                /*
                 * Loop responsavel por percorrer toda lista de onlines
                 * procurando uma posição vazia para cadastrar um novo
                 * cliente como online.
                 */
                for (i = 0; i < 10; i++)
                {
                    // Verifica se a posição está vazia.
                    if (list[i].flag == 0)
                    {
                        // Muda a posição para ocupada, e atribui os valores do novo cliente.
                        list[i].flag = 1;
                        strcpy(list[i].number, number);
                        strcpy(list[i].ip, ip);
                        strcpy(list[i].port, port);

                        // Monta a resposta para o cliente.
						strcpy(sendbuf, "\nVoce esta ONLINE!");

                        /* Envia uma mensagem ao cliente atraves do socket conectado */
                        if (send((long int)ns, sendbuf, strlen(sendbuf) + 1, 0) < 0)
                        {
                            perror("Send()\n");
                            exit(7);
                        }
                        break;
                    }
                }
                // Verifica se a lista está lotada
                if (i == 10)
                {
                    // Monta a resposta para o cliente.
                    strcpy(sendbuf, "\nERRO - Capacidade maxima atingida!\n");

                    /* Envia uma mensagem ao cliente atraves do socket conectado */
                    if (send((long int)ns, sendbuf, strlen(sendbuf) + 1, 0) < 0)
                    {
                        perror("Send()\n");
                        exit(7);
                    }
                    break;
                }

                // Destrava o MUTEX.
                pthread_mutex_unlock (&mutex); //UNLOCK
            }
            /*
             * Verifica se o cliente deseja receber todos que estão online.
             */
			else if (recvbuf[0] == 'L')
			{
                /*
                 * Loop responsavel por percorrer todas lista de onlines
                 * e enviar ao cliente.
                 */
				for (i = 0; i < 10; i++)
                {
                    // Verifica se a posição esta ocupada
                    if (list[i].flag == 1)
                    { 
                        // Monta a resposta para o cliente
                        strcat(sendbuf, list[i].number);
                        strcat(sendbuf, "-");
                        strcat(sendbuf, list[i].ip);
                        strcat(sendbuf, "-");
                        strcat(sendbuf, list[i].port);
                        strcat(sendbuf, "|");

                        // PRINT para DEBUG
                        printf("%s\n", sendbuf);
                    }
                }
                strcat(sendbuf, "\0");
                /* Envia uma mensagem ao cliente atraves do socket conectado */
                if (send((long int)ns, sendbuf, strlen(sendbuf) + 1, 0) < 0)
                {
                    perror("Send()\n");
                    exit(7);
                }
			}
            /*
             * Verifica se o cliente deseja receber o ip e porta
             * de um determinado numero, caso ele esteja online.
             */
            else if(recvbuf[0] == 'N')
            {
                // Pula o primero BYTE.
                number = strtok(recvbuf, "-");

                // Recupera o number enviado no pacote.
                number = strtok(NULL, "-");

                /*
                 * Loop responsavel por percorrer toda lista de onlines
                 * para verificar se o numero enviado esta online.
                 */
				for (i = 0; i < 10; i++)
                {
                    // Verifica se a posição esta ocupada
                    if (list[i].flag == 1)
                    { 
                        // Verifica se o é o numero enviado pelo cliente.
                        if(strcmp(list[i].number, number) == 0)
                        {
                            // Monta a resposta para o cliente
                            strcat(sendbuf, list[i].ip);
                            strcat(sendbuf, "-");
                            strcat(sendbuf, list[i].port);
                            break;
                        }
                    }
                }
                strcat(sendbuf, "\0");

                if(i == 10){
                    strcpy(sendbuf, "E");
                }

                // PRINT para DEBUG
                printf("%s\n", sendbuf);

                /* Envia uma mensagem ao cliente atraves do socket conectado */
                if (send((long int)ns, sendbuf, strlen(sendbuf) + 1, 0) < 0)
                {
                    perror("Send()\n");
                    exit(7);
                }
            }
        }
			
	} while (bytes_recv != -1 && bytes_recv != 0);

    /*
     * Caso a conexão seja encerrada, ou ocorra algum erro no envio do pacote
     * Percorre toda a lista muda o cliente dessas conexão para offline
     * tornando sua posição na lista como liberada para ser reescrita por uma nova conexão.
     */
    for (i = 0; i < 10; i++)
    {
        // Verifica se é o cliente que acabou de ter a conexão encerrada.
        if (list[i].flag == 1 && strcmp(list[i].number, auxNumber) == 0 && strcmp(list[i].ip, auxIp) == 0 && strcmp(list[i].port, auxPort) == 0)
        {
            // Torna sua posição na lista como desocupada.
            list[i].flag = 0;
            break;
        }
    }
    // PRINT para DEBUG.
    printf("\n=====================================================================\n");
	printf("Cliente [%s] OFFLINE!\n", auxNumber);

    // Fecha o socket dessa conexão.
	close((long int)ns);

    // Mata a Thread.
	pthread_exit(NULL);
}