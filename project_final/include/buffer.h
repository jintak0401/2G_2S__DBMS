#ifndef __BUFFER_H__
#define __BUFFER_H__

#include  "buffer_structure.h"
#include <stdlib.h>


typedef struct _buffer_pool_t* buffer_pool_t;
typedef struct _buffer_pool_t{
  buffer_t buffer;
  int head_index[11];
  int current;
  int size;
  int capacity;
} _buffer_pool_t;



void printing_buffer();
void printing_head();

int next_current();
int input_index();

// make_internal 과 make_leaf 에서 불리는 함수 | 다른 page_num 을 저장하는 변수를 -1 로 초기화
page_t make_page( void );

// is_leaf 를 0 으로 초기화
internal_t make_internal( void );

// is_leaf 를 1 로 초기화
leaf_t make_leaf( void );

// 인자인 num_buf 크기를 가지는 buffer_pool 생성
int init_db(int num_buf);

// table_id 에 해당하는 페이지들을 파일에 저장
int close_table(int table_id);

// buffer_pool 의 모든 페이지들을 파일에 저장
int shutdown_db();

buffer_t find_buffer(int table_id, pagenum_t page_num);

page_t buffer_alloc_page(int table_id, header_t head, pagenum_t* page_num, int is_leaf);
page_t buffer_read_page(int table_id, pagenum_t page_num);
int buffer_write_page(int table_id, pagenum_t page_num, page_t src, int is_dirty);
int buffer_write_for_join(char* pathname, pagenum_t page_num, char src[PAGE_SIZE], int phase);
void buffer_read_for_join(char* pathname);
void buffer_free_page(int table_id, header_t head, pagenum_t page_num);



#endif /*__BUFFER_H__*/
