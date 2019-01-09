#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>     /* inet_ntoa */
#include <netdb.h>         /* gethostname */
#include <sys/socket.h>

#include "socket.h"
void string_append(char **original, char* tail);
void *Malloc(int size, char* desc);

/*
 * Initialize a server address associated with the given port.
 */
struct sockaddr_in *init_server_addr(int port) {
    struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
    if (addr == NULL){
      perror("malloc");
      exit(1);
    }

    // Allow sockets across machines.
    addr->sin_family = PF_INET;

    // The port the process will listen on.
    addr->sin_port = htons(port);

    // Clear this field; sin_zero is used for padding for the struct.
    memset(&(addr->sin_zero), 0, 8);

    // Listen on all network interfaces.
    addr->sin_addr.s_addr = INADDR_ANY;

    return addr;
}

/*
 * Create and set up a socket for a server to listen on.
 */
int set_up_server_socket(struct sockaddr_in *self, int num_queue) {
    int soc = socket(PF_INET, SOCK_STREAM, 0);
    if (soc < 0) {
        perror("socket");
        exit(1);
    }

    // Make sure we can reuse the port immediately after the
    // server terminates. Avoids the "address in use" error
    int on = 1;
    int status = setsockopt(soc, SOL_SOCKET, SO_REUSEADDR,
        (const char *) &on, sizeof(on));
    if (status < 0) {
        perror("setsockopt");
        exit(1);
    }

    // Associate the process with the address and a port
    if (bind(soc, (struct sockaddr *)self, sizeof(*self)) < 0) {
        // bind failed; could be because port is in use.
        perror("bind");
        exit(1);
    }

    // Set up a queue in the kernel to hold pending connections.
    if (listen(soc, num_queue) < 0) {
        // listen failed
        perror("listen");
        exit(1);
    }

    return soc;
}

/*
* Initialize a new sockname struct that connects to fd
*/
sockname *initialiate_sockname(int fd){
    sockname *user = malloc(sizeof(sockname));
    if (user == NULL){
      perror("malloc for sockname");
      exit(1);
    }

    user->name = NULL;
    user->state = 0;
    user->role = -1;

    user->buf[0] = '\0';
    user->inbuf = 0;
    user->room = BUF_SIZE;
    user->after = user->buf;

    user->next = NULL;
    user->prev = NULL;

    user->sock_fd = accept(fd, NULL, NULL);
    if (user->sock_fd < 0) {
        perror("server: accept");
        close(fd);
        exit(1);
    }

    return user;

}

/* Accept a connection. Note that a new file descriptor is created for
 * communication with the client. The initial socket descriptor is used
 * to accept connections, but the new socket is used to communicate.
 * Return the new client's file descriptor or -1 on error.
 */
int accept_connection(int fd, sockname **usernames) {
    sockname *new_user = NULL;
    if ((*usernames) == NULL){ //socket list is empty

      new_user = initialiate_sockname(fd);
      (*usernames) = new_user;

    } else { //create new user and add to end of list

      sockname *tmp = *usernames;

      while(tmp->next != NULL){ //loop till last node
        tmp = tmp->next;
      }

      new_user = initialiate_sockname(fd);
      tmp->next = new_user;
      new_user->prev = tmp;

    }

    return new_user->sock_fd;
}

/*
* Write msg to file descriptor fd with a network newline at the end
*/
void write_output(int fd, char* msg){
  char *output = NULL;
  string_append(&output, msg);

  if (msg[strlen(msg)-1] == '\n'){ //replace new line with network new line
    output[strlen(msg)-1] = '\0';
  }

  string_append(&output, "\r\n");
  if (write(fd, output, strlen(output)) == -1 ){
    perror("write");
    exit(1);
  }

  free(output);

}

/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int n) {
    int index = 0;
    while (index <= n - 2){
      if (buf[index] == '\r'){
        if (buf[index + 1] == '\n'){
          return index + 2;
        }
      }
      index += 1;
    }
    return -1;
}

/*
* Returns the socket for the user with the target name and role. Return NULL
* if this user is not found.
* Precondition: role is 0 (student) or 1 (student)
*/
sockname *find_socket(int role, char *name, sockname *usernames){
    sockname *tmp = usernames;
    while(tmp != NULL){
      if(tmp->role == role && strcmp(name, tmp->name) == 0){
        return(tmp);
      }
      tmp = tmp->next;
    }

    return NULL;
  }

/*
* Disconnect the user from ther server by remove list of users and remove user's
* socket fd from set of socket all_fds
*/
void disconnect_connection(sockname* user, sockname **usernames, fd_set *all_fds){

    FD_CLR(user->sock_fd, all_fds); //remove socket from fd list and close pipe
    if (close(user->sock_fd) == -1){
      perror("close");
      exit(1);
    }

    /*Remove socket from linked list*/
    if(user->prev != NULL && user->next != NULL){ // sandwhich between 2 nodes
      user->prev->next = user->next;
      user->next->prev = user->prev;
    } else if (user->prev == NULL && user->next == NULL){ // only node in the list
      *usernames = NULL;
    }
     else if (user->prev == NULL && user->next != NULL) {
      // first in list and there is a node after
      *usernames = user->next;
      user->next->prev = NULL;
    } else if (user->next == NULL){ // last in list
        user->prev->next = user->next;
    }

    free(user->name);
    free(user);
  }
