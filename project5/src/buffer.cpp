#include "buffer.h"
#include <pthread.h>
#include "transaction.h"

buffer_pool_t buffer_pool = NULL;
hash_t* hash = NULL;

pthread_mutex_t buffer_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t* buffer_page_mutex = NULL;

// buffer_pool 을 생성함
int init_db(int num_buf){

  int hash_size = num_buf;

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
  buffer_page_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t) * num_buf);
  if (buffer_pool->buffer == NULL || buffer_page_mutex == NULL){
    perror("buffer creation fail in [ buffer.cpp ] < init_db >\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < 11; i++){
    buffer_pool->head_index[i] = -1;
  }
  for (int i = 0; i < num_buf; i++){
    buffer_pool->buffer[i].frame = make_page();
    buffer_pool->buffer[i].is_dirty = 0;
    buffer_pool->buffer[i].is_pinned = 0;
    buffer_pool->buffer[i].referenced = 0;
    buffer_pool->buffer[i].index = -1;
    buffer_pool->buffer[i].left = NULL;
    buffer_pool->buffer[i].right = NULL;
    buffer_page_mutex[i] = PTHREAD_MUTEX_INITIALIZER;
  }
  buffer_pool->current = 0;
  buffer_pool->size = 0;
  buffer_pool->capacity = num_buf;

  hash = (hash_t*)malloc(sizeof(hash_t) * 11);
  for (int i = 1; i <= 10; i++){
    hash[i] = make_hash(hash_size);
  }
  
  return 0;
}


void printing_buffer(){
  _buffer_t tmp;
  printf("\n");
  for (int i = 0; i < buffer_pool->size; i++){
    tmp = buffer_pool->buffer[i];
    printf("buffer[%d] : table->%d / page_num->%ld / is_dirty->%d / is_pinned->%d / referenced->%d / index->%d ( 0x%p )\n", i, tmp.table_id, tmp.page_num, tmp.is_dirty, tmp.is_pinned, tmp.referenced, tmp.index, buffer_pool->buffer[i].frame);
  }
  printf("\ncurrent : %d", buffer_pool->current);
  printf("\nsize : %d\n\n", buffer_pool->size);
}

void printing_head(){
  _buffer_t tmp;
  printf("\n\n\n");
  for (int i = 1; i < 11; i++){
    if (buffer_pool->head_index[i] >= 0){
      int index = buffer_pool->head_index[i];
      tmp = buffer_pool->buffer[index];
      printf("buffer[%d] : table->%d / page_num->%ld / is_dirty->%d / is_pinned->%d / referenced->%d / index->%d ( 0x%p )\n", i, tmp.table_id, tmp.page_num, tmp.is_dirty, tmp.is_pinned, tmp.referenced, tmp.index, buffer_pool->buffer[i].frame);
    }
  }
  printf("\n\n");
}

// internal, leaf 를 만들기 위한 범용적인 page 생성함수
page_t make_page( void ) {
  page_t new_page = NULL;
  new_page = (page_t)calloc(1, sizeof(_page_t));
  if (new_page == NULL) {
    perror("Node creation.");
    exit(EXIT_FAILURE);
  }
  return new_page;
}

// make_page 를 이용하여 internal_page 를 만든다.
internal_t make_internal(){
  internal_t internal = (internal_t)make_page();
  internal->is_leaf = 0;
  return internal;
}

// make_page 를 이용하여 leaf_page 를 만든다.
leaf_t make_leaf( void ) {
  leaf_t leaf = (leaf_t)make_page();
  leaf->is_leaf = 1;
  return leaf;
}

// 다음 current 를 알려줌
int next_current(){
  return (buffer_pool->current + 1) % buffer_pool->size;
}

buffer_t find_buffer(int table_id, pagenum_t page_num){

  int hash_key = hash_function(hash[table_id], page_num);  

  // buffer 에 있으면 해당 frame, 없으면 NULL 을 return 한다
  return find_tree(hash[table_id]->table[hash_key], page_num); 
}

// file_manager 로부터 새로운 free page 를 할당받아 pagenum 을 반환한다
page_t buffer_alloc_page(int table_id, header_t head, pagenum_t* page_num, int is_leaf){

  int index, hash_key;
  _buffer_t target;

  *page_num = file_alloc_page(table_id, head);

  if (buffer_pool->capacity != buffer_pool->size){
    index = (buffer_pool->size)++;
  }
  else {
    index = input_index();
    if (buffer_pool->buffer[index].table_id != 0){
      target = buffer_pool->buffer[index];
      // 없애야 하는 버퍼가 헤더가 아닌 경우
      if (target.page_num != 0){
        hash_key = hash_function(hash[target.table_id], target.page_num);
        if (target.is_dirty == 1){
          file_write_page(target.table_id, target.page_num, target.frame);
        }
        // delete 이후에 index 가 바뀌어야 한다
        hash[target.table_id]->table[hash_key] = delete_tree(hash[target.table_id]->table[hash_key], &buffer_pool->buffer[index], &index);
      }
      else {
        buffer_pool->head_index[target.table_id] = -1; 
        if (target.is_dirty == 1){
          file_write_page(target.table_id, target.page_num, target.frame);
        }
      }
    }
  }

  memset(buffer_pool->buffer[index].frame, 0, sizeof(_page_t));
  buffer_pool->buffer[index].frame->is_leaf = is_leaf;
  buffer_pool->buffer[index].table_id = table_id;
  buffer_pool->buffer[index].page_num = *page_num;
  buffer_pool->buffer[index].is_dirty = 0;
  buffer_pool->buffer[index].is_pinned = 1;  // 읽기 위해 버퍼에 새로 할당 받은 것이므로 is_pinned = 1 이다
  buffer_pool->buffer[index].referenced = 1;
  buffer_pool->buffer[index].index = index;

  hash_key = hash_function(hash[table_id], buffer_pool->buffer[index].page_num);
  hash[table_id]->table[hash_key] = insert_tree(hash[table_id]->table[hash_key], &buffer_pool->buffer[index]);

  return buffer_pool->buffer[index].frame; 
}

// index layer 로부터 table_id 와 page_num 을 받아 해당하는 페이지가 버퍼에 있으면 알려주고, 없으면 file_manager 에서 읽어들여 알려준다
page_t buffer_read_page(int table_id, pagenum_t page_num){

  int index, hash_key;
  buffer_t toReturn;
  buffer_t target;


  // pool mutex lock ===========================================================
  if (pthread_mutex_trylock(&buffer_pool_mutex) != 0) {
    return NULL;
  }

  if (page_num == 0 && buffer_pool->head_index[table_id] >= 0){
    // page_latch  ********************************************************************
    if (pthread_mutex_trylock(&buffer_page_mutex[buffer_pool->head_index[table_id]]) == 0) {
      toReturn = &buffer_pool->buffer[buffer_pool->head_index[table_id]];
      toReturn->is_pinned++;
      toReturn->referenced = 1;
      pthread_mutex_unlock(&buffer_pool_mutex);
      return toReturn->frame;
    }
    else {
      pthread_mutex_unlock(&buffer_pool_mutex);
      return NULL;
    }
  }

  else if (page_num != 0){
    toReturn = find_buffer(table_id, page_num);

    if (toReturn != NULL){

    // page_latch  ********************************************************************
      if (pthread_mutex_trylock(&buffer_page_mutex[toReturn->index]) == 0) {
      toReturn->is_pinned++;
      toReturn->referenced = 1;
      pthread_mutex_unlock(&buffer_pool_mutex);
      // pool mutex unlock ===========================================================
      return toReturn->frame;

      }
      else {
        pthread_mutex_unlock(&buffer_pool_mutex);
        return NULL;
      }
    }
  }

  if (buffer_pool->capacity != buffer_pool->size){
    index = (buffer_pool->size)++;
  }
  else {
    index = input_index();
    if (buffer_pool->buffer[index].table_id != 0){
      target = &buffer_pool->buffer[index];
      // 없애야 하는 버퍼가 헤더가 아닌 경우
      if (target->page_num != 0){
        hash_key = hash_function(hash[target->table_id], target->page_num);
        if (target->is_dirty == 1){
          file_write_page(target->table_id, target->page_num, target->frame);
          target->is_dirty = 0;
        }
        hash[target->table_id]->table[hash_key] = delete_tree(hash[target->table_id]->table[hash_key], &buffer_pool->buffer[index], &index);
      }
      // 없애야 하는 버퍼가 헤더를 담고 있는 경우
      else {
        buffer_pool->head_index[target->table_id] = -1; 
        if (target->is_dirty == 1){
          file_write_page(target->table_id, target->page_num, target->frame);
          target->is_dirty = 0;
        }
      }
    }
  }

  file_read_page(table_id, page_num, buffer_pool->buffer[index].frame);

  // page_latch  ********************************************************************
  if (pthread_mutex_trylock(&buffer_page_mutex[index]) == 0) {
  buffer_pool->buffer[index].table_id = table_id;
  buffer_pool->buffer[index].page_num = page_num;
  buffer_pool->buffer[index].is_dirty = 0;
  buffer_pool->buffer[index].is_pinned = 1;  // 읽기 위해 버퍼에 새로 할당 받은 것이므로 is_pinned = 1 이다
  buffer_pool->buffer[index].referenced = 1;
  buffer_pool->buffer[index].index = index;

  if (page_num == 0){
    buffer_pool->head_index[table_id] = index;
  }
  else {
    hash_key = hash_function(hash[table_id], buffer_pool->buffer[index].page_num);
    hash[table_id]->table[hash_key] = insert_tree(hash[table_id]->table[hash_key], &buffer_pool->buffer[index]);
  }
  pthread_mutex_unlock(&buffer_pool_mutex);
  // pool MUTEX unlock ===========================================================
  
  return buffer_pool->buffer[index].frame;  // return 값은 insert 가 아닌 find 일 경우 바로 unpin 을 하기 위해 버퍼상의 index 를 알려준다

  }
  else {
    pthread_mutex_unlock(&buffer_pool_mutex);
    return NULL;
  }
}

// index layer 로 부터 페이지를 받아 버퍼에 저장한다 | 기존 버퍼에 없던 페이지라면 clock policy 에 따라 한 버퍼를 삭제 후 해당 버퍼에 저장한다
int buffer_write_page(int table_id, pagenum_t page_num, page_t src, int is_dirty){

  int index, hash_key;
  buffer_t toReturn;
  _buffer_t target;

  if (page_num == 0 && buffer_pool->head_index[table_id] >= 0){
    toReturn = &buffer_pool->buffer[buffer_pool->head_index[table_id]];
    toReturn->is_pinned--;
    toReturn->referenced = 1;
    toReturn->is_dirty = (toReturn->is_dirty) ? (toReturn->is_dirty) : is_dirty;
    *toReturn->frame = *src; 
    pthread_mutex_unlock(&buffer_page_mutex[toReturn->index]);
    // page_latch unlock *************************************************************
    return 1;
  }

  else if (page_num != 0){
    toReturn = find_buffer(table_id, page_num);

    if (toReturn != NULL){
      toReturn->is_pinned--;
      toReturn->referenced = 1;
      toReturn->is_dirty = (toReturn->is_dirty) ? (toReturn->is_dirty) : is_dirty;
      *toReturn->frame = *src; 
    pthread_mutex_unlock(&buffer_page_mutex[toReturn->index]);
    // page_latch unlock *************************************************************
      return 1;
    }
  }

  if (buffer_pool->capacity != buffer_pool->size){
    index = (buffer_pool->size)++;
  }
  else {
    index = input_index();
    if (buffer_pool->buffer[index].table_id != 0){
      target = buffer_pool->buffer[index];
      // 없애야 하는 버퍼가 헤더가 아닌 경우
      if (target.page_num != 0){
        hash_key = hash_function(hash[target.table_id], target.page_num);
        if (target.is_dirty == 1){
          file_write_page(target.table_id, target.page_num, target.frame);
        }
        hash[target.table_id]->table[hash_key] = delete_tree(hash[target.table_id]->table[hash_key], &buffer_pool->buffer[index], &index);
      }
      // 없애야 하는 버퍼가 헤더를 담고 있는 경우
      else {
        buffer_pool->head_index[target.table_id] = -1; 
        if (target.is_dirty == 1){
          file_write_page(target.table_id, target.page_num, target.frame);
        }
      }
    }
  }

  *buffer_pool->buffer[index].frame = *src; 
  buffer_pool->buffer[index].table_id = table_id;
  buffer_pool->buffer[index].page_num = page_num;
  buffer_pool->buffer[index].is_dirty = is_dirty;
  buffer_pool->buffer[index].referenced = 1;
  buffer_pool->buffer[index].is_pinned = 0;  // 쓰기 위해 buffer 에 새로 할당 받은 것이므로 is_pinned = 0 이다

  if (page_num == 0){
    buffer_pool->head_index[table_id] = index;
  }
  else {
    hash_key = hash_function(hash[table_id], buffer_pool->buffer[index].page_num);
    hash[table_id]->table[hash_key] = insert_tree(hash[table_id]->table[hash_key], &buffer_pool->buffer[index]);
  }

  pthread_mutex_unlock(&buffer_page_mutex[index]);
  // page_latch unlock *************************************************************
  
  return index;
  
}

// page_num 에 해당하는 페이지를 free_page 로 만든다
void buffer_free_page(int table_id, header_t head, pagenum_t page_num){

  int index, hash_key;
  buffer_t target;

  hash_key = hash_function(hash[table_id], page_num);

  target = find_buffer(table_id, page_num);
  
  hash[table_id]->table[hash_key] = delete_tree(hash[table_id]->table[hash_key], target, &index);

  file_free_page(table_id, head, page_num); 

}


// 버퍼 풀이 가득차서 버퍼 중 하나를 삭제해야 할 경우, 해당 버퍼의 인덱스
int input_index(){

  // 에러 가능성 다분함 --> 한바퀴 돌아서 제자리를 출력할 수 있음 
  buffer_pool->current = next_current();
  int start_current = buffer_pool->current;
  int index = start_current;

  int check_error = 0;

  while(1){

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

int buffer_write_for_join(char* pathname, pagenum_t page_num, char src[PAGE_SIZE], int phase){
  return file_write_for_join(pathname, page_num, src, phase);
}

void buffer_read_for_join(char* pathname){
  file_read_for_join(pathname);
}

void destroy_buffer(){
  for (int i = 0; i < buffer_pool->capacity; i++){
    free(buffer_pool->buffer[i].frame);
  }
  for (int i = 1; i <= 10; i++){
    free(hash[i]->table);
  }
  free(hash);
  free(buffer_pool->buffer);
  free(buffer_pool);
}

int shutdown_db(){
  buffer_t tmp;
  printf("\n");

  for (int table_id = 1; table_id <= 10; table_id++){
    for (int i = 0; i < hash[table_id]->table_size; i++){
      file_write_inorder(hash[table_id]->table[i]);
    }
    if (buffer_pool->head_index[table_id] >= 0){
      tmp = &buffer_pool->buffer[buffer_pool->head_index[table_id]];
      if (tmp->is_dirty){
        file_write_page(table_id, tmp->page_num, tmp->frame);
        tmp->table_id = 0;
        tmp->page_num = 0;
        tmp->is_dirty = 0;
        tmp->referenced = 0;
        tmp->is_pinned = 0;
      }
      buffer_pool->head_index[table_id] = -1;
    }
  }

  destroy_buffer();
  return 0;
}

// table_id 에 해당하는 페이지들을 파일에 저장
int close_table(int table_id){
  buffer_t tmp;

  for (int i = 0; i < hash[table_id]->table_size; i++){
    file_write_inorder(hash[table_id]->table[i]);
  }

  if (buffer_pool->head_index[table_id] >= 0){
    tmp = &buffer_pool->buffer[buffer_pool->head_index[table_id]];
    if (tmp->is_dirty){
      file_write_page(table_id, tmp->page_num, tmp->frame);
      tmp->table_id = 0;
      tmp->page_num = 0;
      tmp->is_dirty = 0;
      tmp->referenced = 0;
      tmp->is_pinned = 0;
    }
    buffer_pool->head_index[table_id] = -1;
  }

  close_fd(table_id);
  return 0;
}

