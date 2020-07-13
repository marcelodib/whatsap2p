/*
*=====================================================================================
*---------------------------------------CLIENTE---------------------------------------
*=====================================================================================
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
//#include <stdio_ext.h>

/*
 * Definição da struct online_list, para guardar
 * todos os clientes que estão online no momento.
 */
typedef struct
{
	char number[20];
	char ip[80];
	char port[10];
    int s;
    int flag;
} online_list;

typedef struct
{
    char name[100];
	char number[20];
} contact_list;

typedef struct
{
    char name[100];
	contact_list groupMembers[100];
    int numberOfContacts;
} typeGroup;

/*
 * Instancia de um vetor para poder guardar até 100 contatos.
 */
contact_list contactList[100];
/*
 * Instancia de um vetor para poder guardar até 100 grups.
 */
typeGroup group[100];
int numberOfGroups = 0;

/**
 * Variavel para contar o numero de contatos existentes na lista
 */
int numeroDeContatos = 0;

char myNumber[20];

// Variavel de controle para indicar que o cliente esta logado com o servidor.
int flag_logado = 0;

char fileName[50];
char groupFileName[50];

FILE *contatos;                 /* Arquivo contendo os contatos salvos.*/
FILE *grupos;

void *iniciaModoServidor(void *myPort);
void *atendeCliente(void* ns);
void menuLogado(int socket);
void menuConversar(int socket);
void conversar(char ip[], char port[]);
void conversarGrupo(online_list list[]);
void sendImage(char name[], int socket);
void menuListar(int socket);
void createGroup();
int insertContact(typeGroup *group, char contactNumber[]);


int main(int argc, char *argv[])
{
    unsigned short port;        /* Variavel que contem a porta do servidor passada por parametro no inicio.*/
    unsigned int myPort;        /* Variavel qie contem a porta em que a aplicação cliente esta rodando*/
    char myIP[16];              /* Vetor que contem o IP da maquina do cliente.*/
    char myPortS[10];           /* Vetor que contem a porta em que a aplicação cliente esta rodando.*/
    char number[20];            /* Vetor que contem o numero do cliente.*/
    char sendbuf[150];          /* Vetor para montar mensagem de envio do cliente para o servidor.*/
    char recvbuf[100];          /* Vetor para receber a mensagem enviada pelo servidor.*/
    struct hostent *hostnm;     /* Struct para executar o gethostbyname e recuperar o Ip do servidor*/
    struct sockaddr_in server;  /* Struct para instanciar os dados de conexão do servidor*/
    struct sockaddr_in my_addr; /* Struct para recuperar os dados de conexão do cliente*/
    int s;                      /* Socket para aceitar conexões.*/
    int opcao;                  /* Variavel para guardar a opcao do menu escolhida pelo cliente.*/
    int len;                    /* Variavel para guardar o tamanho da struct sockaddr_in*/
    int *param;                 /* Ponteiro para passar o socket criado em cada nova conexão.*/
    pthread_t thread_id;        /* Variavel que contem o id da thread criada.*/
    int index_populacao = 0;    /* Variavel utilizada para a populacao da lista de contatos.*/
	int groupIndex = 0;
    contact_list aux_contact;   /* Variavel utilizada para a populacao da lista de contatos.*/

    /*
     * O primeiro argumento (argv[1]) eh o hostname do servidor.
     * O segundo argumento (argv[2]) eh a porta do servidor.
     */
    if (argc != 3)
    {
        fprintf(stderr, "Use: [%s] hostname porta\n", argv[0]);
        exit(1);
    }

    /*
     * Obtendo o endereco IP do servidor
     */
    hostnm = gethostbyname(argv[1]);
    if (hostnm == (struct hostent *)0)
    {
        fprintf(stderr, "Gethostbyname failed\n");
        exit(2);
    }
    port = (unsigned short)atoi(argv[2]);

    /*
     * Define o endereco IP e a porta do servidor
     */
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    // Print do menu e aguarda a entrada do usuario.
    printf("\n=====================================================================");
    printf("\n1 -> Logar\n2 -> Sair\n");
    printf("Digite a opcao >> ");
    scanf("%i", &opcao);

    /*
     * Loop responsavel por ficar rodando o menu enquanto a aplicação estiver aberta.
     */
    while (opcao != 2)
    {
        /*
        * Cria um socket TCP (stream)
        */
        if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
            perror("\nERRO ao declarar o socket (main)\n");
            exit(3);
        }

        /* Estabelece conexao com o servidor */
        if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
            perror("\nERRO ao conectar com o servidor (main)\n");
            exit(4);
        }

        switch (opcao)
        {
        case 1:
            // Print de login e recebimento do numero do cliente.
            printf("\n=====================================================================");
            printf("\n<1> Login\n");
            printf("Numero >> ");
            fpurge(stdin);
            fgets(number, 20, stdin);

            number[strlen(number) - 1] = '\0';

            strcpy(myNumber, number);

            /* Recupera o IP e porta do cliente.*/
            bzero(&my_addr, sizeof(my_addr)); // Zera a struct my_addr.
            len = sizeof(my_addr); // Atribui o tamanho dela a variavel len.
            getsockname(s, (struct sockaddr *) &my_addr, (unsigned int *)&len); // Recupera os dados de conexão do cliente e atribui ao my_addr.
            inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP)); // Atribui o IP do cliente a string myIP.
            myPort = htons(my_addr.sin_port); // Atribui a porta do cliente a variavel myPort.

            // PRINT para DEBUG
            printf("Local ip address: [%s]\n", myIP);
            printf("Local port : [%u]\n", myPort);

            sprintf(myPortS, "%u", myPort); // Atribui o valor da porta para a string myPortS.

            // Montagem do sendbuf para ser enviado ao servidor.
            sendbuf[0] = 'C';
            sendbuf[1] = '-';
            sendbuf[2] = '\0';
            strcat(sendbuf, number);
            strcat(sendbuf, "-");
            strcat(sendbuf, myIP);
            strcat(sendbuf, "-");
            strcat(sendbuf, myPortS);

            // PRINT para DEBUG
            printf("sendbuf: [%s]\n", sendbuf);

            /* Envia uma mensagem ao servidor atraves do socket conectado */
            if (send(s, sendbuf, strlen(sendbuf) + 1, 0) < 0)
            {
                perror("\nERRO ao enviar a mensagem (main)\n");
                exit(5);
            }

            /* Recebe a mensagem do servidor no buffer recvbuf */
            if (recv(s, recvbuf, sizeof(recvbuf), 0) < 0)
            {
                perror("\nERRO ao receber a mensagem (main)\n");
                exit(6);
            }

            // PRINT para DEBUG.
            printf("%s\n", recvbuf);

            // Atribui o status de logado para o cliente.
            flag_logado = 1;

            // Popula struct de contatos
            sprintf(fileName, "%s-contatos", number);
            if ((contatos = fopen(fileName, "r")) == NULL){
                //printf("Erro ao abrir arquivo de contatos\n");
				contatos = fopen(fileName, "w");
            }
            else{
                while(fread(&(contactList[index_populacao]), sizeof(contact_list), 1, contatos) == 1){
                    // fseek(contatos,sizeof(contact_list)*index_populacao,SEEK_SET);
                    // fread(&(contactList[index_populacao]), sizeof(contact_list), 1, contatos);
                    // printf("%s - %s\n", contactList[index_populacao].name, contactList[index_populacao].number);
                    index_populacao++;
                    numeroDeContatos++;
                }
            }

            fclose(contatos);

            // Popula struct de grupos
            sprintf(groupFileName, "%s-grupos", number);
            if ((grupos = fopen(groupFileName, "r")) == NULL){
                //printf("Erro ao abrir arquivo de grupos\n");
				grupos = fopen(groupFileName, "w");
            }
            else{
                while(fread(&(group[groupIndex]), sizeof(typeGroup), 1, grupos) == 1){
                groupIndex++;
                numberOfGroups++;
            }
            }
            fclose(grupos);

            param = (int *)(long)myPort;

            /* Cria uma nova thread para iniciar o modo servidor do cliente
             * passando como parametro a função iniciaModoServidor que
             * realizará o atendimento, e recebe como parametro a porta que deve aguardar
             * conexões.
             */
            pthread_create(&thread_id, NULL, iniciaModoServidor, (void *)param);

            /*
            * Realiza um detach na thread criada, para permitir
            * que seja desalocada assim que terminar sua execução.
            */
            pthread_detach(thread_id);

            /*
             * Chama a função menuLogado que libera as opções do cliente para se comunicar
             * com outro clientes.
             */
            menuLogado(s);

            numeroDeContatos = 0;

            /*
             * Quando a função menuLogado retornar, significa que o cliente se deslogou.
             */

            // Atribui o valor de deslogado a flag de login do cliente.
            flag_logado = 0;
            close(s);
            break;
        default:
            break;
        }
        printf("\n=====================================================================");
        printf("\n1 -> Login\n2 -> Sair\n");
        printf("Digite a opcao >> ");
        scanf("%i", &opcao);
    }
    // Fecha o socket dessa conexão.
    close(s);
}
/*
 * Função a ser executada em uma nova thread que é responsavel por abrir um socket
 * e ficar aguardando a conexão de outros clientes, assim disparando uma nova thread
 * de atendimento para cada cliente, para que eles possam trocar mensagens.
 */
void *iniciaModoServidor(void *myPort)
{
    struct sockaddr_in client; /* Struct para instanciar os dados de conexão do cliente.*/
    struct sockaddr_in server; /* Struct para instanciar os dados de conexão do Servidor.*/
    int s;                     /* Socket para aceitar conexões.*/
	int ns;                    /* Socket conectado ao cliente.*/
	int client_len;            /* Variavel para recuperar o tamanho da struct sockaddr_in do cliente.*/
    int *param;                /* Ponteiro para passar o socket criado em cada nova conexão.*/
    pthread_t thread_id;       /* Variavel que contem o id da thread criada.*/

    /*
     * Define a qual endereco IP e porta o servidor interno do cliente estará ligado.
     * IP = INADDDR_ANY -> faz com que o servidor se ligue em todos
     * os enderecos IP.
     */
	server.sin_family = AF_INET;
	server.sin_port = (long int)myPort;
	server.sin_addr.s_addr = INADDR_ANY;

    /*
     * Cria um socket TCP (stream) para aguardar conex�es
     */
	if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror("\nERRO ao atribuir o socket (iniciaModoServidor)\n");
		exit(2);
	}

    /*
     * Instancia um Socket para receber novas conexões
     * com as configurações feita acima.
     */
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("\nERRO ao dar bind na porta (iniciaModoServidor)\n");
		exit(3);
	}

    /*
     * Prepara o socket para aguardar por conexões e
     * cria uma fila de conexões pendentes.
     */
	if (listen(s, 1) != 0)
	{
		perror("\nERRO ao dar listen (iniciaModoServidor)\n");
		exit(4);
	}

    do
	{
		client_len = sizeof(client); /* Atribui o tamanho da struct sockaddr_in.*/

        /*
         * Aceita uma nova conexão, e cria um novo socket para atender a mesma.
         */
		if ((ns = accept(s, (struct sockaddr *)&client, (unsigned int *)&client_len)) == -1)
		{
			perror("\n ERRO no Accept (iniciaModoServidor)\n");
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

	} while (flag_logado == 1); // executa esse loop enquanto o cliente estiver com a flag de logado ativada.

    // Fecha o socket dessa conexão.
	close((long int)s);

    // Mata a Thread.
	pthread_exit(NULL);
}

/*
 * Função em uma nova Thread que é responsavel por realizar o atendimento de um novo cliente
 * que deseja se comunicar com os este através de troca de mensagens, recebendo como parametro
 * o socket criado para essa conexão.
 */
void *atendeCliente(void *ns)
{
    int bytes_recv;     /* Variavel para verificar quantos bytes foram recebidos no recv().*/
    int imgLength;
    char recvbuf[1024]; /* Vetor para receber os bytes enviados pelo cliente.*/
	char sendbuf[1000]; /* Vetor para enviar os bytes de resposta para o cliente.*/
    char imgName[100];
    char auxMsg[1000];
    char *number;       /* Ponteiro para recuperar o number do cliente no pacote.*/
	char *msg;          /* Ponteiro para recuperar o mensagem do cliente no pacote.*/
    char type;
    FILE *fileptr;


	long int tid = (long int)pthread_self(); // Recupera o id da Thread.

    /*
     * Aponta todos os ponteiros para NULL, para evitar lixo de memoria.
     */
	msg = NULL;

    /*
     * Loop para recebimento de pacotes do cliente nessa conexão.
     */
	do
	{
        strcpy(recvbuf, "");
		/*
         * Recebe uma mensagem do cliente atraves do novo socket conectado
         * passado como parametro.
         */
		bytes_recv = recv((long int)ns, recvbuf, sizeof(char) * 1000, 0);

        /*
         * Verifica se não ocorreu nenhum erro no pacote recebido.
         */
		if (bytes_recv != -1 && bytes_recv != 0)
		{
            // PRINT para DEBUG
            printf("\n=====================================================================\n");
			//printf("Pacote recebido: [%s]\n", recvbuf);

            type = recvbuf[0];

            if(type == 'T'){
                number = strtok(recvbuf, "-");
                number = strtok(NULL, "-");
                msg = strtok(NULL, "-");
                strcpy(auxMsg, msg);
                // PRINT para DEBUG
                for (int i = 0; i < numeroDeContatos; i++) {
                    if (strcmp(number, contactList[i].number) == 0) {
                        strcpy(number, contactList[i].name);
                        break;
                    }
                }
			    printf("Mensagem recebida do [%s]: %s\n",number, auxMsg);
            }
            else if(type == 'I'){
                msg = strtok(recvbuf, "-");
                msg = strtok(NULL, "-");
                number = strtok(NULL, "-");
                imgLength = atoi(number);
                strcpy(imgName, msg);
                //printf("\n%i\n", imgLength);
                fileptr = fopen(msg, "w");
                while (imgLength > 0)
                {
                    //printf("Recebendo a Imagem\n");
                   /*
                    * Recebe uma mensagem do cliente atraves do novo socket conectado
                    * passado como parametro.
                    */
                    bytes_recv = recv((long int)ns, recvbuf, sizeof(recvbuf), 0);
                    //printf("\n%i\n", bytes_recv);

                    fwrite(recvbuf, 1, sizeof(char)*bytes_recv, fileptr);

                    imgLength = imgLength - bytes_recv;
                    //printf("\n%i\n", imgLength);
                }
                fclose(fileptr);
			    printf("Imagem recebida [%s]\n", imgName);
            }

        }
	} while ((bytes_recv != -1 && bytes_recv != 0) || flag_logado == 1);

    // PRINT para DEBUG.
	printf("Conversa com [%s] ENCERRADA!\n", number);

    // Fecha o socket dessa conexão.
	close((long int)ns);

    // Mata a Thread.
	pthread_exit(NULL);
}

/*
 * Função responsavel por disponibilizar as opções do cliente apos realizar
 * seu login no servidor.
 */
void menuLogado(int socket)
{
    int opcao; /* Variavel que recebe a opcao selecionada pelo cliente.*/
    char sendbuf[100]; /* Vetor para montar mensagem de envio do cliente para o servidor.*/
    char recvbuf[100]; /* Vetor para receber a mensagem enviada pelo servidor.*/
    int i; /* Variavel para controle de interação em loops*/
    char *client; /* Ponteiro para string de cada cliente a ser listado.*/
    char *aux; /* Ponteiro para o resto de clientes a serem listados*/
    char *number; /* Ponteiro para string do numero do cliente*/
    char *ip; /* Ponteiro para string do ip do cliente*/
    char *port; /* Ponteiro para string da porta do cliente*/

    // Print do menu e aguarda o cliente selecionar uma opção.
    printf("\n=====================================================================\n");
    printf("Opções: \n1 -> Enviar mensagem\n2 -> Listar\n3 -> Adicionar contato\n4 -> Criar grupo\n5 -> Logout\n");
    printf("Digite a opcao >> ");
    scanf("%i", &opcao);

    while (opcao != 5)
    {
        switch (opcao)
        {
        case 1:
            // Print da opção selecionada.
            printf("\n=====================================================================\n");
            printf("<1> Enviar mensagem\n");
            menuConversar(socket);
            break;
        case 2:
            // Print da opção selecionada.
            printf("\n=====================================================================\n");
            printf("<2> Listar\n");
            menuListar(socket);
            break;
        case 3:
            // Print da opção selecionada.
            if (numeroDeContatos >= 99) {
                printf("\nNumero MÁXIMO de contatos atingido!\n");
                break;
            }

            printf("\n=====================================================================");
            printf("\n<3> Adicionar contato\n");

			printf("Nome >> ");
            fpurge(stdin);
            fgets(contactList[numeroDeContatos].name, 20, stdin);

            printf("Numero: ");
            fpurge(stdin);
            fgets(contactList[numeroDeContatos].number, 20, stdin);

            contactList[numeroDeContatos].name[strlen(contactList[numeroDeContatos].name) - 1] = '\0';
            contactList[numeroDeContatos].number[strlen(contactList[numeroDeContatos].number) - 1] = '\0';
            numeroDeContatos++;

            // salvar para o arquivo
            if ((contatos = fopen(fileName, "w")) == NULL){
                printf("Erro ao abrir arquivo de contatos\n");
            }

            fwrite(contactList, sizeof(contact_list), numeroDeContatos, contatos);

            fclose(contatos);
            break;
        case 4:
            // Print da opção selecionada.
            printf("\n=====================================================================\n");
            printf("<4> Criar grupo\n");
            createGroup();

			// salvar para o arquivo
			numberOfGroups++;

			if ((grupos = fopen(groupFileName, "w")) == NULL){
				printf("Erro ao abrir arquivo de grupo\n");
			}

			fwrite (group, sizeof(typeGroup), numberOfGroups, grupos);
			fclose(grupos);
            break;
        default:
            break;
        }

        // Print do menu e aguarda o cliente selecionar uma opção.
        printf("\n=====================================================================\n");
        printf("Opções: \n1 -> Enviar mensagem\n2 -> Listar\n3 -> Adicionar contato\n4 -> Criar grupo\n5 -> Logout\n");
        printf("Digite a opcao >> ");
        scanf("%i", &opcao);
    }
    return;
}

int insertContact(typeGroup *group, char contactNumber[]) {
    for (int i = 0; i < numeroDeContatos; i++) {
        if (!strcmp(contactNumber, contactList[i].number)) {
            strcpy(group->groupMembers[group->numberOfContacts].name, contactList[i].name);
            strcpy(group->groupMembers[group->numberOfContacts].number, contactList[i].number);
            return 1;
        }
    }
    return 0;
}

void createGroup() {
    if (numberOfGroups >= 99) {
        printf("Numero MÁXIMO de grupos atingido!\n");
        return;
    }

    char charNumberOfMembers[20];
    char auxContactNumber[20];

    printf("\n=====================================================================");
    printf("\n<3> Criar grupo\n");

    printf("Nome do grupo >> ");
	fpurge(stdin);
    fgets(group[numberOfGroups].name, 20, stdin);
	group[numberOfGroups].name[strlen(group[numberOfGroups].name) - 1] = '\0';

    printf("Numero de membros >> ");
    fpurge(stdin);
    fgets(charNumberOfMembers, 20, stdin);
	charNumberOfMembers[strlen(charNumberOfMembers) - 1] = '\0';

    group[numberOfGroups].numberOfContacts = 0;

    for (int i = 0; i < atoi(charNumberOfMembers); i++){

        printf("Numero do contato [%i] >> ", i + 1);
        fpurge(stdin);
        fgets(auxContactNumber, 20, stdin);
				auxContactNumber[strlen(auxContactNumber) - 1] = '\0';

        if(insertContact(&group[numberOfGroups],auxContactNumber)) {
            group[numberOfGroups].numberOfContacts++;
            printf("Adicionado com sucesso!\n");
        } else {
            printf("Contato não encontrado!\n");
        }
    }
}

void menuListar(int socket){
    int opcao;
    char sendbuf[100]; /* Vetor para montar mensagem de envio do cliente para o servidor.*/
    char recvbuf[100]; /* Vetor para receber a mensagem enviada pelo servidor.*/
    int i; /* Variavel para controle de interação em loops*/
    char *client; /* Ponteiro para string de cada cliente a ser listado.*/
    char *aux; /* Ponteiro para o resto de clientes a serem listados*/
    char *number; /* Ponteiro para string do numero do cliente*/
    char *ip; /* Ponteiro para string do ip do cliente*/
    char *port; /* Ponteiro para string da porta do cliente*/

    printf("\n=====================================================================\n");
    printf("\nOpções: \n1 -> Onlines\n2 -> Contatos\n3 -> Grupos\n4 -> Voltar\n");
    printf("Digite a opcao >> ");
    scanf("%i", &opcao);
    while (opcao != 4)
    {
        switch (opcao)
        {
        case 1:
            // Print da opção selecionada.
            printf("\n=====================================================================\n");
            printf("<1> Lista de Onlines\n");
            // Montagem do pacote a ser enviado para o servidor.
            sendbuf[0] = 'L';

            // PRINT para DEBUG
            //printf("sendbuf: [%s] \n", sendbuf);

            /* Envia uma mensagem ao servidor atraves do socket conectado */
            if (send(socket, sendbuf, strlen(sendbuf) + 1, 0) < 0)
            {
                perror("\nERRO ao enviar a mensagem (menuLogado)\n");
                exit(5);
            }

            /* Recebe a mensagem do servidor no buffer recvbuf */
            if (recv(socket, recvbuf, sizeof(recvbuf), 0) < 0)
            {
                perror("\nERRO ao receber a mensagem (menuLogado)\n");
                exit(6);
            }

            // PRINT para DEBUG.
            //printf("\nPacote recebido [%s]\n", recvbuf);

            // Ponteiro client aponta para o primeiro da lista.
            client = strtok_r(recvbuf, "|", &aux);
            while (1)
                {
                    // PRINT para DEBUG
                    //printf("\nClient a ser cadastrado [%s]\n", client);

                    /* Verifica se o o ponteiro para o cliente não esta vazio*/
                    if(client != NULL){

                        /* Recupera number, Ip, e porta do cliente a ser listado*/
                        number = strtok(client, "-");
                        ip = strtok(NULL, "-");
                        port = strtok(NULL, "-");

                        // Recupera o number, ip e port enviado no pacote.
                        if(number != NULL){
                            printf("ONLINE number: [%s]\n", number);
                        }
                        //printf("NEW client ip: [%s]\n", ip);
                        //printf("NEW client port: [%s]\n", port);

                        // Ponteiro client aponta para o proximo da lista.
                        client = strtok_r(aux, "|", &aux);
                    }
                    else{
                        break;
                    }
                }
            break;
        case 2:
            // Print da opção selecionada.
            printf("\n=====================================================================\n");
            printf("<2> Lista de Contatos\n");
            for(int i = 0; i < numeroDeContatos; i++){
                printf("[%d]: [%s] - [%s]\n", i, contactList[i].name, contactList[i].number);
            }
            break;
        case 3:
            // Print da opção selecionada.
            printf("\n=====================================================================\n");
            printf("<3> Lista de Grupos\n");
			for(int i = 0; i < numberOfGroups; i++){
		        printf("\nGrupo: [%s]| Numero de membros: [%i]\n", group[i].name, group[i].numberOfContacts);
				printf("[%5s] - [%20s] - [%10s]\n", "index", "nome", "numero");
				for (int j = 0; j < group[i].numberOfContacts; j++) {
					printf("[%5i] - [%20s] - [%10s]\n", j, group[i].groupMembers[j].name, group[i].groupMembers[j].number);
				}
			}
            break;
        default:
            break;
        }
        printf("\n=====================================================================\n");
        printf("\nOpções: \n1 -> Onlines\n2 -> Contatos\n3 -> Grupos\n4 -> Voltar\n");
        printf("Digite a opcao >> ");
        scanf("%i", &opcao);
    }
}

void menuConversar(int socket) {
    int opcao;
    int i;
    int j;
    int countOnlines = 0;
    char number[20];
    char sendbuf[100];
    char recvbuf[200];
    char *ip;
    char *port;
    online_list list[100];

    printf("\n=====================================================================\n");
    printf("\nOpções: \n1 -> Para numero\n2 -> Para Contato\n3 -> Para Grupo\n4 -> Voltar\n");
    printf("Digite a opcao >> ");
    scanf("%i", &opcao);
    while (opcao != 4)
    {
        for(i = 0; i < 100; i++){
            list[i].flag = 0;
        }
        countOnlines = 0;
        switch (opcao)
        {
        case 1:
            printf("\n=====================================================================\n");
            printf("<1> Para numero\n");
            printf("Digite o numero >> ");
            fpurge(stdin);
            fgets(number, 20, stdin);
            number[strlen(number) - 1] = '\0';

            sendbuf[0] = 'N';
            sendbuf[1] = '-';
            sendbuf[2] = '\0';
            strcat(sendbuf, number);

            // PRINT para DEBUG
            printf("sendbuf: [%s] \n", sendbuf);

            /* Envia uma mensagem ao servidor atraves do socket conectado */
            if (send(socket, sendbuf, strlen(sendbuf) + 1, 0) < 0)
            {
                perror("\nERRO ao enviar a mensagem (menuConversar)\n");
                exit(5);
            }

            /* Recebe a mensagem do servidor no buffer recvbuf */
            if (recv(socket, recvbuf, sizeof(recvbuf), 0) < 0)
            {
                perror("\nERRO ao receber a mensagem (menuConversar)\n");
                exit(6);
            }
            printf("%s\n", recvbuf);
            if(recvbuf[0] == 'E'){
                printf("\nEste Numero esta Offline!\n");
                break;
            }
            /* Recupera number, Ip, e porta do cliente a ser listado*/
            ip = strtok(recvbuf, "-");
            port = strtok(NULL, "-");

            conversar(ip, port);

            break;
        case 2:
            printf("\n=====================================================================\n");
            printf("<2> Para Contato\n");
            printf("Digite o Nome >> ");
            fpurge(stdin);
            fgets(number, 20, stdin);
            number[strlen(number) - 1] = '\0';

            for(i = 0; i < numeroDeContatos; i++){
                if(strcmp(contactList[i].name, number) == 0){
                    strcpy(number, contactList[i].number);
                    break;
                }
            }

            sendbuf[0] = 'N';
            sendbuf[1] = '-';
            sendbuf[2] = '\0';
            strcat(sendbuf, number);

            // PRINT para DEBUG
            printf("sendbuf: [%s] \n", sendbuf);

            /* Envia uma mensagem ao servidor atraves do socket conectado */
            if (send(socket, sendbuf, strlen(sendbuf) + 1, 0) < 0)
            {
                perror("\nERRO ao enviar a mensagem (menuConversar)\n");
                exit(5);
            }

            /* Recebe a mensagem do servidor no buffer recvbuf */
            if (recv(socket, recvbuf, sizeof(recvbuf), 0) < 0)
            {
                perror("\nERRO ao receber a mensagem (menuConversar)\n");
                exit(6);
            }
            printf("%s\n", recvbuf);
            if(recvbuf[0] == 'E'){
                printf("\nEste Contato esta Offline!\n");
                break;
            }
            /* Recupera number, Ip, e porta do cliente a ser listado*/
            ip = strtok(recvbuf, "-");
            port = strtok(NULL, "-");

            conversar(ip, port);

            break;
        case 3:
            printf("\n=====================================================================\n");
            printf("<3> Para Grupo\n");
            printf("Digite o Grupo >> ");
            fpurge(stdin);
            fgets(number, 20, stdin);
            number[strlen(number) - 1] = '\0';

            for(i = 0; i < numberOfGroups; i++){
                if(strcmp(group[i].name, number) == 0){

                    for(j = 0; j < group[i].numberOfContacts; j++){
                        sendbuf[0] = 'N';
                        sendbuf[1] = '-';
                        sendbuf[2] = '\0';
                        strcat(sendbuf, group[i].groupMembers[j].number);

                        // PRINT para DEBUG
                        printf("sendbuf: [%s] \n", sendbuf);

                        /* Envia uma mensagem ao servidor atraves do socket conectado */
                        if (send(socket, sendbuf, strlen(sendbuf) + 1, 0) < 0)
                        {
                            perror("\nERRO ao enviar a mensagem (menuConversar)\n");
                            exit(5);
                        }

                        /* Recebe a mensagem do servidor no buffer recvbuf */
                        if (recv(socket, recvbuf, sizeof(recvbuf), 0) < 0)
                        {
                            perror("\nERRO ao receber a mensagem (menuConversar)\n");
                            exit(6);
                        }
                        printf("%s\n", recvbuf);
                        if(recvbuf[0] != 'E'){

                            /* Recupera number, Ip, e porta do cliente a ser listado*/
                            ip = strtok(recvbuf, "-");
                            port = strtok(NULL, "-");

                            strcpy(list[j].ip, ip);
                            strcpy(list[j].port, port);
                            list[j].flag = 1;
                            countOnlines ++;
                        }

                    }
                    conversarGrupo(list);

                    break;
                }
            }

            break;
        default:
            break;
        }
        printf("\n=====================================================================\n");
        printf("Opções: \n1 -> Para numero\n2 -> Para Contato\n3 -> Para Grupo\n4 -> Voltar\n");
        printf("Digite a opcao >> ");
        scanf("%i", &opcao);
    }
}

void conversar(char ip[], char port[]) {
    int s;
    int opcao = 1;
    unsigned short clientPort;        /* Variavel que contem a porta do servidor passada por parametro no inicio.*/
    char msg[100];
    char sendbuf[150];          /* Vetor para montar mensagem de envio do cliente para o servidor.*/
    struct sockaddr_in client;  /* Struct para instanciar os dados de conexão do servidor*/
    struct hostent *hostnm;     /* Struct para executar o gethostbyname e recuperar o Ip do servidor*/


    hostnm = gethostbyname(ip);
    if (hostnm == (struct hostent *)0)
    {
        fprintf(stderr, "Gethostbyname failed\n");
        exit(2);
    }
    clientPort= (unsigned short)atoi(port);

    /*
     * Define a qual endereco IP e porta o servidor interno do cliente estará ligado.
     * IP = INADDDR_ANY -> faz com que o servidor se ligue em todos
     * os enderecos IP.
     */
	client.sin_family = AF_INET;
	client.sin_port = (long int)clientPort;
	client.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    /*
     * Cria um socket TCP (stream) para aguardar conex�es
     */
	if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror("\nERRO ao atribuir o socket (conversar)\n");
		exit(2);
	}

    /* Estabelece conexao com o servidor */
    if (connect(s, (struct sockaddr *)&client, sizeof(client)) < 0)
    {
        perror("\nERRO ao conectar com o servidor (conversar)\n");
        exit(4);
    }
    printf("\n=====================================================================\n");
    printf("Opções: \n1 -> Mensagem de Texto\n2 -> Imagem\n3 -> Voltar\n");
    printf("Digite a opcao >> ");
    scanf("%i", &opcao);
    while (opcao != 3)
    {
        switch (opcao)
        {
        case 1:
            strcpy(msg, "");
            // Print de recebimento da mensagem.
            printf("\n=====================================================================\n");
            printf("Mensagem >> ");
            fpurge(stdin);
            fgets(msg, 99, stdin);

            // Montagem do sendbuf para ser enviado ao servidor.
            sendbuf[0] = 'T';
            sendbuf[1] = '-';
            sendbuf[2] = '\0';
            strcat(sendbuf, myNumber);
            strcat(sendbuf, "-");
            strcat(sendbuf, msg);

            /* Envia uma mensagem ao servidor atraves do socket conectado */
            if (send(s, sendbuf, strlen(sendbuf) + 1, 0) < 0)
            {
                perror("\nERRO ao enviar a mensagem (conversar)\n");
                exit(5);
            }
            break;
        case 2:
            strcpy(msg, "");
            // Print de recebimento da imagem.
            printf("\n=====================================================================\n");
            printf("Imagem >> ");
            fpurge(stdin);
            fgets(msg, 99, stdin);
            msg[strlen(msg) - 1] = '\0';

            sendImage(msg, s);
        default:
            break;
        }
        printf("\n=====================================================================\n");
        printf("Opções: \n1 -> Mensagem de Texto\n2 -> Imagem\n3 -> Voltar\n");
        printf("Digite a opcao >> ");
        scanf("%i", &opcao);
    }
    // Fecha o socket dessa conexão.
    close(s);
    return;
}

void conversarGrupo(online_list list[]){
    int s;
    int i;
    int opcao = 1;
    unsigned short clientPort;        /* Variavel que contem a porta do servidor passada por parametro no inicio.*/
    char msg[100];
    char sendbuf[150];          /* Vetor para montar mensagem de envio do cliente para o servidor.*/
    struct sockaddr_in client;  /* Struct para instanciar os dados de conexão do servidor*/
    struct hostent *hostnm;     /* Struct para executar o gethostbyname e recuperar o Ip do servidor*/

    for(i = 0; i < sizeof(*list); i++){
        if(list[i].flag == 1){
            hostnm = gethostbyname(list[i].ip);
            if (hostnm == (struct hostent *)0)
            {
                fprintf(stderr, "Gethostbyname failed\n");
                exit(2);
            }
            clientPort= (unsigned short)atoi(list[i].port);

            /*
            * Define a qual endereco IP e porta o servidor interno do cliente estará ligado.
            * IP = INADDDR_ANY -> faz com que o servidor se ligue em todos
            * os enderecos IP.
            */
            client.sin_family = AF_INET;
            client.sin_port = (long int)clientPort;
            client.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

            /*
            * Cria um socket TCP (stream) para aguardar conex�es
            */
            if ((list[i].s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
            {
                perror("\nERRO ao atribuir o socket (conversar)\n");
                exit(2);
            }

            /* Estabelece conexao com o servidor */
            if (connect(list[i].s, (struct sockaddr *)&client, sizeof(client)) < 0)
            {
                perror("\nERRO ao conectar com o servidor (conversar)\n");
                exit(4);
            }
        }
    }

    printf("\n=====================================================================\n");
    printf("Opções: \n1 -> Mensagem de Texto\n2 -> Imagem\n3 -> Voltar\n");
    printf("Digite a opcao >> ");
    scanf("%i", &opcao);
    while (opcao != 3)
    {
        switch (opcao)
        {
        case 1:
            strcpy(msg, "");
            // Print de recebimento da mensagem.
            printf("\n=====================================================================\n");
            printf("Mensagem >> ");
            fpurge(stdin);
            fgets(msg, 99, stdin);

            // Montagem do sendbuf para ser enviado ao servidor.
            sendbuf[0] = 'T';
            sendbuf[1] = '-';
            sendbuf[2] = '\0';
            strcat(sendbuf, myNumber);
            strcat(sendbuf, "-");
            strcat(sendbuf, msg);

            for(i = 0; i < sizeof(*list); i++){
                if(list[i].flag == 1){
                    /* Envia uma mensagem ao servidor atraves do socket conectado */
                    if (send(list[i].s, sendbuf, strlen(sendbuf) + 1, 0) < 0)
                    {
                        perror("\nERRO ao enviar a mensagem (conversar)\n");
                        exit(5);
                    }
                }
            }
            break;
        case 2:
            strcpy(msg, "");
            // Print de recebimento da imagem.
            printf("\n=====================================================================\n");
            printf("Imagem >> ");
            fpurge(stdin);
            fgets(msg, 99, stdin);
            msg[strlen(msg) - 1] = '\0';

            for(i = 0; i < sizeof(*list); i++){
                if(list[i].flag == 1){
                    sendImage(msg, list[i].s);
                }
            }
            break;
        default:
            break;
        }
        printf("\n=====================================================================\n");
        printf("Opções: \n1 -> Mensagem de Texto\n2 -> Imagem\n3 -> Voltar\n");
        printf("Digite a opcao >> ");
        scanf("%i", &opcao);
    }
    for(i = 0; i < sizeof(*list); i++){
        if(list[i].flag == 1){
            // Fecha o socket dessa conexão.
            close(list[i].s);
        }
    }
    return;
}

void sendImage(char name[], int socket){
    struct stat fileStat;
    char sendbuf[1024];
    char sizeFile[20];
    FILE *fileptr;
    int i;
    long int size;

    // if(stat(name,&fileStat) < 0){
    //     printf("ERRO ao ler informacoes do arquivo!\n");
    //     return;
    // }
    fileptr = fopen(name, "r");

    fseek(fileptr, 0, SEEK_END);
    size = ftell(fileptr);
    fseek(fileptr, 0, SEEK_SET);

    printf("Information for [%s]\n",name);
    printf("---------------------------\n");
    printf("File Size: \t\t[%li] bytes\n", size);

    sprintf(sizeFile, "%li", size);

   //fileptr = fopen(name, "r");

    sendbuf[0] = 'I';
    sendbuf[1] = '-';
    sendbuf[2] = '\0';
    strcat(sendbuf, "");
    strcat(sendbuf, name);
    strcat(sendbuf, "-");
    strcat(sendbuf, sizeFile);

    /* Envia uma mensagem ao servidor atraves do socket conectado */
    if (send(socket, sendbuf, strlen(sendbuf) + 1, 0) < 0)
    {
        perror("\nERRO ao enviar a mensagem (conversar)\n");
        exit(5);
    }

    // Read in the entire file
    //fread(sendbuf, sizeof(char), 1023, fileptr);
    //i = 0;
    while(!feof(fileptr)){
        i = fread(sendbuf, 1, sizeof(sendbuf), fileptr);
        //i++;
        //printf("Mandando a imagem %i\n", i);
        /* Envia uma mensagem ao servidor atraves do socket conectado */
        while (send(socket, sendbuf, sizeof(sendbuf), 0) < 0){}

        // if (send(socket, sendbuf, strlen(sendbuf), 0) < 0)
        // {
        //     perror("\nERRO ao enviar a mensagem (conversar)\n");
        //     exit(5);
        // }
        // fseek(fileptr, sizeof(char)*1024, SEEK_CUR);
        bzero(sendbuf, sizeof(sendbuf));
    }
        //printf("%hhx\n", sendbuf[i]);
}
