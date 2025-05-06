#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

#define BUFFER_SIZE BUFSIZ

// 安全に write() を実行する関数（すべてのデータを送信する）
ssize_t write_all(int sockfd, const void *buf, size_t len) {
    size_t total_written = 0;
    while (total_written < len) {
        ssize_t written = write(sockfd, buf + total_written, len - total_written);
        if (written <= 0) {
            if (errno == EINTR) continue; // シグナル割り込み時はリトライ
            perror("write error");
            return -1;
        }
        total_written += written;
    }
    return total_written;
}

int main(int argc, char *argv[]) { 
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *hostname = argv[1];
    const char *port = argv[2];

    int sockfd;
    struct addrinfo hints, *res, *p;

    // SIGPIPE を無視（サーバー切断時に異常終了しないように）
    signal(SIGPIPE, SIG_IGN);

    // getaddrinfo() を使用してホスト名を解決
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    if (getaddrinfo(hostname, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    // 接続可能なアドレスを探してソケットを作成
    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0) {
            break; // 接続成功
        }

        close(sockfd);
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to connect to server\n");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);
    printf("Connected to %s on port %s\n", hostname, port);

    fd_set read_fds;
    char buf[BUFFER_SIZE];

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);

        int max_fd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            break;
        }

        // **標準入力からのデータを送信**
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            memset(buf, 0, BUFFER_SIZE);
            if (fgets(buf, BUFFER_SIZE, stdin) == NULL) {
                printf("EOF detected. Closing connection...\n");
                break;
            }

            // fgets() の改行を削除
            buf[strcspn(buf, "\n")] = '\0';

            if (write_all(sockfd, buf, strlen(buf)) < 0) {
                break;
            }
        }

        // **サーバーからの受信**
        if (FD_ISSET(sockfd, &read_fds)) {
            memset(buf, 0, BUFFER_SIZE);
            ssize_t nbytes = read(sockfd, buf, BUFFER_SIZE - 1);
            if (nbytes <= 0) {
                if (nbytes == 0) {
                    printf("Server closed the connection.\n");
                } else {
                    perror("read error");
                }
                break;
            }
            printf("Server: %s\n", buf);
        }
    }

    close(sockfd);
    printf("Connection closed.\n");
    return 0;
}