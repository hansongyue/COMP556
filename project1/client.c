#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>

#define SA struct sockaddr

unsigned int domainToIP(char *domain) {
    /* variables for identifying the server */
    unsigned int server_addr;
    struct sockaddr_in sin;
    struct addrinfo *getaddrinfo_result, hints;

    /* convert server domain name to IP address */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /* indicates we want IPv4 */

    if (getaddrinfo(domain, NULL, &hints, &getaddrinfo_result) == 0) {
        server_addr = (unsigned int) ((struct sockaddr_in *) (getaddrinfo_result->ai_addr))->sin_addr.s_addr;
        freeaddrinfo(getaddrinfo_result);
    }
    return server_addr;
}

int main(int argc, char **argv)
{
    int		sockfd;
    struct sockaddr_in	servaddr;

    if (argc != 5) {
        printf("usage: <IP Address> <Port> <Size of Network Message> <Number of Sending Messages>");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("creating socket failed");
        exit(1);
    }

    char* hostname = argv[1];
    int port = atoi(argv[2]);
    unsigned short size = atoi(argv[3]);
    int cnt = atoi(argv[4]);
    printf("hostname: %s; port: %d; size: %d; cnt: %d\n", hostname, port, size, cnt);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);	/* daytime server */
    servaddr.sin_addr.s_addr = domainToIP(hostname);

    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0) {
        printf("connect error");
        exit(1);
    }

    char* original_data = "hello world";
    char data[size + 1];
    data[0] = 0;
    int n = size;
    while (n > 0) {
        strcat(data, original_data);
        n -= strlen(original_data);
    }
    data[size] = 0;
    //printf("%s\n", data);
    int total_len = 10 + size;
    long sum = 0;
    for (int i = 0; i < cnt; i++) {
        unsigned char msg[total_len];
        struct timeval start, end;
        bzero(msg, sizeof(msg));
        //printf("size=%d; time_sec=%ld; time_usec=%d\n", size, timeval.tv_sec, timeval.tv_usec);
        memcpy(msg, &size, 2);
        memcpy(msg + 2, &start.tv_sec, 4);
        memcpy(msg + 6, &start.tv_usec, 4);
        memcpy(msg + 10, data, size);
        gettimeofday(&start, NULL);
        if ((n = write(sockfd, msg, sizeof(msg))) <= 0) {
            printf("write error");
            exit(1);
        }
        if ((n = read(sockfd, msg, sizeof(msg))) <= 0) {
            printf("read error");
            exit(1);
        }
        //int size = msg[0] + (msg[1] << 8);
/*
        int time_sec = msg[2] + (msg[3] << 8) + (msg[4] << 16) + (msg[5] << 24);
        int time_usec = msg[6] + (msg[7] << 8) + (msg[8] << 16) + (msg[9] << 24);
*/
/*
        char data[size + 1];
        data[size] = 0;
        memcpy(data, msg + 10, size);
        printf("size=%d; time_sec=%d; time_user=%d\n", size, time_sec, time_usec);
        printf("%s; len: %d\n", data, strlen(data));
        printf("after receive message: time_sec=%ld; time_user=%d\n", timeval.tv_sec, timeval.tv_usec);
*/
        gettimeofday(&end, NULL);
        long diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        sum += diff;
        printf("ping pong delay time is %ld microseconds\n", diff);
    }

    double average_delay = sum / (double)cnt;
    double bandwidth = (size + 10) * 2 * 8 / average_delay;
    printf("average delay is: %f us\n", average_delay);
    printf("band width is: %f Mbps\n", bandwidth);
    return 0;
}