#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <netinet/in.h>    /* Internet domain header, for struct sockaddr_in */

#define MAX_BACKLOG 5
#define BUF_SIZE 30 //30 is max before we force user to disconnect

/*
* Linked list struct to allow for unlimited connections. Contains buffer to
* hold partial reads.
}
*/
typedef struct sockname{
    int sock_fd;
    struct sockname *next;
    struct sockname *prev;

    char buf[BUF_SIZE];
    int inbuf;
    int room;  // How many bytes remaining in buffer?
    char *after; // Pointer to position after the data in buf

    int role; // -1 for neither, 0 for student, 1 for TA
    int state; //used to output correct string. Refer below:

}sockname;

// initialize server address
struct sockaddr_in *init_server_addr(int port);

// create socket for server to listen on
int set_up_server_socket(struct sockaddr_in *self, int num_queue);

//initialize a sockname struct with a given fd
sockname *initialiate_sockname(int fd);

// accept and conncetion from fd and create a user to add to user list
int accept_connection(int fd, sockname **usernames);

//find a network newline in a given buffer
int find_network_newline(const char *buf, int n);

// write a msg to fd with network newline
void write_output(int fd, char* msg);

// finds a user with a given name and role
sockname *find_socket(int role, char* name, sockname *usernames);

// disconnect user by closing their respective socket, removing them from user lists
// and removing their fd from fd_set all_fds
void disconnect_connection(sockname* user, sockname **usernames, fd_set *all_fds);

#endif
