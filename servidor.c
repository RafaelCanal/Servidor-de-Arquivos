/*
  PROGRAMA SERVIDOR PARA CONEXÃO SOCKETS EM SERVIDOR DE ARQUIVOS
  Programadores: Claudio André Korb
                 Rafael Canal
  Instituição: Universidade Federal de Santa Catarina
  Disciplina: Sistemas Operacionais

  Big thanks to Brian "Beej Jorgensen" Hall for his amazing guide to Network Programming
  Available at: http://beej.us/guide/bgnet/html/multi/index.html
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8979
#define BACKLOG 5

pthread_mutex_t mutex;                                                          //Mutex utilizado para controle de condições de corrida

int strcmpst1nl (const char * s1, const char * s2);
void * at_connection(void *socket_fd);
int file_create(int new_socket, char *current_dir_name);
int file_delete(int new_socket, char *current_dir_name);
int file_read(int new_socket, char *current_dir_name);
int file_write(int new_socket, char *current_dir_name);
int directory_make(int new_socket, char *current_dir_name);
int directory_remove(int new_socket, char *current_dir_name);
int directory_print(int new_socket, DIR *current_dir);

int main(int argc, char const * argv[]){
  int socket_fd, socket_client, tam, *new_socket;
  struct sockaddr_in server, client;

  //CRIANDO O SOCKET PARA O SERVIDOR
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(socket_fd == -1){
    printf("Erro: Create socket");
  }
  printf("Socket criado\n");
  //============================================================================

  //ADICIONANDO O ENDEREÇO AO SOCKET
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(PORT);
  //============================================================================

  //LIGANDO O SOCKET À PORTA 8080
  if(bind(socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0){
    printf("Erro: Bind");
    return 1;
  }
  printf("Bind ok\n");
  //============================================================================
  pthread_mutex_init(&mutex, NULL);
  //ESPERANDO POR CONEXÕES
  while(1){
    listen(socket_fd, BACKLOG);
    printf("Esperando conexoes...\n");
    //============================================================================
    tam = sizeof(struct sockaddr_in);
    while(socket_client = accept(socket_fd, (struct sockaddr*)&client, (socklen_t*)&tam)){ //ACEITANDO A CONEXÃO
      struct in_addr ipAddr = client.sin_addr;
      char ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &ipAddr, ip, INET_ADDRSTRLEN);
      printf("Cliente conectado: %s\n", ip);
      pthread_t w_thread;
      new_socket = (int*)malloc(sizeof(int));
      *new_socket = socket_client;

      if(pthread_create(&w_thread, NULL, at_connection, (void*)new_socket) < 0){
        printf("Erro: create thread");
        return 1;
      }
      printf("Thread criada\n");
    }
    if(socket_client < 0){
      printf("Erro: accept");
      return 1;
    }
    pthread_mutex_destroy(&mutex);
  }
  return 0;
}

int strcmpst1nl (const char * s1, const char * s2){
  char s1c;                                     //FUNÇÃO USADA PARA COMPARAR DUAS STRINGS, SENDO QUE
  do                                            // A SEGUNDA TERMINA EM CARACTERE NULO
    {
      s1c = *s1;
      if (s1c == '\n')
          s1c = 0;
      if (s1c != *s2)
          return 1;
      s1++;
      s2++;
    } while (s1c);
  return 0;
}

void * at_connection(void* socket_fd){
  DIR *current_dir = NULL;
  char current_dir_name[1024];
  int new_socket = *(int*)socket_fd;
  int valread;
  char opt[1024] = "";
  char greeting[1024];

  getcwd(current_dir_name, sizeof(current_dir_name));                           //RECEBE O NOME DO DIRETORIO INICIAL
  strcat(current_dir_name, "/Servidor de Arquivos");
  current_dir = opendir(current_dir_name);                                      //ABRE O DIRETORIO
  strcpy(greeting, "\nBem vindo ao servidor de arquivos!\nComandos: \ncreate -- Criar Arquivo \ndelete -- Deletar Arquivo \nwrite -- Escrever no Arquivo\nread -- Mostrar Conteudo do Arquivo\nmkdir -- Criar Diretorio \nrmdir -- Remover Diretorio \ndir -- Mostrar conteudo do diretorio\ncd -- Trocar de Diretorio\nhelp -- Comandos \nclose -- Encerrar conexao\n");
  send(new_socket, greeting, strlen(greeting), 0);                              //ENVIA MENSAGEM DE CUMPRIMENTO

  valread = read(new_socket, opt, 1024);
  while(1){
    //CRIANDO UM ARQUIVO-------------------------------------
    if(strcmpst1nl(opt, "create") == 0){
      file_create(new_socket, current_dir_name);
    }
    //DELETANDO UM ARQUIVO-----------------------------------------------------
    else if (strcmpst1nl(opt, "delete") == 0){
      pthread_mutex_lock(&mutex);
      file_delete(new_socket, current_dir_name);
      pthread_mutex_unlock(&mutex);
    }
    //MOSTRANDO O CONTEUDO DO ARQUIVO-------------------------------------------
    else if(strcmpst1nl(opt, "read") == 0){
      pthread_mutex_lock(&mutex);
      file_read(new_socket, current_dir_name);
      pthread_mutex_unlock(&mutex);
    }
    //ESCREVENDO EM UM ARQUIVO--------------------------------------------------
    else if(strcmpst1nl(opt, "write") == 0){
      pthread_mutex_lock(&mutex);
      file_write(new_socket, current_dir_name);
      pthread_mutex_unlock(&mutex);
    }

    //CRIANDO UM DIRETORIO -------------------------------------------------------
    else if(strcmpst1nl(opt, "mkdir") == 0){
      directory_make(new_socket, current_dir_name);
    }

    // REMOVENDO UM DIRETORIO -------------------------------------------------
    else if(strcmpst1nl(opt, "rmdir") == 0){
      pthread_mutex_lock(&mutex);
      directory_remove(new_socket, current_dir_name);
      pthread_mutex_unlock(&mutex);
    }
    // MOSTRANDO CONTEUDO DO DIRETORIO -----------------------------------------
    else if(strcmpst1nl(opt, "dir") == 0){
      directory_print(new_socket, current_dir);
    }
    // TROCANDO DE DIRETORIO ---------------------------------------------------
    else if(strcmpst1nl(opt, "cd") == 0){
      char mensagem[1024] = "";
      char buffer[1024] = "";
      char caminho[1024] = "";
      strcpy(mensagem, "Nome do diretorio: ");
      send(new_socket, mensagem, strlen(mensagem), 0);
      if(valread = read(new_socket, buffer, 80) == -1){
        strcpy(mensagem, "Nome invalido");
        send(new_socket, mensagem, strlen(mensagem), 0);
      }
      else{
        buffer[strlen(buffer) - 1] = '\0';
        snprintf(caminho, sizeof(caminho), "%s/%s", current_dir_name, buffer);
        if(chdir(caminho) == -1){
          strcpy(mensagem, "Erro ao trocar de diretorio");
          send(new_socket, mensagem, strlen(mensagem), 0);
        }
        else{
          getcwd(current_dir_name, sizeof(current_dir_name));
          current_dir = opendir(current_dir_name);
          strcpy(mensagem, "OK");
          send(new_socket, mensagem, strlen(mensagem), 0);
        }

      }
    }
    //MOSTRANDO COMANDOS -------------------------------------------------------
    else if(strcmpst1nl(opt, "help") == 0){
      char mensagem[1024] = "";
      char buffer[1024] = "";
      strcpy(mensagem, "Comandos: \ncreate -- Criar Arquivo \ndelete -- Deletar Arquivo \nwrite -- Escrever no Arquivo\nread -- Mostrar Conteudo do Arquivo\nmkdir -- Criar Diretorio \nrmdir -- Remover Diretorio \ndir -- Mostrar conteudo do diretorio\ncd -- Trocar de Diretorio\nhelp -- Comandos \nclose -- Encerrar conexao\n");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
    //ENCERRANDO CONEXAO
    else if(strcmpst1nl(opt, "close") == 0){
      close(new_socket);
      pthread_kill(pthread_self(), 0);
    }
    else{
      char mensagem[1024] = "";
      char buffer[1024] = "";
      strcpy(mensagem, "COMANDO INVALIDO!\n");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
    valread = read(new_socket, opt, 1024);
  }
}

int file_create(int new_socket, char *current_dir_name){
  int valread;
  char nome[1024]     = "";
  char mensagem[1024] = "";
  char buffer[1024]   = "";
  char caminho[1024]  = "";

  strcpy(mensagem, "Digite o nome do arquivo: ");
  send(new_socket, mensagem, strlen(mensagem), 0);
  if(valread = read(new_socket, buffer, 32) == -1){
    strcpy(mensagem, "Nome do arquivo invalido!");
    send(new_socket, mensagem, strlen(mensagem), 0);
  }
  else{
    buffer[strlen(buffer)-1] = '\0';                                            //REMOVENDO ULTIMO CARACTERE
    strcpy(caminho, current_dir_name);
    caminho[strlen(caminho)] = '/';
    strcat(caminho, buffer);
    snprintf(nome, sizeof(nome), "%s.txt", caminho);                            //ADICIONANDO A EXTENSAO .TXT AO NOME
    FILE* new_file = fopen(nome, "w");                                          //CRIANDO O ARQUIVO
    if(new_file == NULL){                                                       //CHECANDO ERRO
      strcpy(mensagem, "Falha ao criar arquivo!");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
    else{
      strcpy(mensagem, "Arquivo criado com sucesso");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
    fclose(new_file);
  }
}

int file_delete(int new_socket, char* current_dir_name){
  char mensagem[1024] = "";
  char buffer[1024]   = "";
  char caminho[1024]  = "";
  char nome[1024]     = "";
  int valread;
  strcpy(mensagem, "Digite o nome do arquivo a ser deletado: ");
  send(new_socket, mensagem, strlen(mensagem), 0);
  if(valread = read(new_socket, buffer, 32) == -1){
    strcpy(mensagem, "Nome do arquivo invalido!");
    send(new_socket, mensagem, strlen(mensagem), 0);
  }
  else{
    buffer[strlen(buffer)-1] = '\0';
    snprintf(caminho, sizeof(caminho), "%s/%s.txt", current_dir_name, buffer);                             //INSERINDO A EXTENSAO .TXT AO NOME
    if(remove(caminho) == -1){                                                     //REMOVENDO O ARQUIVO
      strcpy(mensagem, "Falha ao deletar arquivo");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
    else{
      strcpy(mensagem, "Arquivo excluido com sucesso");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
  }
}

int directory_make(int new_socket, char *current_dir_name){
  char mensagem[1024] = "";
  char buffer[1024] = "";
  char caminho[1024] = "";
  int valread;
  strcpy(mensagem, "Digite o nome do diretorio a ser criado: ");
  send(new_socket, mensagem, strlen(mensagem), 0);
  if(valread = read(new_socket, buffer, 32) == -1){
    strcpy(mensagem, "Nome do diretorio invalido!");
    send(new_socket, mensagem, strlen(mensagem), 0);
  }
  else{
    buffer[strlen(buffer)-1] = '\0';

    snprintf(caminho, sizeof(caminho), "%s/%s", current_dir_name, buffer);
    if(mkdir(caminho, ALLPERMS) == -1){
      strcpy(mensagem, "Erro ao criar diretorio!");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
    else{
      chmod(buffer, ALLPERMS);                                                    //ALTERANDO MODO DE PERMISSÃO
      strcpy(mensagem, "Diretorio criado com sucesso!");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
  }
}

int file_read(int new_socket, char *current_dir_name){
  char mensagem[1024] = "";
  char buffer[1024]   = "";
  char caminho[1024]  = "";
  int valread;
  strcpy(mensagem, "Digite o nome do arquivo que deseja imprimir: ");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof mensagem);
  if(valread = read(new_socket, buffer, 32) == -1){
    strcpy(mensagem, "Nome do arquivo invalido!");
    send(new_socket, mensagem, strlen(mensagem), 0);
  }
  else{
    buffer[strlen(buffer)-1] = '\0';
    snprintf(caminho, sizeof(caminho), "%s/%s.txt", current_dir_name, buffer);
    FILE* read_file = fopen(caminho, "r");
    if(read_file == NULL){
      strcpy(mensagem, "Falha ao abrir arquivo!");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
    else{
      memset(mensagem, 0, sizeof mensagem);
      fread(mensagem, sizeof(char), 1024, read_file);
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
    fclose(read_file);
  }
}

int file_write(int new_socket, char *current_dir_name){
  char mensagem[1024] = "";
  char buffer[1024]   = "";
  char caminho[1024]  = "";
  int valread;
  strcpy(mensagem, "Digite o nome do arquivo em que deseja escrever: ");
  send(new_socket, mensagem, strlen(mensagem), 0);
  memset(mensagem, 0, sizeof mensagem);
  if(valread = read(new_socket, buffer, 32) == -1){
    strcpy(mensagem, "Nome do arquivo invalido!");
    send(new_socket, mensagem, strlen(mensagem), 0);
  }
  else{
    buffer[strlen(buffer)-1] = '\0';
    snprintf(caminho, sizeof(caminho), "%s/%s.txt", current_dir_name, buffer);
    FILE* write_file = fopen(caminho, "w");
    if(write_file == NULL){
      strcpy(mensagem, "Erro ao encontrar o arquivo");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
    else{
      strcpy(mensagem, "O que deseja escrever? ");
      send(new_socket, mensagem, strlen(mensagem), 0);
      memset(buffer, '\0', strlen(buffer));
      memset(mensagem, 0, sizeof mensagem);
      if(valread = read(new_socket, buffer, 1024) == -1){
        strcpy(mensagem, "Falha ao escrever no arquivo!");
        send(new_socket, mensagem, strlen(mensagem), 0);
      }
      else{
        strcpy(mensagem, "OK");
        send(new_socket, mensagem, strlen(mensagem), 0);
        memset(mensagem, 0, sizeof mensagem);
        fputs(buffer, write_file);
        fclose(write_file);
      }
    }
  }
}

int directory_remove(int new_socket, char *current_dir_name){
  char mensagem[1024] = "";
  char buffer[1024] = "";
  char caminho[1024] = "";
  int valread;
  strcpy(mensagem, "Digite o nome do diretorio a ser excluido: ");
  send(new_socket, mensagem, strlen(mensagem), 0);
  if(valread = read(new_socket, buffer, 32) == -1){
    strcpy(mensagem, "Nome do diretorio invalido!");
    send(new_socket, mensagem, strlen(mensagem), 0);
  }
  else{
    buffer[strlen(buffer)-1] = '\0';
    if(strcmp(buffer, "Servidor de Arquivos") == 0){
      strcpy(mensagem, "Falha ao remover diretorio");
      send(new_socket, mensagem, strlen(mensagem), 0);
      return 0;
    }
    snprintf(caminho, sizeof(caminho), "%s/%s", current_dir_name, buffer);
    if(rmdir(caminho) == -1){
      strcpy(mensagem, "Falha ao remover diretorio!");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
    else{
      strcpy(mensagem, "Diretorio removido com sucesso!");
      send(new_socket, mensagem, strlen(mensagem), 0);
    }
  }

}

int directory_print(int new_socket, DIR *current_dir){
  char mensagem[1024] = "";
  char buffer[1024] = "";
  struct dirent *dir = NULL;
  dir = readdir(current_dir);
  memset(mensagem, 0, sizeof(mensagem));
  while(dir = readdir(current_dir)){
    strcat(mensagem, dir->d_name);
    strcat(mensagem, "\n");
  }
  rewinddir(current_dir);
  send(new_socket, mensagem, strlen(mensagem), 0);
}