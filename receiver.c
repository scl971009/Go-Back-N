#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

typedef struct {
	int length;
	int seqNumber;
	int ackNumber;
	int fin;
	int syn;
	int ack;
} header;

typedef struct{
	header head;
	char data[1000];
} segment;

void setIP(char *dst, char *src) {
    if(strcmp(src, "0.0.0.0") == 0 || strcmp(src, "local") == 0
            || strcmp(src, "localhost")) {
        sscanf("127.0.0.1", "%s", dst);
    } else {
        sscanf(src, "%s", dst);
    }
}

int main(int argc, char* argv[]){
    char path[100];
    strcpy(path, "result.txt");
	char ip[2][50]; //0 = receiver, 1 = agent
	int port[2];
	int recvsocket;
	struct sockaddr_in agent, receiver, tmp_addr;
	socklen_t agent_size, tmp_size;
	if(argc != 4){
        fprintf(stderr,"用法: %s <agent IP> <recv port> <agent port>\n", argv[0]);
        fprintf(stderr, "例如: ./receiver local 8889 8888\n");
        exit(1);
    } else {
        setIP(ip[0], "local");
        setIP(ip[1], argv[1]);

        sscanf(argv[2], "%d", &port[0]);
        sscanf(argv[3], "%d", &port[1]);
    }
    recvsocket = socket(PF_INET, SOCK_DGRAM, 0);

    receiver.sin_family = AF_INET;
    receiver.sin_port = htons(port[0]);
    receiver.sin_addr.s_addr = inet_addr(ip[0]);
    memset(receiver.sin_zero, '\0', sizeof(receiver.sin_zero));
    bind(recvsocket,(struct sockaddr *)&receiver,sizeof(receiver));

    agent.sin_family = AF_INET;
    agent.sin_port = htons(port[1]);
    agent.sin_addr.s_addr = inet_addr(ip[1]);
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));

    agent_size = sizeof(agent);
    tmp_size = sizeof(tmp_addr);

    int segment_size;
    segment message, to_send;
    char buffer[32][1000];
    int curack = 0;
    int i = 0;
    int flush = 0;
    int fin = 0;
    FILE *fp;
    fp = fopen(path, "w");

    to_send.head.ack = 1;
    to_send.head.syn = 0;
    to_send.head.fin = 0;
    to_send.head.ackNumber = 0;
    to_send.head.seqNumber = 0;
    
    while(1){
        flush = 0;
        segment_size = recvfrom(recvsocket, &message, sizeof(message), 0, (struct sockaddr *)&tmp_addr, &tmp_size);
        if(segment_size > 0){
            if(message.head.fin == 1){
                printf("recv fin\n");
                //printf("length = %d\n", message.head.length);
                for(int j = 0; j < i - 1; j++){
                    fwrite(buffer[j], sizeof(char), 1000, fp);
                }
                fwrite(buffer[i - 1], sizeof(char), message.head.length, fp);
                to_send.head.ackNumber = curack;
                to_send.head.fin = 1;
                sendto(recvsocket, &to_send, segment_size, 0, (struct sockaddr *)&agent, agent_size);
                printf("send finack\n");
                printf("flush\n");
                return 0;
            }
            else{
                printf("recv data #%d\n", message.head.seqNumber);
            }
            if(message.head.seqNumber == curack + 1){
                if(i == 32){
                    for(int j = 0; j < 32; j++){
                        fwrite(buffer[j], sizeof(char), 1000, fp);
                    }
                    i = 0;
                    printf("drop data #%d\n", curack + 1);
                    flush = 1;
                    //printf("flush\n");
                }
                else{
                    memcpy(buffer[i], message.data, 1000);
                    //buffer[i] = message.data;
                    i++;
                    curack++;
                }
            }
            to_send.head.ackNumber = curack;
            to_send.head.ack = 1;
            sendto(recvsocket, &to_send, segment_size, 0, (struct sockaddr *)&agent, agent_size);
            printf("send ack #%d\n", to_send.head.ackNumber);
            if(flush == 1){
                printf("flush\n");
            }
        }
    }
	return 0;
}