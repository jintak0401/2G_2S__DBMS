#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define ARR_SIZE 20

char log_buffer[4096];
long LSD = 0;


enum Type {B, U, C, A};

typedef struct _checking_t* checking_t;
typedef struct _checking_t {
  long LSD;
  long prevLSD;
  int tid;
  enum Type type;
  int table_id;
  unsigned int page_num;
  int offset;
  int length;
} _checking_t;

typedef struct _log_t* log_t;
typedef struct _log_t {
  long LSD;
  long prevLSD;
  int tid;
  enum Type type;
  int table_id;
  unsigned int page_num;
  int offset;
  int length;
  char old_image[120];
  char new_image[120];
} _log_t;

log_t make_log() {
  log_t log = (log_t)malloc(sizeof(_log_t));
  return log;
}

char* changer(int num){
  switch(num){
    case 0 :
      return "BEGIN";
    case 1 :
      return "UPDATE";
    case 2 :
      return "COMMIT";
    case 3 :
      return "ABORT";
  }
}

void main(){

  int fd = open("log_file", O_RDWR | O_CREAT, 0644);
  log_t arr[ARR_SIZE];
  void* ptr = arr;
  int size = 0;
  log_t log;
  ssize_t s;

  for (int i = 0; i < ARR_SIZE; i++){
    arr[i] = (log_t)malloc(sizeof(_log_t));
  }
  /*
   *arr = (log_t*)malloc(sizeof(log_t));
   **arr = (log_t)malloc(sizeof(_log_t) * ARR_SIZE);
   */
  /*log = (log_t)malloc(sizeof(_log_t));*/

  /*
   *if ((s = pread(fd, log_buffer, sizeof(_log_t) * ARR_SIZE, 0)) > 0){
   *  for (int i = 0; i < s / sizeof(_log_t); i++){
   *    memcpy(arr[i], log_buffer + (i * sizeof(_log_t)), sizeof(_log_t));
   *  }
   *}
   */

  int i = 0;
  /*
   *while (pread(fd, arr[i], sizeof(_log_t), sizeof(_log_t) * i) > 0) {
   *  i++;
   *}
   */

  /*printf("%s\n\n\n\n", log_buffer);*/


  log = (log_t)malloc(sizeof(_log_t));
  checking_t check = (checking_t)malloc(sizeof(_checking_t));
  int position = 0;
  while (1) {
    if (pread(fd, check, sizeof(_checking_t), position) <= 0) {
      break;
    }
    else {
      if (check->type == U){
        pread(fd, log, sizeof(_log_t), position);
      }
      else {
        pread(fd, log, sizeof(_checking_t), position);
      }
      i++;
    }
    if (log->type == U){
      printf("LSD->%ld / prevLSD->%ld / tid->%d / type->%s / table_id->%d / page_num->%d / offset->%d / length->%d / old_image->%s / new_image->%s\n",
          log->LSD, log->prevLSD, log->tid, changer(log->type), log->table_id, log->page_num, log->offset, log->length, log->old_image, log->new_image);
      position += sizeof(_log_t);
    }
    else {
      printf("LSD->%ld / prevLSD->%ld / tid->%d / type->%s / table_id->%d / page_num->%d / offset->%d / length->%d \n",
          log->LSD, log->prevLSD, log->tid, changer(log->type), log->table_id, log->page_num, log->offset, log->length);
      position += sizeof(_checking_t);
    }
    printf("i-->%d\n\n\n", i);
  }

  close(fd);
}
