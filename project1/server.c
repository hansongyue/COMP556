//
// Created by Zequn Jiang on 1/18/20.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>

#define MAX_SIZE 66000
#define LISTENQ 1024

int isEndWith(const char *str, char *end) {
    if (NULL == str || NULL == end)
    {
        return -1;
    }
    int m = strlen(str);
    int n = strlen(end);
    if (m < n || m == 0 || n == 0)
    {
        return -1;
    }
    while (n > 0)
    {
        if (str[m - 1] != end[n - 1])
        {
            return -1;
        }
        --m;
        --n;
    }
    return 0;
}

void pingPongIO(int connfd) {
    int n;
    unsigned char msg[MAX_SIZE];
    while ((n = read(connfd, msg, MAX_SIZE)) > 0) {
        unsigned int size = msg[0] + (msg[1] << 8);
        int time_sec = msg[2] + (msg[3] << 8) + (msg[4] << 16) + (msg[5] << 24);
        int time_usec = msg[6] + (msg[7] << 8) + (msg[8] << 16) + (msg[9] << 24);
        char data[size + 1];
        data[size] = 0;
        memcpy(data, msg + 10, size);
        printf("size=%d; time_sec=%d; time_user=%d\n", size, time_sec, time_usec);
        //printf("%s; len: %lu\n", data, strlen(data));
        if (n < 0) {
            printf("read error");
        }
        if ((n = write(connfd, msg, sizeof(msg))) <= 0) {
            printf("write error");
        }
    }
}

int parseRequest(char *s, char *method, char *uri, char * protocol) {
    const char *p0 = strchr(s, ' ');
    if (!p0) {
        printf("parse error");
        return -1;
    }
    int n = p0 - s;
    strncpy(method, s, n);
    method[n] = 0;
    const char *p1 = strchr(p0 + 1, ' ');
    if (!p1) {
        printf("parse error");
        return -1;
    }
    n = p1 - p0 - 1;
    strncpy(uri, p0 + 1, n);
    uri[n] = 0;
    const char *p2 = strchr(p1 + 1, '\r');
    if (!p2 && *(p2 + 1) != '\n') {
        printf("parse error");
        return -1;
    }
    n = p2 - p1 - 1;
    strncpy(protocol, p1 + 1, n);
    protocol[n] = 0;
    return 0;
}

int writeStatus(int connfd, int code) {
    int n;
    char msg[MAX_SIZE];
    char code_str[10];
    char content[20];
    sprintf(code_str, "%d", code);
    switch (code) {
        case 200:
            sprintf(content, "%s", "OK");
            break;
        case 404:
            sprintf(content, "%s", "Not Found");
            break;
        default:
            return -1;
    }
    //puts(code_str);
    strcpy(msg, "HTTP/1.1 ");
    strcat(msg, code_str);
    strcat(msg, " ");
    strcat(msg, content);
    strcat(msg, "\r\n");
    //puts(msg);
    if ((n = write(connfd, msg, strlen(msg))) <= 0) {
        printf("write error");
    }
    return 0;
}

int writeFile(int connfd, char *filename) {
    char *url;
    if((url = getcwd(NULL,0))==NULL){
        perror("getcwd error");
    }
    strcat(url, filename);
    //puts(url);
    FILE* fd = fopen(url, "r");
    char header[4096];
    char server[] = "Server: DWBServer\r\n";
    char content_type_plain[] = "Content-Type: text/plain;charset=utf-8\r\n";
    char content_type_html[] = "Content-Type: text/html;charset=utf-8\r\n";
    char content_type_option[] = "X-Content-Type-Options: nosniff\r\n";
    strcpy(header, server);
    if (isEndWith(url, ".html") == 0) {
        strcat(header, content_type_html);
    } else {
        strcat(header, content_type_plain);
    }
    //strcat(header, content_type_option);
    strcat(header, "\r\n");
    if (fd == NULL) {
        writeStatus(connfd, 404);
        write(connfd, header, sizeof(header));
        return -1;
    }
    writeStatus(connfd, 200);
    write(connfd, header, strlen(header));
    char buff[4096];
    while (fgets(buff, 4000, fd) != NULL) {
        write(connfd, buff, strlen(buff));
    }
    fclose(fd);
    return 0;
}

void webIO(int connfd, char *root_dir) {
    puts("web I/O starts");
    int n;
    char msg[4096];
    while ((n = read(connfd, msg, MAX_SIZE)) <= 0) {
        printf("read error");
        exit(1);
    }
    char method[40], uri[40], protocol[40];
    if ((parseRequest(msg, method, uri, protocol) < 0)) {
        printf("parse error");
        exit(1);
    }
    //puts(method);
    n = strncmp(method, "GET", 3);
    //printf("%s %s %s\n", method, uri, protocol);
    char file_name[1000];
    if (strncmp(root_dir, ".", 1) == 0) {
        strcpy(file_name, uri);
    } else {
        strcpy(file_name, "/");
        strcat(file_name, root_dir);
        strcat(file_name, uri);
    }
    //puts(file_name);
    writeFile(connfd, file_name);
    puts("web I/O ends");
}


int main(int argc, char **argv) {
    int listenfd, connfd, n;
    socklen_t client_len;
    struct sockaddr_in client_addr, server_addr;

    if (argc < 2) {
        printf("usage: <Port> (<Working Mode> <Root Directory of Static Files>)");
        exit(1);
    }

    int port = atoi(argv[1]);
    listenfd = socket(AF_INET, SOCK_STREAM, 0); // TCP Socket
    if (argc > 2 && argc != 4) {
        printf("usage: <Port> (<Working Mode> <Root Directory of Static Files>)");
        exit(1);
    }
    char *mode = "ping-pong";
    char *root_dir = ".";
    if (argc == 4) {
        mode = argv[2];
        root_dir = argv[3];
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {

    }
    listen(listenfd, LISTENQ);
    puts("waiting for connection");

    while (1) {
        client_len = sizeof(client_addr);
        connfd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len);
        char data[MAX_SIZE];
        puts("connection accepted");
        if (strcmp("www", mode) == 0) {
            webIO(connfd, root_dir);
        } else {
            pingPongIO(connfd);
        }
        close(connfd);
    }

}

