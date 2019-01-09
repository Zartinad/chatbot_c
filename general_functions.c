#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>

/*
* Appends the tail to the original string by creating a new memory allocated
* string of their concatenation. The original string must be null or mutable
*/
void string_append(char **original, char* tail){
  char *new_buffer;

  if (*original == NULL){ //If empty, create new string and return
    if (asprintf(&new_buffer, "%s", tail) == -1){
      perror("asprintf");
      exit(1);
    }
    *original = new_buffer;

  } else { //concatonate original and new string
    if (asprintf(&new_buffer, "%s%s", *original, tail) == -1){
      perror("asprintf");
      exit(1);
    }
    free(*original);
    *original = new_buffer;
  }

}

/*
* malloc but with error checking
*/
void *Malloc(int size, char* desc){
  char *error_msg = NULL;
  string_append(&error_msg, "malloc for");
  string_append(&error_msg, desc);

  void *address = malloc(size);
  if (address == NULL){
    perror(error_msg);
    free(error_msg);
    exit(1);
  }
  return address;
}
