#include "buffer.h"


buffer_pool_t buffer_pool = NULL;

// buffer_pool 을 생성함
int init_db(int num_buf){

  if (buffer_pool != NULL){
    printf("\nThere is already buffer_pool\n");
    return -1;
  } 
  buffer_pool = (buffer_pool_t)malloc(sizeof(_buffer_pool_t));
  if (buffer_pool == NULL){
    perror("buffer_pool creation fail in init_db");
    exit(EXIT_FAILURE);
  }

  // num_buf 개수의 buffer 를 생성함
  buffer_pool->buffer = (buffer_t)malloc(sizeof(_buffer_t) * num_buf);
  if (buffer_pool->buffer == NULL){
    perror("buffer creation fail in init_db");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < num_buf; i++){
    buffer_pool->buffer[i].is_dirty = 0;
    buffer_pool->buffer[i].is_pinned = 0;
    buffer_pool->buffer[i].referenced = 0;
  }
  buffer_pool->current = 0;
  buffer_pool->size = 0;
  buffer_pool->capacity = num_buf;
  
  return 0;
}


void printing_buffer(){
  _buffer_t tmp;
  printf("\n");
  for (int i = 0; i < buffer_pool->size; i++){
    tmp = buffer_pool->buffer[i];
    printf("buffer[%d] : table->%d / page_num->%d / is_dirty->%d / is_pinned->%d / referenced->%d\n", i, tmp.table_id, tmp.page_num, tmp.is_dirty, tmp.is_pinned, tmp.referenced);
  }
  printf("\ncurrent : %d\n\n", buffer_pool->current);
}

/*
 *void test(){
 *  for (int i = 0; i < buffer_pool->capacity; i++)
 *    if (buffer_pool->buffer[i].is_pinned == -1){
 *      printf("=============== at buffer[%d] ====================\n", i);
 *      printing_buffer();
 *      perror("pinned error");
 *      exit(EXIT_FAILURE);
 *    }
 *}
 */

// 다음 current 를 알려줌
int next_current(){
  return (buffer_pool->current + 1) % buffer_pool->size;
}

// file_manager 로부터 새로운 free page 를 할당받아 pagenum 을 반환한다
pagenum_t buffer_alloc_page(int table_id, header_t head){
  return file_alloc_page(table_id, head);
}

// index layer 로부터 table_id 와 page_num 을 받아 해당하는 페이지가 버퍼에 있으면 알려주고, 없으면 file_manager 에서 읽어들여 알려준다
int buffer_read_page(int table_id, pagenum_t page_num, page_t dest){

  int index;

  for (int i = 0; i < buffer_pool->size; i++){
    if (table_id == buffer_pool->buffer[i].table_id && page_num == buffer_pool->buffer[i].page_num){
      buffer_pool->buffer[i].is_pinned++;
      buffer_pool->buffer[i].referenced = 1;
      memcpy(dest, &(buffer_pool->buffer[i].frame), sizeof(_page_t));

      /*test();*/

      return i;  // return 값은 insert 가 아닌 find 일 경우 바로 unpin 을 하기 위해 버퍼상의 index 를 알려준다
    }
  }
  
  index = (buffer_pool->capacity != buffer_pool->size) ? (buffer_pool->size)++ : input_index();
  
  if (buffer_pool->buffer[index].is_dirty == 1){
    file_write_page(buffer_pool->buffer[index].table_id, buffer_pool->buffer[index].page_num, &buffer_pool->buffer[index].frame);
  }

  // 기존 버퍼에 없어서 file 로부터 읽어 들여서 dest 에 저장하여 return 한다 
  file_read_page(table_id, page_num, &(buffer_pool->buffer[index].frame));
  memcpy(dest, &(buffer_pool->buffer[index].frame), sizeof(_page_t));
  buffer_pool->buffer[index].table_id = table_id;
  buffer_pool->buffer[index].page_num = page_num;
  buffer_pool->buffer[index].is_dirty = 0;
  buffer_pool->buffer[index].is_pinned = 1;  // 읽기 위해 버퍼에 새로 할당 받은 것이므로 is_pinned = 1 이다
  buffer_pool->buffer[index].referenced = 1;

  /*test();*/
  
  return index;  // return 값은 insert 가 아닌 find 일 경우 바로 unpin 을 하기 위해 버퍼상의 index 를 알려준다

}

// index layer 로 부터 페이지를 받아 버퍼에 저장한다 | 기존 버퍼에 없던 페이지라면 clock policy 에 따라 한 버퍼를 삭제 후 해당 버퍼에 저장한다
int buffer_write_page(int table_id, pagenum_t page_num, page_t src, int is_dirty){

  int index;

  for (int i = 0; i < buffer_pool->size; i++){
    if (table_id == buffer_pool->buffer[i].table_id && page_num == buffer_pool->buffer[i].page_num){
      buffer_pool->buffer[i].is_pinned--;
      buffer_pool->buffer[i].referenced = 1;
      buffer_pool->buffer[i].is_dirty = (buffer_pool->buffer[i].is_dirty) ? (buffer_pool->buffer[i].is_dirty) : is_dirty;
      memcpy(&(buffer_pool->buffer[i].frame), src, sizeof(_page_t));

      /*test();*/

      return i;
    }
  }

  index = (buffer_pool->capacity != buffer_pool->size) ? (buffer_pool->size)++ : input_index();
  
  // 삭제해야 하는 페이지가 is_dirty 이면 파일에 저장
  if (buffer_pool->buffer[index].is_dirty == 1){
    file_write_page(buffer_pool->buffer[index].table_id, buffer_pool->buffer[index].page_num, &buffer_pool->buffer[index].frame);
  }

  memcpy(&(buffer_pool->buffer[index].frame), src, sizeof(_page_t));
  buffer_pool->buffer[index].table_id = table_id;
  buffer_pool->buffer[index].page_num = page_num;
  buffer_pool->buffer[index].is_dirty = is_dirty;
  buffer_pool->buffer[index].referenced = 1;
  buffer_pool->buffer[index].is_pinned = 0;  // 쓰기 위해 buffer 에 새로 할당 받은 것이므로 is_pinned = 0 이다

  /*test();*/
  
  return index;
  
}

// page_num 에 해당하는 페이지를 free_page 로 만든다
void buffer_free_page(int table_id, header_t head, pagenum_t page_num){
  buffer_t tmp;
  for (int i = 0; i < buffer_pool->size; i++){
    tmp = &buffer_pool->buffer[i];
    if (table_id == tmp->table_id && page_num == tmp->page_num){
      tmp->table_id = 0;
      tmp->page_num = 0;
      tmp->is_dirty = 0;
      tmp->is_pinned = 0;
      tmp->referenced = 0;
      break;
    }
  }
  file_free_page(table_id, head, page_num); 

  /*test();*/
}


// 버퍼 풀이 가득차서 버퍼 중 하나를 삭제해야 할 경우, 해당 버퍼의 인덱스
int input_index(){


  // 에러 가능성 다분함 --> 한바퀴 돌아서 제자리를 출력할 수 있음 
  buffer_pool->current = next_current();
  int start_current = buffer_pool->current;
  int index = start_current;

  int check_error = 0;

  while(1){

    check_error++;
    if (check_error > 100){
      perror("cannot find buffer in < input_index > at [ buffer.c ]");
      exit(EXIT_FAILURE);
    }

    // 아무도 참조 안할 때
    if (buffer_pool->buffer[index].is_pinned == 0){
      // 한바퀴 돌 동안 아무도 참조 안했을 때
      if (buffer_pool->buffer[index].referenced == 0){
        break;
      }
      // 참조가 됐을 때
      else {
        buffer_pool->buffer[index].referenced = 0;
      }
    }
    buffer_pool->current = next_current();
    index = buffer_pool->current;
    
  }
  
  return index;

}

void destroy_buffer(){
  free(buffer_pool->buffer);
  free(buffer_pool);
}

int shutdown_db(){
  _buffer_t tmp;
  printf("\n");
  for (int i = 0; i < buffer_pool->size; i++){
    tmp = buffer_pool->buffer[i];
    /*printf("buffer[%d] : %d\n", i, tmp.is_pinned);*/
    if (tmp.is_dirty == 1){
      file_write_page(tmp.table_id, tmp.page_num, &tmp.frame);
    }
  }
  destroy_buffer();
  return 0;
}

// table_id 에 해당하는 페이지들을 파일에 저장
int close_table(int table_id){
  buffer_t tmp;
  for (int i = 0; i < buffer_pool->size; i++){
    tmp = &buffer_pool->buffer[i]; 
    if (tmp->table_id == table_id && tmp->is_dirty == 1){
      file_write_page(table_id, tmp->page_num, &tmp->frame);
      tmp->table_id = 0;
      tmp->page_num = 0;
      tmp->is_dirty = 0;
      tmp->referenced = 0;
      tmp->is_pinned = 0;
    }
  }
  close_fd(table_id);
  return 0;
}


void pinning(int table_id, pagenum_t page_num){
  for (int i = 0; i < buffer_pool->size; i++){
    if (table_id == buffer_pool->buffer[i].table_id && page_num == buffer_pool->buffer[i].page_num){
      buffer_pool->buffer[i].is_pinned++;
      /*test();*/

    }
  }
}

void unpinning(int table_id, pagenum_t page_num){
  for (int i = 0; i < buffer_pool->size; i++){
    if (table_id == buffer_pool->buffer[i].table_id && page_num == buffer_pool->buffer[i].page_num){
      buffer_pool->buffer[i].is_pinned--;
      /*test();*/

    }
  }
}

int exist_in_buffer(int table_id, pagenum_t page_num){
  for (int i = 0; i < buffer_pool->size; i++){
    if (table_id == buffer_pool->buffer[i].table_id && page_num == buffer_pool->buffer[i].page_num){
      return 1;
    }
  }
  return 0;
}
