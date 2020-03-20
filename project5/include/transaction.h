#ifndef __TRANSACTION_H__
#define __TRANSACTION_H__

#include "bpt.h"
#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <list>
#include <algorithm>
#include <unordered_map>
#include <pthread.h>

// for acquire_record_lock
#define SUCCESS 0
#define NO_APPEND 1
#define CONFLICT 2
#define DEADLOCK 3
#define NO_RECORD 4

// for cycle_detection

extern pthread_mutex_t create_mutex;
extern pthread_mutex_t access_mutex;


typedef struct _lock_t _lock_t;
typedef struct _lock_t* lock_t;
typedef struct _trx_t _trx_t;
typedef struct _trx_t* trx_t;
typedef struct _table_slot_t _table_slot_t;
typedef struct _table_slot_t* table_slot_t;
typedef struct _log_t* log_t;
typedef struct _log_t _log_t;


enum lock_mode { S = 0, X = 1 };


struct _lock_t {
  int table_id; // yes
  pagenum_t page_num;  // yes
  int64_t key;  // yes
  enum lock_mode mode; // yes
  trx_t owner;  // yes
  std::list <lock_t> wait_lock; // yes
  lock_t record_lock[2];  // yes
  table_slot_t slot; // yes
  int index;  // yes
};

struct _trx_t {
  int tid;  // yes
  std::list <lock_t> holding_locks;  // yes
  lock_t wait_lock;  // yes
  std::list <log_t> log;  // yes
  pthread_cond_t cond;  // yes
};

struct _table_slot_t {
  int64_t* key;    // yes
  int num_keys;    // yes
  int table_id;    // yes
  pagenum_t page_num;  // yes
  lock_t* head;  // yes
  lock_t* tail;  // yes
  table_slot_t left;  // yes
  table_slot_t right;  // yes
};

struct _log_t {
  int table_id;
  int64_t key;
  pagenum_t page_num;
  char* old_value;
  char* new_value;
};

int begin_trx();
int end_trx(int tid);
int abort(int tid);

lock_t make_lock(int table_id, int64_t key, lock_mode mode, int tid);
void insert_lock(lock_t lock, table_slot_t slot, int index);
int is_record_unlock(lock_t lock);
int acquire_record_lock(int table_id, int64_t key, lock_mode mode, int tid, bool is_first);

int cycle_detection(lock_t target_lock, lock_t exist_lock, bool start);
void recover(trx_t trx);
void write_log(int table_id, pagenum_t page_num, int64_t key, char* old_value, char* values, int tid);

table_slot_t make_slot(int table_id, int64_t key);
int get_index_from_slot(int table_id, int64_t key, table_slot_t* slot);
table_slot_t get_slot(table_slot_t* slot, int table_id, int64_t key);

void unlocking(lock_t lock);
void sleep_trx(int tid);
void unlock_mutex();

#endif /*__TRANSACTION_H__*/
