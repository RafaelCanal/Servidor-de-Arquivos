/*
  PROGRAMA CLIENTE PARA CONEXÃO SOCKETS EM SERVIDOR DE ARQUIVOS
  Programadores: Claudio André Korb
                 Rafael Canal
  Instituição: Universidade Federal de Santa Catarina
  Disciplina: Sistemas Operacionais

  Big thanks to Brian "Beej Jorgensen" Hall for his amazing guide to Network Programming
  Available at: http://beej.us/guide/bgnet/html/multi/index.html
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 8979
#define LEN  1024

void f_send_message(int sock);
int strcmpst1nl (const char * s1, const char * s2);

int main(int argc, char const *argv[])
{
  struct sockaddr_in adress;                                                    // ENDEREÇO DO CLIENTE -- NAO UTILIZADO?
  int sock = 0, valread;                                                        // SOCKET PARA O CLIENTE
  struct sockaddr_in serv_addr;                                                 // ENDEREÇO DO SERVIDOR
  char *msg;
  msg = (char *)malloc(1024*sizeof(char));
  char buffer[1024] = "";
  char path[1024] = "";

  struct addrinfo hints, *res;                                                  //ENDERECOS UTILIZADOS PARA PESQUISA DNS
  memset(&hints, 0, sizeof(hints));                                             //PREENCHENDO A MEMORIA COM 0s
  hints.ai_family = AF_UNSPEC;                                                  //IPv4 ou IPv6
  hints.ai_socktype = SOCK_STREAM;                                              //PROTOCOLO TCP

  if(getaddrinfo("127.0.0.1", "http", &hints, &res) != 0){                      //RECUPERA INFORMACOES SOBRE O SERVIDOR DE DESTINO
    printf("Nao foi possivel identificar o servidor\n");
    return -1;
  }
  if((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)   // AF_INET = IPV4, SOCK_STREAM = TCP, 0 = PROTOLOCO IP
  {                                                                             // CRIA A CONEXAO SOCKET COM O SERVIDOR
    printf("\n Socket creation error \n");
    return -1;
  }

  memset(&serv_addr, '0', sizeof(serv_addr));                                   // PREENCHE SERV_ADDR COM '0'

  serv_addr.sin_family = AF_INET;                                               // FAMILIA IPV4
  serv_addr.sin_port = htons(PORT);                                             // PORT DO SERVIDOR     htons = host to network (se necessário inverte a ordem dos bits)

  if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)                 // CONVERTENDO O IP DE 'CHAR' PARA IPV4 -- 127.0.0.1 => Loopback adress = esta máquina
  {                                                                             // pton = presentation to network
    printf("\nInvalid adress / Adress not supported \n");                       // Retorna -1 em erro e 0 se o ip não está no formato certo
    return -1;
  }

  if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) > 0)       //CONECTANDO COM O SERVIDOR
  {
    printf("\nConnection failed \n");
    return -1;
  }

  while(1)
  {
    valread = recv(sock, buffer, 1024, 0);
    printf("%s\n", buffer);                                                     //IMPRIME MENSAGEM DE CHEGADA
    f_send_message(sock);
    memset(buffer, '\0', strlen(buffer));
    if(strcmpst1nl(buffer, "exit") == 0)
    {
      exit(0);
      close(sock);
    }
  }

  return 0;
}

void f_send_message(int sock){
  char* mensagem = (char*)malloc(LEN*sizeof(char));
  fgets(mensagem, LEN, stdin);
  if(strcmpst1nl(mensagem, "cls") == 0)
  {
    system("clear");
    free(mensagem);
    char* mensagem = (char*)malloc(LEN*sizeof(char));
    fgets(mensagem, LEN, stdin);
  }
  send(sock, mensagem, strlen(mensagem), 0);
  memset(mensagem, '\0', sizeof(mensagem));
  free(mensagem);
  return;
}

int strcmpst1nl (const char * s1, const char * s2)                              //FUNCAO UTILIZADA PARA COMPARAR STRINGS IGNORANDO O CARACTERE \n
{                                                                               //Fonte: Usuario StackOverflow abligh
  char s1c;                                                                     //https://stackoverflow.com/a/21475992
  do
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
