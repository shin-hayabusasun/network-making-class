#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int main (void) { 
    int po;
    printf("port:");
    scanf("%d",&po);
  printf("\n");
char mesg[BUFSIZ];
int sockfd, nbytes;
char buf [BUFSIZ];

//make ip class
struct sockaddr_in servaddr;
//sock make
if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
perror ("socket"); exit (1);
}
memset (&servaddr, 0, sizeof (servaddr));
//ip class set
servaddr.sin_family = AF_INET;
servaddr.sin_port = htons(po); /* echo port is 7 */
//netip set
if (inet_pton (AF_INET, "127.0.0.1", &servaddr.sin_addr) < 0) {
perror ("inet_pton"); exit (1);
}
//connect
if (connect (sockfd, (struct sockaddr *)&servaddr, sizeof (servaddr))
< 0) {
perror ("connect"); exit (1);
}

pid_t pid = fork();
if(pid<0){
    perror("fork failed");
    close(sockfd);
    exit(1);
}
if(pid==0){
    while(1){
        memset(buf,0,BUFSIZ);
        nbytes = read (sockfd, buf, sizeof (buf) - 1);
        buf [nbytes] = '\0'; /* 後ろにnull文字を追加 */
fputs (buf, stdout);
    }
}
else{
    while(1){
memset(mesg,0,BUFSIZ);
fgets(mesg,BUFSIZ,stdin);
nbytes = write (sockfd, mesg, strlen (mesg));

    }
    close(sockfd);
}
return 0;
}