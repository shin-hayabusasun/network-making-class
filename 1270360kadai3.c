#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>

#define MAX_CLIENTS 10
#define PORT 20030

typedef struct {
    int sockfd;
    char name[50];
    int score;
} Client;

int main() {
    int server_fd, new_fd, max_fd, activity;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    fd_set read_fds;
    Client clients[MAX_CLIENTS];
    int client_count = 0;
    int target_number = rand() % 100 + 1;  // 1〜100のランダムな数字

    // サーバーソケットの作成
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d\n", PORT);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;

        // クライアントソケットをselectに追加
        for (int i = 0; i < client_count; i++) {
            FD_SET(clients[i].sockfd, &read_fds);
            if (clients[i].sockfd > max_fd) {
                max_fd = clients[i].sockfd;
            }
        }

        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // 新規接続
        if (FD_ISSET(server_fd, &read_fds)) {
            if ((new_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len)) == -1) {
                perror("accept");
                continue;
            }

            // クライアント情報の取得
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            printf("[FD:%d] Accept: %s:%d\n", new_fd, client_ip, ntohs(client_addr.sin_port));

            // クライアント名の受け取り
            char client_name[50];
            int len = recv(new_fd, client_name, sizeof(client_name), 0);
            if (len <= 0) {
                close(new_fd);
                continue;
            }
            client_name[len] = '\0';

            printf("[%d] start %s\n", new_fd, client_name);

            // クライアントをリストに追加
            clients[client_count].sockfd = new_fd;
            strcpy(clients[client_count].name, client_name);
            clients[client_count].score = 0;
            client_count++;

            // クライアントに歓迎メッセージを送信
            char welcome_msg[100];
            snprintf(welcome_msg, sizeof(welcome_msg), "%s joined.\nScores: ", client_name);
            send(new_fd, welcome_msg, strlen(welcome_msg), 0);
            for (int i = 0; i < client_count; i++) {
                char score_msg[100];
                snprintf(score_msg, sizeof(score_msg), "%s[%d] ", clients[i].name, clients[i].score);
                send(new_fd, score_msg, strlen(score_msg), 0);
            }
            send(new_fd, "\n", 1, 0);
        }

        // クライアントからのメッセージを処理
        for (int i = 0; i < client_count; i++) {
            if (FD_ISSET(clients[i].sockfd, &read_fds)) {
                char buffer[1024];
                int bytes_read = recv(clients[i].sockfd, buffer, sizeof(buffer), 0);
                if (bytes_read <= 0) {
                    // クライアントが切断した場合
                    printf("[%d] %s left.\n", clients[i].sockfd, clients[i].name);
                    close(clients[i].sockfd);
                    clients[i] = clients[client_count - 1];
                    client_count--;
                } else {
                    buffer[bytes_read] = '\0';
                    int guess = atoi(buffer);
                    printf("[%s]: %d\n", clients[i].name, guess);

                    // ゲームのロジック
                    if (guess == target_number) {
                        int points = 11 - client_count;
                        clients[i].score += points;
                        send(clients[i].sockfd, "Correct! You got ", 17, 0);
                        char points_msg[20];
                        snprintf(points_msg, sizeof(points_msg), "%d points\n", points);
                        send(clients[i].sockfd, points_msg, strlen(points_msg), 0);
                        target_number = rand() % 100 + 1; // 新しい数字
                    } else {
                        char hint[50];
                        if (guess < target_number) {
                            snprintf(hint, sizeof(hint), "Higher\n");
                        } else {
                            snprintf(hint, sizeof(hint), "Lower\n");
                        }
                        send(clients[i].sockfd, hint, strlen(hint), 0);
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
