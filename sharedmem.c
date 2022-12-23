/* ____                       _  __                     _       _       _       
  |  _ \ _ __ ___ _ __ ___   | |/ /__ _ _ __ ___   __ _| |     | | __ _(_)_ __  
  | |_) | '__/ _ \ '_ ` _ \  | ' // _` | '_ ` _ \ / _` | |  _  | |/ _` | | '_ \ 
  |  __/| | |  __/ | | | | | | . \ (_| | | | | | | (_| | | | |_| | (_| | | | | |
  |_|   |_|  \___|_| |_| |_| |_|\_\__,_|_| |_| |_|\__,_|_|  \___/ \__,_|_|_| |_|
*/                                                                            

/*Using shared memory*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <fcntl.h>           /* For O_* constants */

#define BLOCKSIZE 80*1024
#define MAXFILENAMELENGTH 256
#define LOG_SEM_NAME "/os2021483"

sem_t* sem = NULL;

struct shmbuf
{
    int sequencenumber; // to inform the client that fresh data is available. starts at 0
    char filename[MAXFILENAMELENGTH];
    char data[BLOCKSIZE];
    int filesize;
    int blocksize;
    int numberofreads; // to inform server how many clients have read the data
};

void server(int shmid, int numberofclients, const char *filename)
{
    struct shmbuf *object = shmat(shmid, NULL, 0); // Attach to this process's address space
    strncpy(object->filename, filename, MAXFILENAMELENGTH);

    FILE *filepointer = fopen(filename, "r");
    if (filepointer == NULL)
    {
        perror("SERVER: fopen");
        exit(EXIT_FAILURE);
    }

    fseek(filepointer, 0L, SEEK_END);
    int filesize = ftell(filepointer);      // Just for printing progress to screen
    rewind(filepointer);

    object->filesize = filesize;

    while (!feof(filepointer))
    {
        // puts("SERVER: Sending a block");
        int bytesread = fread(object->data, sizeof(char), BLOCKSIZE, filepointer);

        if (ferror(filepointer))
        {
            perror("SERVER: fread");
            fflush(stderr);
            fclose(filepointer);
            exit(EXIT_FAILURE);
        }

        printf("\nSERVER: Written %d bytes out of %d ( %f%% )\n\n",
            object->sequencenumber*BLOCKSIZE + bytesread,
            filesize,
            100.0*((double)object->sequencenumber*BLOCKSIZE + bytesread)/(double)filesize
        );
        fflush(stdout);

        object->blocksize = bytesread;
        object->numberofreads = 0;
        object->sequencenumber++;
        
        while (object->numberofreads < numberofclients)
        {
            // printf("numberofreads: %d\n", object->numberofreads);
            fflush(stdout);
            // sleep(1);
        }
    }

    shmdt(object); // Detach shared memory segment
    puts("SERVER: Finished transferring entire file");
    /* Mark the shared memory segment for deallocation.  */ 
    shmctl(shmid, IPC_RMID, NULL); 
    fclose(filepointer);
}

void client(int shmid)
{
    struct shmid_ds sharedmemoryinfo;
    memset(&sharedmemoryinfo, 0, sizeof sharedmemoryinfo);
    struct shmbuf *object = shmat(shmid, NULL, 0); // Attach to this process's address space

    if (object == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    char outputfilepath[512];
    while(object->sequencenumber == 0);
    sprintf(outputfilepath, "received-files/pid-%d_%s", getpid(), object->filename);

    FILE *filepointer = fopen(outputfilepath, "w");
    if (filepointer == NULL)
    {
        perror("CLIENT: fopen");
        exit(EXIT_FAILURE);
    }

    int lastSequenceNumber = 0;
    int returnvalue;

    while (ftell(filepointer) < object->filesize)
    {
        while (lastSequenceNumber == object->sequencenumber);
        lastSequenceNumber++;

        int byteswritten = fwrite(object->data, 1, object->blocksize, filepointer);

        if (ferror(filepointer))
        {
            perror("CLIENT: fwrite");
            fflush(stderr);
            fclose(filepointer);
            exit(EXIT_FAILURE);
        }
        fflush(filepointer);
        printf("CLIENT %d: Written a block of data. Total received: %ld bytes\n", getpid(), ftell(filepointer));
        fflush(stdout);

        if (sem_wait(sem) != 0)
        {
            perror("sem_wait");
            exit(EXIT_FAILURE);
        }

        object->numberofreads++;        // Increment that I have read the data
        if (sem_post(sem) != 0)
        {
            perror("sem_post");
            exit(EXIT_FAILURE);
        }
    }

    shmdt(object); // Detach shared memory segment
    
    fclose(filepointer);
}

void clearSharedMemory(int shmid)
{
    void *sharedmem = shmat(shmid, NULL, 0);
    if (sharedmem == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    memset(sharedmem, 0, sizeof(struct shmbuf));

    if (shmdt(sharedmem) < 0)
    {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(LOG_SEM_NAME) < 0 && errno == EACCES)
    {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }
	if ((sem = sem_open(LOG_SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) < 0)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // sem_init(sem, 1 /* share semaphore b/w processes */, 1); 
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
    int key = parentPID + rand();

    int shmid = shmget(key, sizeof(struct shmbuf), 0644 | IPC_CREAT | IPC_EXCL); // Get shared memory
    if (shmid < 0)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // INITIALIZE SHARED MEMORY WITH 0s and INITIALIZE SEMAPHORE
    clearSharedMemory(shmid);

    // BEGIN FORKING CLIENTS

    for (int i = 0; i < numberofclients; i++)
    {
        if (fork() == 0)
        {
            client(shmid);
            return 0;
        }
    }

    server(shmid, numberofclients, filename);
    return 0;
}
