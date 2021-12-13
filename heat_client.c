#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define DIRECTION_MAX 45
#define BUFFER_MAX 128
#define VALUE_MAX 256

double target_degree;    

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int create_sock(const int addr,const int port)
{
    int sock;
    struct sockaddr_in serv_addr;
    socklen_t clnt_addr_size;
    char msg[BUFFER_MAX];
    int str_len;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(addr);
    serv_addr.sin_port = htons(atoi(port));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    return sock;
}

void *send(void* serv_sock)
{
    int sock=*((int *)serv_sock);
    double degree;
    char msg[BUFFER_MAX];
    
    while (1)
    {
        sprintf(msg, "%d", degree);
        write(sock, msg, sizeof(msg));
        usleep(500 * 100);
    }

    close(sock);

    return (0);
}


void *receive(void* serv_sock)
{
    int sock=*((int *)serv_sock);
    int str_len;
    char msg[BUFFER_MAX];
    

    while (1)
    {
        str_len=read(sock, msg, sizeof(msg));
        target_degree=atof(msg);
        if (str_len == -1)
            error_handling("read() error");
        usleep(500 * 100);
    }

    close(sock);

    return (0);
}

int main(int argc, char *argv[])
{
    int port=8888;
    int sock;
    char msg[BUFFER_MAX];
    
    pthread_t p_thread[2];
    int thr_id;
    int status;

    if (argc != 2)
    {
        printf("Usage : %s <IP>\n", argv[0]);
        exit(1);
    }

    sock = create_sock(argv[1],port);



    thr_id=pthread_create(&p_thread[0],NULL, send,(void*)&sock);
    if(thr_id<0)
    {
        perror("thread create error : ");
        exit(0);
    }
    thr_id=pthread_create(&p_thread[1], NULL, receive, (void*)&sock);
    if(thr_id<0)
    {
        perror("thread create error : ");
        exit(0);
    }

    pthread_join(p_thread[0], (void **)&status);
    pthread_join(p_thread[1], (void **)&status);

    return 0;
}

