#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <wiringPi.h>
#include <stdint.h>


#define DIRECTION_MAX 45
#define BUFFER_MAX 128
#define VALUE_MAX 256

#define MAXTIMINGS 83
#define DHTPIN 7

double target_degree;    

int dht11_dat[5] = {
    0,
};

double read_dht11_dat()
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    uint8_t flag = HIGH;
    uint8_t state = 0;
    double degree;

    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, LOW);
    delay(18);
    digitalWrite(DHTPIN, HIGH);
    delayMicroseconds(30);
    pinMode(DHTPIN, INPUT);

    for (i = 0; i < MAXTIMINGS; i++)
    {
        counter = 0;
        while (digitalRead(DHTPIN) == laststate)
        {
            counter++;
            delayMicroseconds(1);
            if (counter == 200)
                break;
        }
        laststate = digitalRead(DHTPIN);

        if (counter == 200)
            break; // if while breaked by timer, break for

        if ((i >= 4) && (i % 2 == 0))
        {
            dht11_dat[j / 8] <<= 1;

            if (counter > 50)
                dht11_dat[j / 8] |= 1;

            j++;
        }
    }
    if ((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xff)))
    {
        // printf("humidity = %d.%d %% Temperature = %d.%d *C \n", dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
        double n=(double)dht11_dat[3];
        while (n >= 1.0)
            n = n * 0.1;
        degree=dht11_dat[2]+n;
        return degree;
    }
    else
        printf("Data get failed\n");
}

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
        degree=read_dht11_dat();
        sprintf(msg, "%.1f", degree);
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

