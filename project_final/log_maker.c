#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#define ARR_SIZE 100

char log_buffer[4096];
long LSD = 0;
long trx_LSD[5] = {-1, -1, -1, -1, -1};
char trx_str[30][120];
int unit = 0;


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

void initial_str(int index) {
  int length = random() % 52 + 1;
  char tmp[60];
  char c;
  int i;
  for (i = 0; i < length; i++){
    if (i < 26){
      c = 'a' + i;
    }
    else {
      c = 'A' + i - 26;
    }
    tmp[i] = c;
  }
  tmp[i] = '\0';

  strcpy(trx_str[index], tmp);

}

void make_str(char new[], int tid, int index) {
  int length = random() % 52 + 1;
  char tmp[120];
  char num[10];
  char c;
  int i;
  /*
   *for (i = 0; i < length; i++){
   *  if (i < 26){
   *    c = 'a' + i;
   *  }
   *  else {
   *    c = 'A' + i - 26;
   *  }
   *  tmp[i] = c;
   *}
   *tmp[i] = '\0';
   */
  tmp[6] = '-';
  tmp[7] = '-';
  tmp[8] = '>';
  tmp[0] = 'A' + tid - 1;
  tmp[1] = '(';
  sprintf(num, "%d", 470 + index);
  strcat(tmp, num);
  tmp[5] = ')';
  tmp[9] = '\0';
  strcpy(new, trx_str[index]);
  strcat(new, tmp);
}

log_t make_log(int tid) {
  log_t log = (log_t)malloc(sizeof(_log_t));
  char new_image[120];
  log->LSD = LSD;
  log->prevLSD = trx_LSD[tid - 1];   
  log->tid = tid;
  if (trx_LSD[tid - 1] != -1){
    log->type = U;
  }
  else {
    log->type = B;
    return log;
  }
  log->table_id = 1;
  log->page_num = 14;
  int index = rand() % 31;
  log->offset = 128 * (index + 1) + 8;
  log->length = 120;
  strcpy(log->old_image, trx_str[index]);
  if (trx_str[index][1] != '('){
    trx_str[index][0] = '\0';
  }
  make_str(new_image, tid, index);
  strcpy(log->new_image, new_image);
  strcpy(trx_str[index], new_image);
  return log;
}

void append_log(log_t log){
  char tmp[500];
  /*sprintf(tmp, "%ld%ld%d%d%d%ld%d%d%s%s\n", log->LSD, log->prevLSD, log->tid, log->type, log->table_id, log->page_num, log->offset, log->length, log->old_image, log->new_image);*/
  memcpy(log_buffer + (unit * sizeof(_log_t)), log, sizeof(_log_t));
}

int main(){
  int tid;
  int key;
  FILE* fp = fopen("log_tail.txt", "w");
  /*FILE* fp2 = fopen("log_file.txt", "w");*/
  int fd = open("log_file", O_WRONLY | O_TRUNC | O_CREAT, 0644);
  char new_image[60];
  char old_image[60];
  log_t log;

  /*
   *trx_str[0][0] = '\0';
   *trx_str[1][0] = '\0';
   *trx_str[2][0] = '\0';
   *trx_str[3][0] = '\0';
   *trx_str[4][0] = '\0';
   */

  srand(time(NULL));

  for (int i = 0 ; i< 31; i++){
    initial_str(i);
  }
  log_t arr[ARR_SIZE];
  void* ptr;


  for (int i = 0; i < ARR_SIZE; i++){
    tid = rand() % 5 + 1;
    new_image[0] = '\0';
    old_image[0] = '\0';
    log = make_log(tid);
    arr[i] = log;

    trx_LSD[tid - 1] = LSD;

    fprintf(fp, "LSD->%ld / prevLSD->%ld / tid->%d / type->%d / table_id->%d / page_num->%d / offset->%d / length->%d / old_image->%s / new_image->%s\n",
        log->LSD, log->prevLSD, log->tid, log->type, log->table_id, log->page_num, log->offset, log->length, log->old_image, log->new_image);
    append_log(log);
    if (log->type == U){
      LSD += sizeof(_log_t);
    }
    else {
      LSD += sizeof(_checking_t);
    }
  }

  ptr = (void*)arr[0];
  int position = 0;
  for (int i = 0; i < ARR_SIZE; i++){
    if (arr[i]->type == U){
      pwrite(fd, arr[i], sizeof(_log_t), arr[i]->LSD);
      position += sizeof(_log_t);
    }
    else {
      pwrite(fd, arr[i], sizeof(_checking_t), arr[i]->LSD);
      position += sizeof(_checking_t);
    }
  }
  for (int i = 0; i < 3; i++){
    log = (log_t)malloc(sizeof(_log_t));
    log->tid = i + 1;
    log->LSD = LSD;
    LSD += sizeof(_checking_t);
    log->prevLSD = trx_LSD[i];
    log->type = C;
    pwrite(fd, log, sizeof(_checking_t), log->LSD);
  }

  for (int i = 0; i < 31; i++){
    printf("%s\n", trx_str[i]);
  }

  fclose(fp);
  close(fd);
}
