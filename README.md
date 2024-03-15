# Peer-to-Many Peer File Copy Program Using IPC

## Project Overview
This project aims to demonstrate a file copying mechanism from one peer to multiple peers, utilizing two different Inter-Process Communication (IPC) techniques: domain sockets (AF UNIX) and shared memory. The goal is to efficiently and effectively copy a file across multiple peers by leveraging these IPC mechanisms.

The first one ([socket.c](./socket.c)) involves the of domain sockets (AF UNIX). The sender starts a listening socket and waits for connections from each of the clients. The clients connect to the server, and for each such connection, the client forks a child process that servers the client. The server thereafter sends the file byte by byte to each of the clients using the write() system call. The source file, that is to be copied could be any file of your choice.

The second part ([sharedmem.c](./sharedmem.c)) involves sharing the file contents to peers using shared memory. The sender process reads bytes from the source file and populates the shared memory region. The receiving peer processes would be reading the bytes from the shared memory. The sender and receiver processes should be running simultaneously (and no sequentially). To ensure there are no race conditions you would require to use semaphores.

## Project Structure
- `README.md`: Contains the project overview, structure, and setup instructions.
- `Makefile`: Facilitates the compilation of the project by defining build tasks.
- `sharedmem.c`: Source code for the shared memory segment of the project.
- `socket.c`: Source code for the socket communication segment of the project.
- `Description.txt`: Provides additional information and details about the project.

## Setup Instructions

### Prerequisites
- GCC should be installed on your system to compile the source files. You can install GCC on Ubuntu using:
  ```bash
  sudo apt-get install build-essential
  ```

### Compiling the Project
1. Open a terminal and navigate to the root directory of the project.
2. Run the `make` command to compile the source files into executable binaries. The Makefile contains the necessary instructions for compiling both `sharedmem.c` and `socket.c`.
   ```bash
   cd path/to/Inter-Process-Communication-main
   make
   ```

### Running the Project
- **Socket-based IPC:**
  1. To start the server (listening socket), run:
     ```bash
     ./socket server
     ```
  2. To connect a client to the server, open a new terminal and run:
     ```bash
     ./socket client
     ```
- **Shared Memory IPC:**
  1. To start the sender process (writing to shared memory), run:
     ```bash
     ./sharedmem sender
     ```
  2. To start the receiver process (reading from shared memory), open a new terminal and run:
     ```bash
     ./sharedmem receiver
     ```

### Additional Notes
- Make sure all necessary dependencies and libraries are installed and properly set up before compiling and running the project.
- The actual execution commands might vary depending on the implementation of the source code and how it handles command-line arguments.
