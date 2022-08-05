#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> /* for getaddrinfo */
#include <string.h> /* for memset */
#include <stdlib.h> /* for exit */
#include <arpa/inet.h> /* for inet_ntop */
#include <unistd.h> /* for close */

#define PORT "3940"

/* get_in_addr:  get ipv4 or ipv6 address struct */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        /* ipv4 */
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char **argv)
{
    struct addrinfo hints, *res, *p;
    int status, sockfd;
    char ipaddr[INET6_ADDRSTRLEN], recv_msg[1024];
    size_t recv_num;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s [server hostname or ip address]\n", argv[0]);
        exit(1);
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(argv[1], PORT, &hints, &res)) != 0) {
        fprintf(stderr, "client: getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "no usable address is found.\n");
        freeaddrinfo(res);
        close(sockfd);
        exit(1);
    }

    inet_ntop(p->ai_family, get_in_addr(p->ai_addr), ipaddr, INET6_ADDRSTRLEN);
    printf("connecting to %s ... OK\n", ipaddr);
    freeaddrinfo(res);

    if ((recv_num = recv(sockfd, (void *)recv_msg, 1024, 0)) <= 0) {
        perror("client: recv");
        close(sockfd);
        exit(1);
    }
    recv_msg[recv_num] = '\0';
    printf("got message: %s", recv_msg);
    close(sockfd);
    exit(0);
}

