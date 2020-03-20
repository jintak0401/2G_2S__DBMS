#ifndef __LOG_H__
#define __LOG_H__

#include "bpt.h"
#include "buffer.h"
#include <list>

enum Type {BEGIN, UPDATE, COMMIT, ABORT};

typedef struct _checking_record_t* checking_record_t;
typedef struct _checking_record_t {
  long LSN;
  long prev_LSN;
  int tid;
  Type type;
  int table_id;
  unsigned int page_num;
  int offset;
  int length;
} _checking_record_t;

typedef struct _log_record_t* log_record_t;
typedef struct _log_record_t {
  long LSN;
  long prev_LSN;
  int tid;
  Type type;
  int table_id;
  unsigned int page_num;
  int offset;
  int length;
  char old_image[120];
  char new_image[120];
} _log_record_t;

log_record_t make_log(Type type, int tid, int prev_LSN, int table_id, pagenum_t page_num, int offset, char old_image[], char new_image[]);

log_record_t log_file_read();
void flush_log();

void log_file_write(log_record_t log);
int log_buffer_write(Type type, int tid, int table_id, pagenum_t page_num, int index, char old_image[], char new_image[]);

void abort(int tid);
void recovery();

#endif /*__LOG_H__*/
