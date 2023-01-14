# Inter-Process-Communication-IPC
## Peer to many peer file copy program using IPC
You are supposed to design to use IPCs to copy a file from one peer to many

using two different IPC mechanisms. The first one involves the of domain sock-
ets (AF UNIX). The sender starts a listening socket and waits for connections
from each of the clients. The clients connect to the server, and for each such
connection, the client forks a child process that servers the client. The server
thereafter sends the file byte by byte to each of the clients using the write()
system call. The source file, that is to be copied could be any file of your choice.

The second part would involve sharing the file contents to peers using shared
memory. The sender process reads bytes from the source file and populates the
shared memory region. The receiving peer processes would be reading the bytes
from the shared memory. The sender and receiver processes should be running
simultaneously (and no sequentially). To ensure there are no race conditions
you would require to use semaphores.
