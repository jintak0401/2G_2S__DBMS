#include "log.h"
#include <pthread.h>

uint64_t LSN = 0;
uint64_t position = 0;
//long position = 0;
long trx_prev_LSN[20] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
checking_record_t checking_record = NULL;
//long* trx_prev_LSN;
std::list <log_record_t> log_tail;
int log_fd = open("log_file", O_RDWR | O_CREAT, 0644);

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t flush_mutex = PTHREAD_MUTEX_INITIALIZER;

log_record_t make_log(Type type, int tid, int prev_LSN, int table_id, pagenum_t page_num, int offset, char old_image[], char new_image[]){

  log_record_t log = (log_record_t)malloc(sizeof(_log_record_t));
  //log_record_t log = new _log_record_t();
  if (log == NULL){
    perror("[ log.cpp ] < make_log >");
    exit(EXIT_FAILURE);
  }
  log->LSN = LSN;
  log->prev_LSN = prev_LSN;
  log->tid = tid;
  log->type = type;
  log->table_id = table_id;
  log->page_num = page_num;
  log->offset = offset;
  if (type == UPDATE){
    log->length = 120;
  }
  else {
    log->length = 0;
  }
  if (old_image != NULL){
    strcpy(log->old_image, old_image);
  }
  if (new_image != NULL){
    strcpy(log->new_image, new_image);
  }

  if (type == UPDATE){
    LSN += sizeof(_log_record_t);
  }
  else {
    LSN += sizeof(_checking_record_t);
  }

  return log;
}

int log_buffer_write(Type type, int tid, int table_id, pagenum_t page_num, int index, char old_image[], char new_image[]){

  int ret_LSN; 

  // mutex ===================================================
  pthread_mutex_lock(&log_mutex);

  log_record_t log = make_log(type, tid, trx_prev_LSN[tid - 1], table_id, page_num, 128 * (index + 1) + 8, old_image, new_image);
  trx_prev_LSN[tid - 1] = log->LSN;

  log_tail.push_back(log);

  ret_LSN = log->LSN;

  if (type == COMMIT){
    printf("\n(!!!! tid : %d   commit!!!!)\n", tid);
    flush_log();     
  }

  pthread_mutex_unlock(&log_mutex);
  // mutex ===================================================

  return ret_LSN;
}

void flush_log(){

  pthread_mutex_lock(&flush_mutex);
  for (std::list<log_record_t>::iterator it = log_tail.begin(); it != log_tail.end(); it = log_tail.erase(it)){
    log_file_write(*it);  
    free(*it);
    *it = NULL;
    //delete (*it);
  }   
  pthread_mutex_unlock(&flush_mutex);
}

void log_file_write(log_record_t log){
  if (log->type != UPDATE){
    pwrite(log_fd, log, sizeof(_checking_record_t), log->LSN);
    if (position <= log->LSN){
      position += sizeof(_checking_record_t);
    }
  }
  else {
    pwrite(log_fd, log, sizeof(_log_record_t), log->LSN);
    if (position <= log->LSN){
      position += sizeof(_log_record_t);
    }
  }
}

void abort(int tid){
  log_record_t read_log = NULL;
  log_record_t write_log = NULL;
  leaf_t leaf;
  record_t record;

  // mutex=====================================================
  pthread_mutex_lock(&log_mutex);

  flush_log();

  int tmp = position - 1;

  read_log = (log_record_t)malloc(sizeof(_log_record_t));
  //read_log = new _log_record_t();
  /*
   *while(pread(log_fd, read_log, sizeof(_log_record_t), trx_prev_LSN[tid - 1]) > 0) {
   *  if (read_log->tid == tid){
   *    break;
   *  }
   *  else {
   *    tmp--;
   *  }
   *}
   */
  pread(log_fd, read_log, sizeof(_log_record_t), trx_prev_LSN[tid - 1]);

  while (read_log->type != BEGIN) {
    write_log = make_log(UPDATE, tid, read_log->prev_LSN, read_log->table_id, read_log->page_num, read_log->offset, read_log->new_image, read_log->old_image); 
    while ((leaf = (leaf_t)buffer_read_page(read_log->table_id, read_log->page_num)) == NULL); 



    record = &leaf->record[((read_log->offset - 8) / 128) - 1];

    printf("\n(!!! < tid : %d > UNDO!!!)  [ table %d ]'s [ page %d ]\t\t key : %ld\t\t < value :    %s", 
        tid, read_log->table_id, read_log->page_num, record->key, record->value);

    strcpy(record->value, read_log->old_image);

    printf("    ------>    %s     >\n", record->value);




    leaf->page_LSN = write_log->LSN;
    log_tail.push_back(write_log);
    buffer_write_page(write_log->table_id, write_log->page_num, (page_t)leaf, 1);
    pread(log_fd, read_log, sizeof(_log_record_t), read_log->prev_LSN);
  }

  //delete write_log;
  //delete read_log;

  write_log = make_log(ABORT, tid, -1, 0, 0, 0, NULL, NULL); 
  log_tail.push_back(write_log);
  flush_log();

  printf("\n(!!!! tid : %d   abort!!!!)\n", tid);

  pthread_mutex_unlock(&log_mutex);
  // mutex=====================================================
  
}

void recovery(){
  log_record_t log;
  leaf_t leaf;
  record_t record;
  bool loser[20];
  std::list<log_record_t> log_recovery;

  checking_record = (checking_record_t)malloc(sizeof(_checking_record_t));
  if (checking_record == NULL){
    printf("checking_record error < recovery > [ log.cpp ]");
    exit(EXIT_FAILURE);
  }

/*
 *  trx_prev_LSN = (long*)malloc(sizeof(long) * 20);
 *
 *  for (int i = 0; i < 20; i++){
 *    trx_prev_LSN[i] = -1;
 *  }
 */

  // loser 선별용 bool 배열 초기화
  for (int i = 0; i < 20; i++){
    loser[i] = false;
  }

  // log_file 로 부터 읽어들임
  while (true) {
    log = (log_record_t)malloc(sizeof(_log_record_t));
    //log = new _log_record_t();
    if (pread(log_fd, checking_record, sizeof(_checking_record_t), position) <= 0) {
      break;
    }
    if (checking_record->type != UPDATE){
      pread(log_fd, log, sizeof(_checking_record_t), position);
      position += sizeof(_checking_record_t); 
      LSN += sizeof(_checking_record_t);
    }
    // UPDATE
    else {
      pread(log_fd, log, sizeof(_log_record_t), position);
      position += sizeof(_log_record_t);
      LSN += sizeof(_log_record_t);
    }
    //log_tail.push_back(log);
    log_recovery.push_back(log);
    trx_prev_LSN[log->tid - 1] = log->LSN;
  }

  free(log);
  log = NULL;
  //delete log;

  // redo
  for (std::list<log_record_t>::iterator it = log_recovery.begin(); it != log_recovery.end(); it++){
    if ((*it)->type == BEGIN){
      loser[(*it)->tid - 1] = true;
    }
    // commit 이면 log_buffer 에서 log들 삭제
    if ((*it)->type == COMMIT || (*it)->type == ABORT){
      for (std::list<log_record_t>::iterator inner = log_recovery.begin(); inner != it; inner++){
        if ((*inner)->tid == (*it)->tid){
          free(*inner);
          *inner = NULL;
          //delete (*inner);
          inner = log_recovery.erase(inner);
          inner--; 
        }
      }
      loser[(*it)->tid - 1] = false;
      free(*it);
      it = log_recovery.erase(it);
      it--;
    } 
    // abort 일 경우 loser 에서 삭제
    /*
     *else if ((*it)->type == ABORT){
     *  loser[(*it)->tid - 1] = false;
     *}
     */
    // update 일 경우 redo 진행
    else if ((*it)->type == UPDATE){
      while ((leaf = (leaf_t)buffer_read_page((*it)->table_id, (*it)->page_num)) == NULL); 
      // consider
      if (leaf->page_LSN < (*it)->LSN) {





        record = &leaf->record[(((*it)->offset - 8) / 128) - 1];

        printf("\n(!!! < tid : %d > REDO!!!)  [ table %d ]'s [ page %d ]\t\t key : %ld\t\t < value :    %s", 
            (*it)->tid, (*it)->table_id, (*it)->page_num, record->key, record->value);

        strcpy(record->value, (*it)->new_image);

        printf("    ------>    %s     >\n", record->value);




        leaf->page_LSN = (*it)->LSN;
        buffer_write_page((*it)->table_id, (*it)->page_num, (page_t)leaf, 1);
      }
      else {
        buffer_write_page((*it)->table_id, (*it)->page_num, (page_t)leaf, 0);
      }
    }
  }

  for (std::list<log_record_t>::iterator it = log_recovery.begin(); it != log_recovery.end(); it = log_recovery.erase(it)) {
    free(*it);
    *it = NULL;
  }

  // loser 에 대해 undo
  for (int i = 0; i < 20; i++){
    if (loser[i] == true){
      abort(i + 1);
    }
  }
}

