#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

int threshold = 16;
int window = 1;


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
    strcpy(path, "test_short.txt");
	char ip[2][50]; //0 = sender, 1 = agent
	int port[2];
	int sendersocket;
	struct sockaddr_in sender, agent, tmp_addr;
	socklen_t agent_size, tmp_size;
	if(argc != 4){
        fprintf(stderr,"用法: %s <agent IP> <sender port> <agent port>\n", argv[0]);
        fprintf(stderr, "例如: ./sender local 8887 8888\n");
        exit(1);
    } else {
        setIP(ip[0], "local");
        setIP(ip[1], argv[1]);

        sscanf(argv[2], "%d", &port[0]);
        sscanf(argv[3], "%d", &port[1]);
    }
    sendersocket = socket(PF_INET, SOCK_DGRAM, 0);

    sender.sin_family = AF_INET;
    sender.sin_port = htons(port[0]);
    sender.sin_addr.s_addr = inet_addr(ip[0]);
    memset(sender.sin_zero, '\0', sizeof(sender.sin_zero)); 
    bind(sendersocket,(struct sockaddr *)&sender,sizeof(sender));

    agent.sin_family = AF_INET;
    agent.sin_port = htons(port[1]);
    agent.sin_addr.s_addr = inet_addr(ip[1]);
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));

    agent_size = sizeof(agent);
    tmp_size = sizeof(tmp_addr);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(sendersocket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }

    /*segment to_send;
    int segment_size = sizeof(segment);
    to_send.head.seqNumber = 1;
    to_send.head.ack = 0;
    sendto(sendersocket, &to_send, segment_size, 0, (struct sockaddr *)&agent, agent_size);
    printf("sent\n");*/

    FILE *fp;
    fp = fopen(path, "r");
    segment to_send[1000], recv;
    for(int i = 0; i < 1000; i++){
        to_send[i].head.ack = 0;
        to_send[i].head.syn = 0;
        to_send[i].head.fin = 0;
        to_send[i].head.ackNumber = 0;
        to_send[i].head.seqNumber = 0;
    }
    char buf[1000][1000];
    int temp_int;
    char temp[1000];
    int read[1000] = {0};
    int flag = 0;
    int curack = 0;
    int i;
    int flag_for_read;
    int length = 0;
    int segment_size = sizeof(segment);
    while(1){
        /*if(flag != 0){
            printf("no more read\n");
        }*/
        segment_size = sizeof(segment);
        for(i = 0; i < window; i++){
            //printf("i = %d, window = %d\n", i, window);
            to_send[i].head.seqNumber = curack + i + 1;
            if(read[to_send[i].head.seqNumber] == 0){
                if(to_send[i].head.seqNumber <= flag || flag == 0){
                    to_send[i].head.length = fread(temp, sizeof(char), 1000, fp);
                    length = to_send[i].head.length;
                    //printf("length = %d\n", to_send[i].head.length);
                    memcpy(to_send[i].data, temp, 1000);
                    memcpy(buf[to_send[i].head.seqNumber], temp, 1000);
                    segment_size = sizeof(segment);
                    sendto(sendersocket, &to_send[i], segment_size, 0, (struct sockaddr *)&agent, agent_size);                        printf("send data #%d, winsize = %d\n", to_send[i].head.seqNumber, window);
                }
            }
            else{
                if(to_send[i].head.seqNumber <= flag || flag == 0){
                    if(flag == 1 && to_send[i].head.seqNumber == flag){
                        memcpy(to_send[i].data, buf[to_send[i].head.seqNumber], 1000);
                        segment_size = sizeof(segment);
                        sendto(sendersocket, &to_send[i], segment_size, 0, (struct sockaddr *)&agent, agent_size);
                        printf("resnd data #%d, winsize = %d\n", to_send[i].head.seqNumber, window);
                    }
                    else{
                        memcpy(to_send[i].data, buf[to_send[i].head.seqNumber], 1000);
                        segment_size = sizeof(segment);
                        sendto(sendersocket, &to_send[i], segment_size, 0, (struct sockaddr *)&agent, agent_size);
                        printf("resnd data #%d, winsize = %d\n", to_send[i].head.seqNumber, window);
                    }
                }
            }
            /*if(to_send[i].head.length != 1000){
                memset(&to_send[i].data[to_send[i].head.length], '\0', 1000-to_send[i].head.length);
            }*/
            //printf("curack = %d, i = %d\n", curack, i);
            if(feof(fp) && flag == 0){
                printf("end\n");
                flag = to_send[i].head.seqNumber;
                //printf("flag = %d\n", flag);
            }
            //printf("send ack = %d\n", to_send[i].head.ack);
        }
        temp_int = i;
        while(temp_int--){  
            if(curack == flag && flag != 0){
                break;
            }
            //printf("temp_int = %d, i = %d\n", temp_int, i);
            segment_size = recvfrom(sendersocket, &recv, sizeof(recv), 0, (struct sockaddr *)&tmp_addr, &tmp_size);
            if(segment_size == -1)
                break;
            //printf("time out? %d\n", segment_size);
            if(segment_size > 0){
                printf("recv ack #%d\n", recv.head.ackNumber);
                if(recv.head.ackNumber != to_send[i - temp_int - 1].head.seqNumber){
                    temp_int++;
                }
                if(recv.head.ackNumber > curack){
                    curack = recv.head.ackNumber;
                }
            }
        }
        if(curack == to_send[i - 1].head.seqNumber || (curack == flag && flag != 0)){
            //printf("yes\n");
            if(window < threshold){
                window *= 2;
            }
            else{
                window += 1;
            }
            //printf("flag = %d, curack = %d\n", flag, curack);
            if(flag == curack){
                to_send[0].head.fin = 1;
                to_send[0].head.length = length;
                segment_size = sizeof(segment);
                sendto(sendersocket, &to_send[0], segment_size, 0, (struct sockaddr *)&agent, agent_size);
                printf("send fin\n");
                while(!(recv.head.fin == 1 & recv.head.ack == 1)){
                    segment_size = recvfrom(sendersocket, &recv, sizeof(recv), 0, (struct sockaddr *)&tmp_addr, &tmp_size);
                }
                if(recv.head.fin == 1 & recv.head.ack == 1){
                    printf("recv finack\n");
                }
                return 0;
            }
        }
        else{
            if(window > 3){
                threshold = window / 2;
            }
            else{
                threshold = 1;
            }
            printf("time out, threshold = %d\n", threshold);
            flag_for_read = 0;
            if(to_send[0].head.seqNumber == curack + 1){
                flag_for_read = 1;
            }
            for(int i = 0; i < window; i++){
                if(flag_for_read == 1){
                    read[to_send[i].head.seqNumber] = 1;
                }
                if(to_send[i].head.seqNumber == curack){
                    flag_for_read = 1;
                }    
            }
            window = 1;
        }
    }
	return 0;
}