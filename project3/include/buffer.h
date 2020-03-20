#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "file_manager.h"
#include <stdlib.h>

typedef struct _buffer_t* buffer_t;
typedef struct _buffer_t {
  _page_t frame;
  int table_id;
  int page_num;
  int is_dirty;
  int is_pinned;
  int referenced;
}_buffer_t;



typedef struct _buffer_pool_t* buffer_pool_t;
typedef struct _buffer_pool_t{
  buffer_t buffer;
  int current;
  int size;
  int capacity;
} _buffer_pool_t;


void printing_buffer();

int next_current();
int input_index();

// 인자인 num_buf 크기를 가지는 buffer_pool 생성
int init_db(int num_buf);

// table_id 에 해당하는 페이지들을 파일에 저장
int close_table(int table_id);

// buffer_pool 의 모든 페이지들을 파일에 저장
int shutdown_db();

pagenum_t buffer_alloc_page(int table_id, header_t head);
int buffer_read_page(int table_id, pagenum_t page_num, page_t dest);
int buffer_write_page(int table_id, pagenum_t page_num, page_t src, int is_dirty);
void buffer_free_page(int table_id, header_t head, pagenum_t page_num);

void unpinning(int table_id, pagenum_t page_num);
void pinning(int table_id, pagenum_t page_num);

int exist_in_buffer(int table_id, pagenum_t page_num);


#endif /*__BUFFER_H__*/
