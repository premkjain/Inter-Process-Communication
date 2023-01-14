# Inter-Process-Communication-IPC
 Peer to many peer file copy program using IPC

Compilation:
    make clean
    make build

Execution
    ./socket <number-of-peers> <filename>
	./sharedmem <number-of-peers> <filename>

NOTE: ALL PEERS STORE RECEIVED FILES IN "received-files" DIRECTORY
Received Filename Format: 

DESCRIPTION FOR SOCKETS:
    1. Uses AF_UNIX family of sockets for IPC.
    2. Main process FORKS n clients and then acts as a server.
    3. A client waits for a successful connection, then reads data until socket is closed (i.e. read() returns 0).
    4. Server transmits file using write() and closes the client's socket once file is completely transmitted.
    5. Server exits once all n clients have been served.

DESCRIPTION FOR SHAREDMEMORY:
    1. Created a custom struct 'shmbuf' for shared memory portion:
        1.1. int sequencenumber; // to inform the client that fresh data is available. starts at 0
        1.2. char filename[MAXFILENAMELENGTH];  // name of file being transmitted
        1.3. char data[BLOCKSIZE];
        1.4. int filesize;
        1.5. int blocksize;
        1.6. int numberofreads; // to inform server how many clients have read the data

    2. LOGIC: The file has been split into multiple chunks of BLOCKSIZE = 80 kilobytes.
       Sender writes a chunk and increments sequencenumber by 1
       A change in sequencenumber indicates that fresh data is available for retrieval
       The receiver will detect a change in the sequencenumber, and fetch the newdata
       Subsequently after each block fetch, receiver will increment 'numberofreads' by 1 in the shared memory
       The client will now wait for sequencenumber to update.
       
       Now, when the server detects that numberofreads == numberofclients connected, the server will
        2.1. write the next block of data into (struct shmbuf).data
        2.2. update blocksize variable with number of bytes of filedata written to the shared memory
        2.3. reset numberofreads to 0
        2.4. increment sequencenumber by 1

       This will trigger the client to fetch the next block and repeat until numberofbytesreceived == (struct shmbuf).filesize is achieved

       SEMAPHORE USAGE: updation of numberofreads by n clients has been done safely using named semaphores
    
    3. Summary:
       The file is divided into multiple blocks. A block is written into shared memory and the server syncs with all the clients to ensure each block
       has been retrieved by the client, before writing a new block into shared memory.
