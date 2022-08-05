#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h> /* for memset */
#include <stdlib.h> /* for exit */
#include <arpa/inet.h> /* for inet_ntop */
#include <unistd.h> /* for fork */
#include <errno.h>
#include <sys/wait.h> /* for waitpid */
#include <signal.h> /* for sigaction */

#define PORT "3940"
#define BACKLOG 10

void sigchld_handler(int s)
{
    (void)s; /* quiet unused variable waring */

    /* waitpid() might overwrite errno, so we save and restore it */
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int status, sockfd, accept_sockfd;
    struct addrinfo hints, *res, *p;
    struct sockaddr_storage accept_addr;
    char ipstr[INET6_ADDRSTRLEN];
    socklen_t accept_struct_len = sizeof(accept_addr);
    struct sigaction sa;
    int yes = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; /* ipv4 or ipv6 */
    hints.ai_socktype = SOCK_STREAM; /* tcp */
    hints.ai_flags = AI_PASSIVE; /* fill my own ip address */

    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                    sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    freeaddrinfo(res);

    if (p == NULL) {
        fprintf(stderr, "No usable address is found. Exit.\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; /* reap all dead process */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while (1) {
        if ((accept_sockfd = accept(sockfd, (struct sockaddr *)&accept_addr, &accept_struct_len)) == -1) {
            perror("server: accept");
            continue;
        }
        inet_ntop(accept_addr.ss_family,
                get_in_addr((struct sockaddr *)&accept_addr),
                ipstr, sizeof(ipstr));
        printf("server: got connection from %s\n", ipstr);

        if (fork() == 0) {
            /* child process */
            close(sockfd);
            char *send_msg = "Hello, World\n";
            if (send(accept_sockfd, send_msg, 13, 0) == -1) {
                perror("send");
            }
            close(accept_sockfd);
            exit(0);
        } else {
            /* parent process */
            close(accept_sockfd);
        }
    }
    exit(0);
}
