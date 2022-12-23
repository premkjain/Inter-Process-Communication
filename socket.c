/* ____                       _  __                     _       _       _       
  |  _ \ _ __ ___ _ __ ___   | |/ /__ _ _ __ ___   __ _| |     | | __ _(_)_ __  
  | |_) | '__/ _ \ '_ ` _ \  | ' // _` | '_ ` _ \ / _` | |  _  | |/ _` | | '_ \ 
  |  __/| | |  __/ | | | | | | . \ (_| | | | | | | (_| | | | |_| | (_| | | | | |
  |_|   |_|  \___|_| |_| |_| |_|\_\__,_|_| |_| |_|\__,_|_|  \___/ \__,_|_|_| |_|
*/

/*Used domain sockets(AF_UNIX)*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define BACKLOG 100 // The size of listen() queue for the server

void server(const char *socketname, int numberofclients, const char *filename)
{
    struct sockaddr_un name;
    int connectionsocketfd, returnvalue;

    if ((connectionsocketfd = socket(AF_UNIX, SOCK_SEQPACKET, 0)) < 0)
    {
        perror("SERVER: socket");
        exit(EXIT_FAILURE);
    }

    memset(&name, 0, sizeof(name));

    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, socketname, sizeof(name.sun_path) - 1);

    printf("SERVER: Binding socket to address %s\n", socketname);
    if (bind(connectionsocketfd, (const struct sockaddr *)&name, sizeof(name)) < 0)
    {
        perror("SERVER: bind");
        exit(EXIT_FAILURE);
    }

    printf("SERVER: Listening on socket %d\n", connectionsocketfd);
    if (listen(connectionsocketfd, BACKLOG) < 0)
    {
        perror("SERVER: listen");
        exit(EXIT_FAILURE);
    }

    puts("SERVER: Waiting for connections...");
    fflush(stdout);

    for (int i = 0; i < numberofclients; i++)
    {
        int datasocketfd = accept(connectionsocketfd, NULL, NULL);

        if (datasocketfd < 0)
        {
            perror("SERVER: accept");
            fflush(stderr);
            continue;
        }

        printf("SERVER: Accepted connection on socket %d\n", datasocketfd);
        fflush(stdout);
        FILE *filepointer = fopen(filename, "r");
        if (filepointer == NULL)
        {
            perror("SERVER: fopen");
            close(datasocketfd);
            close(connectionsocketfd);
            exit(EXIT_FAILURE);
        }

        // Inform the filename to the client in a single write() call
        int filenamebytes = write(datasocketfd, filename, strlen(filename)+1);

        if (filenamebytes <= 0)
        {
            perror("SERVER: write");
            fflush(stderr);
            close(datasocketfd);
            fclose(filepointer);
            continue;
        }

        while (!feof(filepointer))
        {
            char buffer[50*1024];     // Asked to read() and write() BYTE-BY-BYTE in the question
            int bytesread = fread(buffer, sizeof(char), sizeof buffer, filepointer);

            if (ferror(filepointer))
            {
                perror("SERVER: fread");
                fprintf(stderr, "SERVER: Warning: Client %d may receive corrupted file\n", datasocketfd);
                fflush(stderr);
                close(datasocketfd);
                fclose(filepointer);
                continue;
            }

            int byteswritten = write(datasocketfd, buffer, bytesread);

            if (byteswritten <= 0)
            {
                perror("SERVER: write");
                fprintf(stderr, "SERVER: Warning: Client %d may receive corrupted file\n", datasocketfd);
                fflush(stderr);
                close(datasocketfd);
                fclose(filepointer);
                continue;
            }
        }
        printf("SERVER: Closing connection on socket %d\n", datasocketfd);
        fflush(stdout);
        close(datasocketfd);
        fclose(filepointer);
    }
}

void client(const char *socketname)
{
    // retries
    struct sockaddr_un serveraddr;
    int returnvalue;
    int datasocketfd;

    if ((datasocketfd = socket(AF_UNIX, SOCK_SEQPACKET, 0)) < 0)
    {
        perror("CLIENT: socket");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    memset(&serveraddr, 0, sizeof(serveraddr));

    serveraddr.sun_family = AF_UNIX;
    strncpy(serveraddr.sun_path, socketname, sizeof(serveraddr.sun_path) - 1);

    // printf("CLIENT: Connecting to address %s\n", socketname);
    while (connect(datasocketfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0);

    // Receive the filename from the server in a single read() call
    char outputfilename[256];
    if (read(datasocketfd, outputfilename, sizeof outputfilename) <= 0)
    {
        perror("CLIENT: read");
        fflush(stderr);
        close(datasocketfd);
        exit(EXIT_FAILURE);
    }

    char buffer[50*1024];
    char outputfilepath[512];
    int bytesread;

    sprintf(outputfilepath, "received-files/pid-%d_%s", getpid(), outputfilename);
    FILE *filepointer = fopen(outputfilepath, "w");
    if (filepointer == NULL)
    {
        perror("CLIENT: fopen");
        close(datasocketfd);
        exit(EXIT_FAILURE);
    }

    while ((bytesread = read(datasocketfd, buffer, sizeof buffer)) != 0)
    {
        if (bytesread < 0)
        {
            perror("CLIENT: read");
            fflush(stderr);
            close(datasocketfd);
            fclose(filepointer);
            exit(EXIT_FAILURE);
        }

        int byteswritten = fwrite(buffer, sizeof(char), bytesread, filepointer);

        if (ferror(filepointer))
        {
            perror("CLIENT: fwrite");
            fflush(stderr);
            close(datasocketfd);
            fclose(filepointer);
            exit(EXIT_FAILURE);
        }
    }

    close(datasocketfd);
    fclose(filepointer);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if (argc < 3)
    {
        fprintf(stderr, "USAGE: %s <number-of-peers> <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int numberofclients = atoi(argv[1]);
    char *filename = argv[2];

    if (numberofclients < 1)
    {
        fprintf(stderr, "Number of peers should be atleast 1. Received %d\n", numberofclients);
        exit(EXIT_FAILURE);
    }

    int parentPID = getpid();
    char socketname[128];
    sprintf(socketname, "/tmp/%d_%d", parentPID, rand());

    for (int i = 0; i < numberofclients; i++)
    {
        if (fork() == 0)
        {
            client(socketname);
            return 0;
        }
    }

    server(socketname, numberofclients, filename);
    return 0;
}
