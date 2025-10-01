/* servidorUDP.c
   Chat UDP não-bloqueante usando threads (servidor).
   Compile: gcc servidorUDP.c -o servidorUDP -pthread
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <dirent.h>

#define ECHOMAX 255
#define PORT 9000
#define NUM_PEERS 2     //MODIFICAR: numero de peers na rede - total de peers -1, não precisa especificar máquina que está rodando o código
#define P2P_DIR "./p2p"


int sock;                       // descritor do socket
int running = 1;                // flag para controle de encerramento

struct sockaddr_in peers[NUM_PEERS];
char *peer_ip[NUM_PEERS] = {"192.168.0.1", "192.168.0.2"}; //MODIFICAR: colocar IP dos PEERS

// Endereço do cliente que enviou mensagem.
// Inicialmente não conhecido (flag client_known == 0).
// struct sockaddr_in client;
// int client_known = 0;

// Thread que fica recebendo mensagens de qualquer client
void *recv_thread(void *arg) {
    char buf[ECHOMAX + 1];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    ssize_t n;

    while (running) {
        // Bloqueia esperando datagrama
        n = recvfrom(sock, buf, ECHOMAX, 0, (struct sockaddr *)&from, &fromlen);
        if (n < 0) {
            if (!running) break;
            perror("recvfrom");
            break;
        }
        buf[n] = '\0';

        // Mostra endereço do remetente e mensagem
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(from.sin_addr), ipstr, sizeof(ipstr));
        printf("[De %s:%d] %s\n", ipstr, ntohs(from.sin_port), buf);

        //client = from;

        // Se o cliente mandou "exit", poderíamos decidir encerrar.
        if (strncmp(buf, "ADD|", 4) == 0) {
            char *filename = buf + 4;
            printf("ADD recebido. Arquivo: %s\n", filename);

            // manda GET para quem enviou
            char mensagem[ECHOMAX + 1];
            snprintf(mensagem, sizeof(mensagem), "GET|%s", filename);

            if (sendto(sock, mensagem, strlen(mensagem), 0,
                    (struct sockaddr *)&from, sizeof(from)) < 0) {
                perror("sendto GET");
            }
        }

        if (strncmp(buf, "GET|", 4) == 0) {
            char *filename = buf + 4;
            printf("GET recebido. Enviando arquivo: %s\n", filename);

            char path[512];
            snprintf(path, sizeof(path), "%s/%s", P2P_DIR, filename);

            FILE *f = fopen(path, "r");
            if (!f) {
                perror("fopen GET");
            } else {
                char conteudo[ECHOMAX - 20];
                fread(conteudo, 1, sizeof(conteudo) - 1, f);
                conteudo[sizeof(conteudo) - 1] = '\0';
                fclose(f);

                char mensagem[ECHOMAX + 1];
                snprintf(mensagem, sizeof(mensagem), "FILE|%s|%s", filename, conteudo);

                if (sendto(sock, mensagem, strlen(mensagem), 0,
                        (struct sockaddr *)&from, sizeof(from)) < 0) {
                    perror("sendto FILE");
                }
            }
        }
        
        if (strncmp(buf, "FILE|", 5) == 0) {
            char *p = strchr(buf + 5, '|');
            if (!p) return NULL;
            *p = '\0';
            char *filename = buf + 5;
            char *data = p + 1;

            char path[512];
            snprintf(path, sizeof(path), "%s/%s", P2P_DIR, filename);

            FILE *f = fopen(path, "w");
            if (!f) {
                perror("fopen FILE");
            } else {
                fprintf(f, "%s", data);
                fclose(f);
                printf("Arquivo %s salvo com sucesso!\n", filename);
            }
        }
        

        if (strncmp(buf, "DEL|", 4) == 0) {
            char *filename = buf + 4;
            printf("DEL recebido. Arquivo: %s\n", filename);
            
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", P2P_DIR, filename);
            if (unlink(path) == 0) {
                printf("Arquivo %s deletado localmente.\n", path);
            } else {
                perror("unlink");
            }
        }

        if (strncmp(buf, "LIST_REQ", 8) == 0) {
            printf("LIST_REQ recebido. Enviando lista de arquivos...\n");

            DIR *d = opendir(P2P_DIR);
            if (!d) { perror("opendir"); continue; }

            char mensagem[ECHOMAX + 1];
            strcpy(mensagem, "LIST_RESP|");

            struct dirent *dir;
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_type == DT_REG) { // arquivos regulares
                    // concatena o nome do arquivo na mensagem
                    if (strlen(mensagem) + strlen(dir->d_name) + 2 < sizeof(mensagem)) {
                        strcat(mensagem, dir->d_name);
                        strcat(mensagem, ","); // separador
                    }
                }
            }
            closedir(d);

            // remove última vírgula se existir
            size_t len = strlen(mensagem);
            if (mensagem[len - 1] == ',') mensagem[len - 1] = '\0';

            // envia resposta apenas para quem pediu
            if (sendto(sock, mensagem, strlen(mensagem), 0,
                    (struct sockaddr *)&from, sizeof(from)) < 0) {
                perror("sendto LIST_RESP");
            }
        }

        if (strncmp(buf, "LIST_RESP|", 10) == 0) {
            printf("Lista recebida: %s\n", buf + 10);
        }
    }
    
    return NULL;
}

int main(void) {
    pthread_t rt;
    struct sockaddr_in me;
    char linha[ECHOMAX + 1];

    // Cria socket UDP
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    // Preenche estrutura local
    memset(&me, 0, sizeof(me));
    me.sin_family = AF_INET;
    me.sin_addr.s_addr = htonl(INADDR_ANY);
    me.sin_port = htons(PORT);

    // Preenche a estrutura de peers da rede
    for (int i = 0; i < NUM_PEERS; i++) {
        memset(&peers[i], 0, sizeof(peers[i]));
        peers[i].sin_family = AF_INET;
        peers[i].sin_port = htons(PORT);         // mesma porta que o outro peer está escutando
        if (inet_aton(peer_ip[i], &peers[i].sin_addr) == 0) { //check de erro com IPs dos peers
            fprintf(stderr, "IP inválido: %s\n", peer_ip[i]);
            exit(1);
        }
    }

    // Faz bind na porta
    if (bind(sock, (struct sockaddr *)&me, sizeof(me)) < 0) {
        perror("bind");
        close(sock);
        exit(1);
    }

    printf("Servidor iniciado. Escutando na porta %d\n", PORT);

    // Cria thread de recebimento
    if (pthread_create(&rt, NULL, recv_thread, NULL) != 0) {
        perror("pthread_create");
        close(sock);
        exit(1);
    }

    // Loop de envio (main thread): envia mensagens para o cliente conhecido
    while (running) {
        if (fgets(linha, sizeof(linha), stdin) == NULL) {
            running = 0;
            break;
        }

        // Remove newline
        size_t len = strlen(linha);
        if (len > 0 && linha[len - 1] == '\n') linha[len - 1] = '\0';

        // // Se usuário digitou exit, sinaliza e envia "exit" para o cliente
        // if (strcmp(linha, "exit") == 0) {
        //     if (sendto(sock, linha, strlen(linha), 0, (struct sockaddr *)&peers[i], sizeof(peers[i])) < 0) {
        //         perror("sendto");
        //     }
        //     running = 0;
        //     break;
        // }
        
        if (strncmp(linha, "add ", 4) == 0){
            char *filename = linha + 4;
            char mensagem[ECHOMAX + 1];
            snprintf(mensagem, sizeof(mensagem), "ADD|%s", filename);
            // Envia para peers conhecidos
            for(int i = 0; i<NUM_PEERS; i++){
                if (sendto(sock, mensagem, strlen(mensagem), 0, (struct sockaddr *)&peers[i], sizeof(peers[i])) < 0) {
                    perror("sendto");
                }
            }
        }

        if (strncmp(linha, "del ", 4) == 0){
            char *filename = linha + 4;
            char mensagem[ECHOMAX + 1];
            snprintf(mensagem, sizeof(mensagem), "DEL|%s", filename);
            for(int i = 0; i<NUM_PEERS; i++){
                if (sendto(sock, mensagem, strlen(mensagem), 0, (struct sockaddr *)&peers[i], sizeof(peers[i])) < 0) {
                    perror("sendto");
                }
            }
        }

        if (strncmp(linha, "list", 4) == 0){
            char mensagem[ECHOMAX + 1];
            snprintf(mensagem, sizeof(mensagem), "LIST_REQ");
            for(int i = 0; i<NUM_PEERS; i++){
                if (sendto(sock, mensagem, strlen(mensagem), 0, (struct sockaddr *)&peers[i], sizeof(peers[i])) < 0) {
                    perror("sendto");
                }
            }            
        }
    }

    // Fecha socket para acordar recvfrom na thread de recepção
    close(sock);

    // Aguarda thread terminar
    pthread_join(rt, NULL);

    printf("Servidor finalizado.\n");
    return 0;
}
