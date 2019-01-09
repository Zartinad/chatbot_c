#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "socket.h"

#ifndef PORT
  #define PORT 52072
#endif

sockname* usernames;
fd_set all_fds;

void string_append(char **original, char* tail);
void *Malloc(int size, char* desc);
void read_from(sockname *user);
int process_input(sockname *user);

int main(int argc, char const *argv[]) {

  struct sockaddr_in *self = init_server_addr(PORT);
  int sock_fd = set_up_server_socket(self, MAX_BACKLOG);

  // First, we prepare to listen to multiple file descriptors by initializing
  // a set of file descriptors.
  int max_fd = sock_fd;
  FD_ZERO(&all_fds);
  FD_SET(sock_fd, &all_fds);

  sockname *tmp; //used to iterate through socknames
  while (1) { //accept connections and select sockets that are ready for reading

      // select updates the fd_set it receives, so we always use a copy and retain the original.
      fd_set listen_fds = all_fds;
      int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
      if (nready == -1) {
          perror("server: select");
          exit(1);
      }

      // Is it the original socket? Create a new connection ...
      if (FD_ISSET(sock_fd, &listen_fds)) {
          int client_fd = accept_connection(sock_fd, &usernames);
          if (client_fd > max_fd) {
              max_fd = client_fd;
          }
          FD_SET(client_fd, &all_fds);
          write_output(client_fd, "Connected");

      }

      tmp = usernames; // reset
      while (tmp != NULL){ //loop through list to check who is ready for read
        if (tmp->sock_fd > -1 && FD_ISSET(tmp->sock_fd, &listen_fds)) {
          read_from(tmp);
        }
        tmp = tmp->next;
      }
  }

  //We should never reach here
  return 1;
}

/*
* Continue to partially read input from user until network newline character
* is found. Process the input when complete message is formed.
*/
void read_from(sockname *user) {

  int nbytes;
  nbytes = read(user->sock_fd, user->after, user->room);
  if (nbytes == -1){
    perror("read");
    exit(1);
  }

  //if user types 30+ chars then user->room == 0 -> nbytes == 0
  if (nbytes == 0){ // also checks if client disconnected by themselves
    disconnect_connection(user, &usernames, &all_fds);
    return;
  }

  /*Update buffer with partial reads. Check if network newline appears*/
  user->inbuf += nbytes;

  int where;
  while ((where = find_network_newline(user->buf, user->inbuf)) > 0) {

      user->buf[where - 2] = '\0'; //null-terminate at network newline

      if( process_input(user) != 0 ){//check for no premature exits
        return;
      }

      //Disregard used buffer content and shift with new content after network new line
      user->inbuf = user->inbuf - (strlen(user->buf) + 2);
      memmove(&user->buf[0], &user->buf[where], user->inbuf);
    }

  user->after = &user->buf[user->inbuf];
  user->room = sizeof(user->buf) - user->inbuf;
}

/*
* Process input and write to user the appropriate string.
*/
int process_input(sockname *user){

  write_output(user->sock_fd, user->buf);
}
