//author: 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>


int main(int argc, char* argv[]){
    char* hostname = argv[1];
    int start = atoi(argv[2]);
    int end = atoi(argv[3]);
    int ret = 0;
    int res = 0;
    char* response = NULL;
    int result = 0;

    char ip[50];
    const char* str = "GET / HTTP/1.0\r\n\r\n";
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct hostent* gethost = gethostbyname(hostname);
    int SERV_PORT = start;
    struct sockaddr_in serverAddr;

    inet_ntop(gethost->h_addrtype, gethost->h_addr_list[0], ip, sizeof(ip));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(SERV_PORT);
    
    while (SERV_PORT < end) {
    	result = connect(sockfd, (const struct sockaddr*) &serverAddr, sizeof(serverAddr));
    	if (result < 0) {
        	printf(".");
    	}
    	else {
        	ret = write(sockfd, str, strlen(str));
        	if (ret < 0) {
            		printf(".");
        	}
        	else {
            		res = read(sockfd, response, 3);
            		if (res < 3 || atoi(response) != 200) { 
                		printf(".");
            		} 
					else {
						printf("%d\n", SERV_PORT);
						close(sockfd);
						return 0;
					}
        	}
        }
    	close(sockfd);
    	SERV_PORT = SERV_PORT + 1;
    	serverAddr.sin_port = htons(SERV_PORT);
    }
    printf("\n");
    return 0;
}

