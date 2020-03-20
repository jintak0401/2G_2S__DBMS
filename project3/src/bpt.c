#include "bpt.h"
#include "buffer.h"
#include "file_manager.h"


void usage( void ) {
  printf("Enter any of the following commands after the prompt > :\n"
      "\ti <n>  <k>  <s> -- Insert <n> (a table_id) <k> (an integer) and <s> (a string) as value.\n"
      "\tf <n>  <k>  -- Find the value under table <n> key <k>.\n"
      "\tp <n>  <k> -- Print the page status at table <n> page <k> "
      "\td <n>  <k>  -- Delete key <k> at table <n> and its associated value.\n"
      "\tl <n> -- Print all leaves at table <n> (bottom row of the tree).\n"
      "\to <s> -- Open a new table, and its pathname is <s>.\n"
      "\tq -- Quit. (Or use Ctl-D.)\n"
      "\t? -- Print this help message.\n");
}

int is_changed = 0;
int root_stored = 0;

// key 인 값을 ret_val 에 저장 | 일치하는 key 존재시 0, 없을시 -1 반환
int db_find(int table_id, int64_t key, char* ret_val) {
  header_t head;
  pagenum_t page_num;
  record_t r;
  leaf_t leaf;
  int toReturn, buffer_index, head_buffer_index;

  // find 함수를 위한 헤더 생성
  head = make_header();
  buffer_read_page(table_id, HEADER_PAGE_NUM, (page_t)head);

  r = find(table_id, key, &page_num, head);

  // head 정보 더 이상 필요 없으므로 unpin 한다
  buffer_write_page(table_id, HEADER_PAGE_NUM, (page_t)head, 0);

  // 일치하는 key 가 없는 경우 
  if (r == NULL){
    printf("\nRecord not found under key %ld.\n", key);
    toReturn = -1;
  }
  // 일치하는 key 가 있는 경우 
  else {

    toReturn = 0;

    // 해당 leaf 의 모든 record 를 출력하기 위한 leaf 
    /*leaf = (leaf_t)malloc(sizeof(_leaf_t));*/
    /*
     *if (leaf == NULL){
     *  perror("leaf_creation");
     *  exit(EXIT_FAILURE);
     *}
     */
    /*buffer_index = buffer_read_page(table_id, page_num, (page_t)leaf);*/

    // 결과 출력
    strcpy(ret_val, r->value); 
    printf("\n[ page %ld ]\t\t key : %ld\t\t value : %s\n", page_num, key, ret_val);
    /*unpin(buffer_index);*/
    /*
     *printf("\n[ page %ld ]\n", page_num);
     *printing((page_t)leaf);
     */

  }
  // r & leaf 메모리 해제
  free(r);
  r = NULL;
  free(head);
  head = NULL;
  /*
   *free(leaf);
   *leaf = NULL;
   */
  return toReturn;
}


// 해당 key 가 있을 수 있는 leaf page 를 찾아가서 해당 page_num 을 반환하는 함수
leaf_t find_leaf(int table_id, int64_t key, pagenum_t* page_num, header_t head) {
  int i = 0;
  internal_t c;

  c = make_internal();
  *page_num = head->root_page_num;

  if (head->root_page_num == 0) {
    printf("\nEmpty tree.\n");
    free(c);
    c = NULL;
  }

  else {

    buffer_read_page(table_id, head->root_page_num, (page_t)c);

    // leaf 층 까지 가능한 경로를 따라 내려간다
    while (!c->is_leaf) {
      i = 0;
      while (i < c->num_keys) {
        if (key >= c->pair[i].key) i++;
        else break;
      }
      // i == 0 이라면 가장 왼쪽의 page 로 가야함
      if (i == 0){
        buffer_write_page(table_id, *page_num, (page_t)c, 0);
        *page_num = c->left_page_num;
        buffer_read_page(table_id, *page_num, (page_t)c);
      }
      else {
        // i >= 1 일 때 부터 (i - 1) 번째 pair 의 페이지로 가야함
        buffer_write_page(table_id, *page_num, (page_t)c, 0);
        *page_num = c->pair[i - 1].page_num;
        buffer_read_page(table_id, *page_num, (page_t)c);
      }
    }
    // pin 이 꽂힌 상태로 return 된다
  }

  // caller 에서 free 해줘야 함
  return (leaf_t)c;
}


// db_find 에게 호출되는 함수, 일치하는 key 값이 있으면 해당 record 주소 반환, 없으면 NULL 반환
record_t find(int table_id, int64_t key, pagenum_t* page_num, header_t head) {
  int i = 0;
  leaf_t leaf = find_leaf(table_id, key, page_num, head);

  if (leaf == NULL) return NULL;

  // c != NULL 일 때
  else {
    record_t toReturn = NULL;
    for (i = 0; i < leaf->num_keys; i++) {
      if (leaf->record[i].key == key) break;
    }
    if (i != leaf->num_keys) {
      toReturn = make_record(leaf->record[i].key, leaf->record[i].value);
    }
    
    // unpin 한다
    buffer_write_page(table_id, *page_num, (page_t)leaf, 0);

    free(leaf);
    leaf = NULL;


    // 해당 key 가 leaf 에 없는 경우(i == c->num_keys) NULL 이 반환된다. | caller 에서 메모리 해제해야 한다
    return toReturn;
  }
}

int cut( int length ) {
  if (length % 2 == 0)
    return length/2;
  else
    return length/2 + 1;
}


// INSERTION

// 인자로 받은 key 와 value 를 가지는 record 주소 반환
record_t make_record(int64_t key, char* value) {
  record_t new_record = (record_t)calloc(1, sizeof(_record_t));
  if (new_record == NULL) {
    perror("Record creation.");
    exit(EXIT_FAILURE);
  }
  else {
    new_record->key = key;
    strcpy(new_record->value, value);
  }
  return new_record;
}

pair_t make_pair(int64_t key, pagenum_t page_num){
  pair_t new_pair = (pair_t)calloc(1, sizeof(_pair_t));
  if (new_pair == NULL){
    perror("Pair creation.");
    exit(EXIT_FAILURE);
  }
  else {
    new_pair->key = key;
    new_pair->page_num = page_num;
  }
  return new_pair;
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



// 인자로 받은 left_page_num 이 존재하는 parent->pair 의 인덱스 반환 | 만약 parent->left_page_num 이라면 -1 반환
int get_left_index(internal_t parent, pagenum_t left_page_num) {

  int left_index = 0;
  // left 가 가장 왼쪽일 경우 -1 반환
  if (parent->left_page_num == left_page_num) {
    return -1;
  }
  // left 가 가장 왼쪽이 아닐 경우 해당 pair 의 인덱스 반환
  while (left_index < parent->num_keys){
    if (parent->pair[left_index].page_num == left_page_num){
      return left_index;
    }
    left_index++;
  }

  printf("\nthere is no same page_num  in parent   In get_left_index()\n");
  exit(EXIT_FAILURE);
}

// leaf 에 record 추가
void insert_into_leaf(leaf_t leaf, pagenum_t page_num, record_t record ) {

  int i, insertion_point;

  // 들어가야하는 지점 찾기
  insertion_point = 0;
  while (insertion_point < leaf->num_keys && leaf->record[insertion_point].key < record->key) {
    insertion_point++;
  }

  // insertion_point 뒤의 record 한 칸씩 물리기
  for (i = leaf->num_keys; i > insertion_point; i--) {
    leaf->record[i] = leaf->record[i - 1];
  }

  // record 집어넣기
  leaf->record[insertion_point] = *record;
  leaf->num_keys++;

  printf("\nkey : %ld\t\tvalue : %s\n", record->key, record->value);
  printf("is inserted at < Leaf > [ page %ld ]\n\n", page_num);

  // 해당 leaf 정보 출력해주는 함수
  /*printing((page_t)leaf);*/

}


// leaf 를 split 시키면서 record 집어 넣기 | 이 함수 내에서 file 에 반영한다 
void insert_into_leaf_after_splitting(int table_id, header_t head, internal_t root, leaf_t old_leaf, pagenum_t old_leaf_page_num, record_t record) {

  leaf_t new_leaf;  // new_leaf 는 이 함수에서 파일에 저장 & 메모리 해제
  record_t temp_records; // 이 함수에서 메모리 해제
  int insertion_index, split, new_key, i, j;
  pagenum_t new_leaf_page_num;
  pair_t pair; // parent 에 반영시킬 pair

  // 오른쪽 leaf 인 new_leaf 생성
  new_leaf = make_leaf();
  new_leaf_page_num = buffer_alloc_page(table_id, head); 

  // 다음 file_alloc_page 을 위해 미리 저장한다
  if (exist_in_buffer(table_id, new_leaf_page_num) == 0){
    buffer_write_page(table_id, new_leaf_page_num, (page_t)new_leaf, 1);
  }
  pinning(table_id, new_leaf_page_num);

  // 두 페이지에 나누기 위한 tmp 배열
  temp_records = (record_t)calloc( (MAX_ORDER_IN_LEAF + 1), sizeof(_record_t) );

  if (temp_records == NULL) {
    perror("Temporary records array.");
    exit(EXIT_FAILURE);
  }


  insertion_index = 0;
  while (insertion_index < MAX_ORDER_IN_LEAF && old_leaf->record[insertion_index].key < record->key) {
    insertion_index++;
  }

  // tmp 배열에 옮김
  for (i = 0, j = 0; i < old_leaf->num_keys; i++, j++) {
    if (j == insertion_index) j++;
    temp_records[j] = old_leaf->record[i];
  }

  temp_records[insertion_index] = *record;

  old_leaf->num_keys = 0;

  split = cut(MAX_ORDER_IN_LEAF);

  // left 에 record 저장
  for (i = 0; i < split; i++) {
    old_leaf->record[i] = temp_records[i];
    old_leaf->num_keys++;
  }

  // right 에 record 저장
  for (i = split, j = 0; i <= MAX_ORDER_IN_LEAF; i++, j++) {
    new_leaf->record[j] = temp_records[i];
    new_leaf->num_keys++;
  }

  // 각자 자신의 오른쪽 leaf 가리키기
  new_leaf->right_leaf_num = old_leaf->right_leaf_num;
  old_leaf->right_leaf_num = new_leaf_page_num;

  // 들어가있는 record 외에 모두 NULL 로 초기화
  for (i = old_leaf->num_keys; i < MAX_ORDER_IN_LEAF; i++) {
    memset(&old_leaf->record[i], 0, sizeof(_record_t));
  }
  for (i = new_leaf->num_keys; i < MAX_ORDER_IN_LEAF; i++) {
    memset(&new_leaf->record[i], 0, sizeof(_record_t));
  }

  // new_leaf_parent 고쳐주기
  new_leaf->parent_page_num = old_leaf->parent_page_num;

  // split 했으므로 heaer_page 의 페이지 개수 1 늘림 && file 에 저장
  if (use_free_page){
    use_free_page = 0;
    is_changed = 1;
  }
  else {
    head->num_pages++;
    is_changed = 1;
    use_free_page = 0;
  }

  // parent page 에 반영할 pair 생성
  pair = make_pair(new_leaf->record[0].key, new_leaf_page_num);

  // 결과 출력
  printf("\nkey : %ld\t\t\tvalue : %s\n", record->key, record->value);
  printf("is inserted at < Leaf > [ page %ld ]\n", ((new_leaf->record[0].key <= record->key) ? new_leaf_page_num : old_leaf_page_num));


// 디버깅용 출력함수 
/*
 *  printf("\n[ page %ld ]\n", old_leaf_page_num);
 *  printing((page_t)old_leaf);
 *  printf("\n[ page %ld ]\n", new_leaf_page_num);
 *  printing((page_t)new_leaf);
 *
 *  printf("\n*************************************************\n");
 *  printf("\n< Leaf > [ page %ld ] split to [ page %ld ]\n\n",  old_leaf_page_num, new_leaf_page_num);
 */

  // 페이지 늘어난 것을 parent 에 반영
  insert_into_parent(table_id, head, root, (page_t)old_leaf, old_leaf_page_num, pair, (page_t)new_leaf, new_leaf_page_num);

  // new_leaf 를 file 에 기록, old_leaf 는 insert 함수에서 저장한다
  buffer_write_page(table_id, new_leaf_page_num, (page_t)new_leaf, 1);

  // temp_records & new_leaf & pair 메모리 해제
  free(temp_records);
  temp_records = NULL;
  free(new_leaf);
  new_leaf = NULL;
  free(pair);
  pair = NULL;

}


// split 없이 internal 에 pair 추가 && 결과 file 에 저장
void insert_into_internal(internal_t n, pagenum_t n_page_num, int left_index, pair_t right) {
  int i;

  // 한칸씩 뒤로 물린 후 넣는다
  for (i = n->num_keys; i > left_index + 1; i--) {
    n->pair[i] = n->pair[i - 1];
  }
  n->pair[left_index + 1] = *right;
  n->num_keys++;
}


// internal_page 를 split 시키면서 pair 집어 넣기 | 이 함수 내에서 file 에 반영한다 
void insert_into_internal_after_splitting(int table_id, header_t head, internal_t root, internal_t old_internal, pagenum_t old_internal_page_num,
    int left_index, pair_t pair, page_t left, page_t right, pagenum_t left_page_num, pagenum_t right_page_num) {

  int i, j, split;
  internal_t new_internal;
  pair_t temp_pairs, k_prime;
  pagenum_t new_internal_page_num;

  // 두 페이지에 옮기기 위한 tmp 배열
  temp_pairs = (pair_t)calloc( (MAX_ORDER_IN_INTERNAL + 1), sizeof(_pair_t) );
  if (temp_pairs == NULL) {
    perror("Temporary pair array for splitting nodes.");
    exit(EXIT_FAILURE);
  }

  // tmp 에 옮겨 담는다 
  for (i = 0, j = 0; i < old_internal->num_keys; i++, j++) {
    if (j == left_index + 1) j++;
    temp_pairs[j] = old_internal->pair[i];
  }
  temp_pairs[left_index + 1] = *pair;

  split = cut(MAX_ORDER_IN_INTERNAL);

  // new_internal 생성 & new_internal 의 page_num 을 free_page 로 부터 받아온다
  new_internal = make_internal();
  new_internal_page_num = buffer_alloc_page(table_id, head);
  
  // 다음 file_alloc_page 을 위해 미리 저장한다
  if (exist_in_buffer(table_id, new_internal_page_num) == 0){
    buffer_write_page(table_id, new_internal_page_num, (page_t)new_internal, 1);
  }
  pinning(table_id, new_internal_page_num);
  /*pinning(table_id, new_internal_page_num);*/

  // pair 절반 old_page 에 할당
  old_internal->num_keys = 0;
  for (i = 0; i < split; i++) {
    old_internal->pair[i] = temp_pairs[i];
    old_internal->num_keys++;
  }

  // new_internal 의 가장 왼 쪽 페이지 = (split)번째 page_num
  new_internal->left_page_num = temp_pairs[split].page_num;

  // 나머지 pair 할당
  for (j = 0, i = split + 1; i <= MAX_ORDER_IN_INTERNAL; i++, j++) {
    new_internal->pair[j] = temp_pairs[i];
    new_internal->num_keys++;
  }

  // 들어가있는 pair 외에 모두 NULL 로 초기화
  for (i = old_internal->num_keys; i < MAX_ORDER_IN_INTERNAL; i++) {
    memset(&old_internal->pair[i], 0, sizeof(_pair_t));
  }
  for (i = new_internal->num_keys; i < MAX_ORDER_IN_INTERNAL; i++) {
    memset(&new_internal->pair[i], 0, sizeof(_pair_t));
  }

  // 해당 page 의 부모 page 에 반영하기 위한 k_prime
  k_prime = make_pair(temp_pairs[split].key, new_internal_page_num);

  // parent_page_num 수정
  new_internal->parent_page_num = old_internal->parent_page_num;

  // new_internal 의 자식 페이지의 parent_page_num 을 new_internal_page_num 으로 교체 && insert_parent 를 호출한 left 와 right 도 parent 가 바뀌었다면 수정
  change_parent_of_child(table_id, new_internal, new_internal_page_num, left_page_num, right_page_num);
  if (right_page_num == new_internal->left_page_num){
    right->parent_page_num = new_internal_page_num;
  }
  if (left_page_num == new_internal->left_page_num){
    left->parent_page_num = new_internal_page_num;
  }
  for (int loop = 0; loop < new_internal->num_keys; loop++){
    if (new_internal->pair[loop].page_num == right_page_num){
      right->parent_page_num = new_internal_page_num;
    } 
    if (new_internal->pair[loop].page_num == left_page_num){
      left->parent_page_num = new_internal_page_num;
    }
  }

  // split 해서 늘어난 page 수 head 에 반영 
  if (use_free_page){
    use_free_page = 0;
    is_changed = 1;
  }
  else {
    head->num_pages++;
    is_changed = 1;
    use_free_page = 0;
  }


  // 디버깅용 출력함수
  /*
   *printf("\n*************************************************\n");
   *printf("\n< Internal > [ page %ld ] split to [ page %ld ]\n",  old_internal_page_num, new_internal_page_num);
   */


  // split 한 결과를 parent 에 반영
  insert_into_parent(table_id, head, root, (page_t)old_internal, old_internal_page_num, k_prime, (page_t)new_internal, new_internal_page_num);

  // file 에 변경된 new_internal 을 저장한다 | old_internal 은 caller 쪽에서 해준다
  buffer_write_page(table_id, new_internal_page_num, (page_t)new_internal, 1);


  // new_internal & temp_pairs & k_prime 메모리 해제
  free(temp_pairs);
  temp_pairs = NULL;
  free(new_internal);
  new_internal = NULL;
  free(k_prime);
  k_prime = NULL;
}



// ***_after_splitting 함수들에게 불리는 함수로 split 결과를 parent 에 반영하는 함수
void insert_into_parent(int table_id, header_t head, internal_t root, page_t left, pagenum_t left_page_num, pair_t pair, page_t right, pagenum_t right_page_num) {

  int left_index, buffer_index;
  internal_t parent; 
  pagenum_t parent_page_num;

  // split 한 페이지가 루트여서 새 루트를 만들어야하는 경우
  if (left->parent_page_num <= 0) {

    // root 가 바뀐다
    insert_into_new_root(table_id, head, root, left, left_page_num, right, pair);

    // root 를 file 에 저장
    /*file_write_page(head->root_page_num, (page_t)root);*/
    return;
  }

  // 결과를 반영할 parent 읽어들이기 && 왼쪽 page 의 pair 상 인덱스 찾기
  parent = (internal_t)calloc(1, sizeof(_internal_t));
  if (parent == NULL){
    perror("parent creation.");
    exit(EXIT_FAILURE);
  }

  // parent 할당
  parent_page_num = left->parent_page_num;
  if (head->root_page_num == parent_page_num){
    memcpy(parent, root, sizeof(_page_t));
  }
  else {
    buffer_read_page(table_id, left->parent_page_num, (page_t)parent);
  }
  left_index = get_left_index(parent, left_page_num);

  // split 없이 parent 에 반영할 수 있을 경우
  if (parent->num_keys < MAX_ORDER_IN_INTERNAL){
    insert_into_internal(parent, left->parent_page_num, left_index, pair);
  }

  // split 을 해야하는 경우
  else {
    insert_into_internal_after_splitting(table_id, head, root, parent, left->parent_page_num, left_index, pair, left, right, left_page_num, right_page_num);
  }

  // buffer 에 parent 저장 & unpin
  buffer_write_page(table_id, parent_page_num, (page_t)parent, 1);

  // parent 가 root 였을 경우
  if (parent_page_num == head->root_page_num){
    /*memcpy(root, parent, sizeof(_internal_t));*/
    root_stored = 1;
  }

  free(parent);
  parent = NULL;
  // ***_after_splitting 에서 생성된 right, pair & 이 함수에서 생성된 parent 메모리 해제 | left 는 insert 함수 에서 해제
  /*
   *if (parent != root){
   *  free(parent);
   *  parent = NULL;
   *}
   */
}


// 기존 root 가 split 됐을 경우 새로운 root 를 생성하는 함수
void insert_into_new_root(int table_id, header_t head, internal_t root, page_t left, pagenum_t left_page_num, page_t right, pair_t right_pair) {

  // 바꿔줄 root 생성
  memset(root, 0, sizeof(_internal_t));
  pagenum_t root_page_num = buffer_alloc_page(table_id, head);

  if (exist_in_buffer(table_id, root_page_num) == 0){
    buffer_write_page(table_id, root_page_num, (page_t)root, 1);
  }
  pinning(table_id, root_page_num);

  // 새로운 root 에 자식 page 할당 && 자식 page 의 parent_page_num 변경
  root->left_page_num = left_page_num;
  left->parent_page_num = root_page_num;
  root->pair[0] = *right_pair;
  root->num_keys++;
  right->parent_page_num = root_page_num;

  // header_page 의 정보 변경 
  head->root_page_num = root_page_num;
  if (use_free_page) {
    use_free_page = 0;
    is_changed = 1;
  }
  else {
    use_free_page = 0;
    head->num_pages++;
    is_changed = 1;
  }

  // 디버깅용 출력함수
  /*
   *printf("\n*************************************************\n");
   *printf("\nnew root [ page %ld ] is created\n", head->root_page_num);
   */
}



// 맨 처음
void start_new_tree(int table_id, header_t head, internal_t root, record_t record) {

  memset(root, 0, sizeof(_internal_t));
  pagenum_t root_page_num = buffer_alloc_page(table_id, head);
  root->is_leaf = 1;

  if (exist_in_buffer(table_id, root_page_num) == 0){
    buffer_write_page(table_id, root_page_num, (page_t)root, 1);
  }
  pinning(table_id, root_page_num);

  // root 에 record 집어넣기
  insert_into_leaf((leaf_t)root, root_page_num, record);

  // header_page 에 변경된 num_pages 와 root_page_num 저장
  head->root_page_num = root_page_num;
  is_changed = 1;

  if (use_free_page){
    use_free_page = 0;
    is_changed = 1;
  }
  else {
    use_free_page = 0;
    head->num_pages++;
    is_changed = 1;
  }
}


// 가장 최초에 불리는 insertion 함수
int db_insert(int table_id, int64_t key, char* value) {

  record_t record, cmp_record;
  leaf_t leaf = NULL;
  pagenum_t page_num;
  header_t head;
  internal_t root;

  head = make_header();
  buffer_read_page(table_id, HEADER_PAGE_NUM, (page_t)head);

  cmp_record = find(table_id, key, &page_num, head);

  // 이미 일치하는 key 를 가지는 record 가 있을 경우 
  if (cmp_record != NULL) {
    printf("\nThere is already that record\n");

    // cmp_record 를 메모리 해제 
    buffer_write_page(table_id, HEADER_PAGE_NUM, (page_t)head, 0);
    free(head);
    head = NULL;
    free(cmp_record);
    cmp_record = NULL;
    return -1;
  }
  // record 생성
  record = make_record(key, value);
  root = make_internal();
  /*buffer_read_page(table_id, head->root_page_num, (page_t)root);*/

  // root 가 없는 경우 생성하고 record 를 삽입한다
  if (head->root_page_num == 0) {

    start_new_tree(table_id, head, root, record);
    printf("\nTree is created\n");
  }

  // root 가 있는 경우
  else {

    buffer_read_page(table_id, head->root_page_num, (page_t)root);

    // record 가 들어가야하는 leaf 찾기
    if (page_num == head->root_page_num){
      leaf = make_leaf();
      memcpy(leaf, root, sizeof(_page_t));
    }
    else {
      leaf = find_leaf(table_id, key, &page_num, head);
    }

    // split 이 필요없는 insert
    if (leaf->num_keys < MAX_ORDER_IN_LEAF) {
      insert_into_leaf(leaf, page_num, record);
    }

    // split 이 필요한 insert
    else {
      insert_into_leaf_after_splitting(table_id, head, root, leaf, page_num, record);
    }

    // leaf 를 file 에 저장
    buffer_write_page(table_id, page_num, (page_t)leaf, 1);

    if (page_num == head->root_page_num){
      root_stored = 1;
    }

  }

  // header 정보가 수정 되었다면 파일에 저장
  if (is_changed){
    buffer_write_page(table_id, HEADER_PAGE_NUM, (page_t)head, 1);
    is_changed = 0;
  }
  else {
    buffer_write_page(table_id, HEADER_PAGE_NUM, (page_t)head, 0);
  }

  // root 를 파일에 저장
  /*if ([>head->root_page_num != 0<] root_stored == 0 && head->root_page_num != page_num){*/
  if (root_stored == 0) {
    buffer_write_page(table_id, head->root_page_num, (page_t)root, 1);
  }
  else {
    root_stored = 0;
  }
  /*
   *else {
   *  buffer_write_page(table_id, head->root_page_num, (page_t)root, 0);
   *}
   */

  // leaf & record 메모리 해제
  if (leaf != NULL){
    free(leaf);
    leaf = NULL;
  }
  free(record);
  record = NULL;
  free(head);
  head = NULL;
  free(root);
  root = NULL;

  return 0;

}

// internal page 의 모든 자식들의 parent_page_num 을 고쳐주는 함수 
void change_parent_of_child(int table_id, internal_t parent, pagenum_t page_num, pagenum_t left, pagenum_t right){
  page_t child = (page_t)calloc(1, sizeof(_page_t));
  if (child == NULL){
    printf("\nI cannot make child in change_parent_of_child function\n");
    return;
  }
  if (parent->left_page_num != left && parent->left_page_num != right){
    buffer_read_page(table_id, parent->left_page_num, child);
    child->parent_page_num = page_num;
    buffer_write_page(table_id, parent->left_page_num, child, 1);
  }

  for (int i = 0; i < parent->num_keys; i++){
    memset(child, 0, sizeof(_page_t));
    if (parent->pair[i].page_num != left && parent->pair[i].page_num != right){
      buffer_read_page(table_id, parent->pair[i].page_num, child);
      child->parent_page_num = page_num;
      buffer_write_page(table_id, parent->pair[i].page_num, child, 1);
    }
  }
  free(child);
  child = NULL;
}


// DELETION.

// 기본적으로 page 가 속한 parent 의 pair 인덱스를 반환하나, page 가 가장 왼쪽 page 일 경우는 그 오른쪽 인덱스를 반환한다
int get_neighbor_index(page_t page, pagenum_t* page_num, internal_t parent, pagenum_t* neighbor_page_num) {

  int i;


  // page 가 가장 왼쪽 page 일 경우 -1 을 반환한다
  if (parent->left_page_num == *page_num){
    *neighbor_page_num = parent->pair[0].page_num;
    i = -1;
  }
  // 0 번 째 pair 에 속할 경우 0 을 반환한다.
  else if (parent->pair[0].page_num == *page_num){
    *neighbor_page_num = parent->left_page_num;
    i = 0;
  }
  // i ( > 0 ) 번 째 일 경우, 그 왼쪽 인덱스를 반환한다.
  else {
    for (i = 1; i < parent->num_keys; i++) {
      if (parent->pair[i].page_num == *page_num){
        *neighbor_page_num = parent->pair[i - 1].page_num;
        break;
      }
    }
  }
  return i;
}


void remove_entry_from_node(page_t page, pagenum_t* page_num, int64_t key) {

  int i;
  internal_t internal;
  leaf_t leaf;

  i = 0;

  // leaf 에서 remove 가 이루어질 때
  if (page->is_leaf){
    leaf = (leaf_t)page;
    while (leaf->record[i].key != key){
      i++;
    }

    // 삭제 예정 record 출력 
    printf("\nkey : %ld\t\t\tvalue : %s\n", leaf->record[i].key, leaf->record[i].value);
    printf("is deleted at < Leaf > [ page %ld ]\n", *page_num);

    // 한 칸씩 앞으로 당겨서 삭제함
    for (++i; i < leaf->num_keys; i++)
      leaf->record[i - 1] = leaf->record[i];
    // key 개수 반영
    leaf->num_keys--;

    // 마지막 record 초기화
    memset(&leaf->record[leaf->num_keys], 0, sizeof(_record_t));

    // 디버깅용 출력함수
    // 삭제 후 남은 record 출력
    /*
     *if (page->num_keys > 0){
     *  printf("\n*************************************************\n");
     *  printf("\n< Leaf > [ page %ld ]\n", *page_num);
     *  printing((page_t)leaf);
     *}
     */
  }

  // internal 에서 remove 가 이루어질 때
  else {
    internal = (internal_t)page;
    while (internal->pair[i].key != key){
      i++;
    }

    // 디버깅용 출력함수
    // 삭제 예정 record 출력 
    /*
     *printf("\n*************************************************\n");
     *printf("\nkey : %ld\t\t\tpage_num : %ld\n", internal->pair[i].key, internal->pair[i].page_num);
     *printf("is deleted at < Internal > [ page %ld ]\n", *page_num);
     */

    // 한 칸씩 앞으로 당겨서 삭제함
    for (++i; i < internal->num_keys; i++){
      internal->pair[i - 1] = internal->pair[i];
    }
    // pair 개수 반영
    internal->num_keys--;

    // 삭제 후 남은 pair 초기화
    memset(&internal->pair[internal->num_keys], 0, sizeof(_pair_t));
    
    // 디버깅용 출력함수
    // 삭제 후 남은 pair 출력
    /*
     *if (page->num_keys > 0){
     *  printf("\n*************************************************\n");
     *  printf("\n< Internal > [ page %ld ]\n", *page_num);
     *  printing((page_t)internal);
     *}
     */
  }
}


// root 정보가 수정됐다면 1, 아니라면 0 을 반환한다
int adjust_root(int table_id, header_t head, internal_t root, page_t page, page_t left_child) {

  pagenum_t root_page_num;

  // root 의 pair 혹은 record 가 아직 남아 있을 때
  if (page->num_keys > 0) {
    memcpy(root, page, sizeof(_page_t));
    return 0; 
  }

  // root 가 internal 이었으면서 모든 pair 가 삭제 됐을 때, 가장 왼쪽 page 를 root 로 삼는다
  if (!root->is_leaf) {

    // root 가 바뀔 것이므로 미리 root 의 page_num 할당 
    root_page_num = head->root_page_num;

    // head 의 root_page_num 을 최신화
    head->root_page_num = root->left_page_num;

    // 기존 root 를 free page 로 만들기 & 파일에 free_page 로 저장
    buffer_free_page(table_id, head, root_page_num);

    // 가장 왼쪽 page 를 root 로 만든다
    /*memcpy(root, left_child, sizeof(_page_t));*/
    buffer_read_page(table_id, head->root_page_num, (page_t)root);
    root->parent_page_num = 0;
    left_child->parent_page_num = 0;


    // 디버깅용 출력함수
    /*
     *printf("\n*************************************************\n");
     *printf("\n< Internal > [ page %ld ] becomes new Root page\n", head->root_page_num);
     */
  }

  // root 가 leaf 였으면서 모든 record 가 삭제 됐을 때
  else{
    // buffer_free_page 함수에서 free_page 로 만들어서 파일에 저장한다
    buffer_free_page(table_id, head, head->root_page_num);
    head->root_page_num = 0;
    root->num_keys = 0;

    root_stored = 1;

    // 디버깅용 출력함수
    /*
     *printf("\n*************************************************\n");
     *printf("\n< Root page > is [ page %ld ] deleted\n", head->root_page_num);
     */
    printf("\nAll pages are deleted\n");
  }

  // head 의 정보가 바뀌었음
  is_changed = 1;
  return 1;
}


// key 가 적어져서 merge 해야하는 경우 불리는 함수 | 기본적으로 왼쪽과 merge 한다
void coalesce_page(int table_id, header_t head, internal_t root, page_t page, page_t neighbor, internal_t parent, int neighbor_index, int64_t k_prime, 
    pagenum_t* page_num, pagenum_t* neighbor_page_num, pagenum_t* parent_page_num, page_t left_child, page_t right_child) {

  int i, j, neighbor_insertion_index, swap;
  leaf_t leaf;
  internal_t internal;
  page_t tmp;

  // key 개수가 적어져서 merge 해야하는 page 가 가장 왼쪽일 경우 그 오른쪽 page 와 하기 위해 neighbor 와 바꾼다 
  if (neighbor_index == -1) {
    
    tmp = (page_t)malloc(sizeof(_page_t));
    if (tmp == NULL){
      perror("cannot exchange page in coalesce_page function");
      exit(EXIT_FAILURE);
    }
    memcpy(tmp, page, sizeof(_page_t));
    memcpy(page, neighbor, sizeof(_page_t));
    memcpy(neighbor, tmp, sizeof(_page_t));

    free(tmp);
    tmp = NULL;

    swap = *page_num;
    *page_num = *neighbor_page_num;
    *neighbor_page_num = swap;

  }

  // merge 하기 위해, 남은 pair 혹은 record 를 옮기기 위한 인덱스
  neighbor_insertion_index = neighbor->num_keys;

  // merge 해야하는 page 가 internal 일 경우 
  if (!page->is_leaf) {

    // neighbor 의 자리에 pair 할당 
    internal = (internal_t)neighbor;
    internal->pair[neighbor_insertion_index].key = k_prime;
    internal->pair[neighbor_insertion_index].page_num = ((internal_t)page)->left_page_num;
    internal->num_keys++;

    // 나머지 자리 할당 
    for (i = neighbor_insertion_index + 1, j = 0; j < page->num_keys; i++, j++){
      internal->pair[i] = ((internal_t)page)->pair[j];
      internal->num_keys++;
    }

    // neighbor 의 새로운 자식의 parent_page_num 을 바꿔줌
    change_parent_of_child(table_id, internal, *neighbor_page_num, -1, -1);  // -1 임시로 넣어놈
    left_child->parent_page_num = *neighbor_page_num;
    right_child->parent_page_num = *neighbor_page_num;

  }

  // merge 해야하는 page 가 leaf 일 경우
  else {

    leaf = (leaf_t)neighbor;

    for (i = neighbor_insertion_index, j = 0; j < page->num_keys; i++, j++){
      leaf->record[i] = ((leaf_t)page)->record[j];
      leaf->num_keys++;
    }

    // 오른쪽 leaf 를 가리키도록 함
    leaf->right_leaf_num = ((leaf_t)page)->right_leaf_num;

    // merge 해야하는 page 가 가장 왼쪽 페이지거나 0 번째 pair 의 페이지 였다면 parent 의 left_page_num 을 neighbor 로 바꿔야 한다
    if (neighbor_index <= 0) {
      parent->left_page_num = *neighbor_page_num;
    }
  }

  // merge 됐으니 page 의 key 는 없다 
  page->num_keys = 0;

  // 디버깅용 출력함수
  // 결과출력
/*
 *  printf("\n*************************************************\n");
 *  printf("\n< %s > [ page %ld ] is merged into [ page %ld ]\n", 
 *      (page->is_leaf) ? "Leaf" : "Internal", *page_num, *neighbor_page_num);
 *
 *  if (page->is_leaf){
 *    printf("\n[ page %ld ]\n", *neighbor_page_num);
 *    printing(neighbor);
 *  }
 */

  buffer_write_page(table_id, *neighbor_page_num, (page_t)neighbor, 1);
  delete_entry(table_id, head, root, (page_t)parent, parent_page_num, k_prime, neighbor, page);

}

// 해당 key 가 존재할 때 불리는 함수
void delete_entry(int table_id, header_t head, internal_t root, page_t page, pagenum_t* page_num, int64_t key, page_t left_child, page_t right_child) {

  int min_keys;
  page_t neighbor;
  pagenum_t neighbor_page_num, parent_page_num;
  internal_t parent;
  int k_prime_index, neighbor_index;
  int64_t k_prime;
  int capacity;


  // 해당 key 가 존재하는 ( leaf / internal ) 의 ( record / pair ) 를 삭제
  remove_entry_from_node(page, page_num, key);

  // 데이터가 삭제된 페이지가 root 일 경우
  if (*page_num == head->root_page_num) {
    adjust_root(table_id, head, root, page, left_child);
    return;
  }

  // delayed merge 를 위해 leaf 의 최소 key 개수는 1개로 설정했다
  /*min_keys = page->is_leaf ? 1 : cut(MAX_ORDER_IN_INTERNAL) - 1;*/
  min_keys = 1;

  // 최소 key 개수를 만족하여 더 이상 바꿀 필요가 없을 때
  if (page->num_keys >= min_keys)
    return;
  
  // merge 가 일어나면 parent 정보를 수정하여야 하므로 할당한다
  parent = (internal_t)calloc(1, sizeof(_internal_t));
  if (parent == NULL){
    perror("parent creation.");
    exit(EXIT_FAILURE);
  }

  // parent 할당받고 get_neighbor_index 함수호출 | get_neighbor_index 함수에서 neighbor_page_num 할당받음
  parent_page_num = page->parent_page_num;
  if (parent_page_num == head->root_page_num){
    memcpy(parent, root, sizeof(_page_t));
  }
  else {
    buffer_read_page(table_id, parent_page_num, (page_t)parent);
  }
  neighbor_index = get_neighbor_index(page, page_num, parent, &neighbor_page_num);

  // parent 에서 삭제해야할 key 값과 인덱스
  k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
  k_prime = parent->pair[k_prime_index].key;

  // neighbor 할당
  neighbor = (page_t)calloc(1, sizeof(_page_t));
  if (neighbor == NULL){
    perror("neighbor creation.");
    exit(EXIT_FAILURE);
  }
  buffer_read_page(table_id, neighbor_page_num, neighbor);

  capacity = page->is_leaf ? MAX_ORDER_IN_LEAF : MAX_ORDER_IN_INTERNAL;

    
  // merge 하는 함수
  coalesce_page(table_id, head, root, page, neighbor, parent, neighbor_index, k_prime, page_num, &neighbor_page_num, &parent_page_num, left_child, right_child);

  // parent 도 merge 되었을 때 | head->next_free_page == parent_page_num 일 때는 adjust_root 함수에서 이미 free_page 로 만들어준다
  if (parent->num_keys == 0 && (head->next_free_page != parent_page_num)){
    buffer_free_page(table_id, head, parent_page_num);
    is_changed = 1;

    // 디버깅용 출력함수
    /*
     *printf("\n*************************************************\n");
     *printf("\n< Internal > [ page %ld ] becomes < Free page >\n", parent_page_num);
     */
  }
  

  // parent 와 neighbor 파일에 저장
  // 기존 root 가 삭제되어 neighbor 가 root 가 되었다면, 기존의 root 였던 parent 는 저장하지 않는다
  if (head->root_page_num != neighbor_page_num && parent->num_keys != 0 && (parent_page_num != head->root_page_num)){
    buffer_write_page(table_id, parent_page_num, (page_t)parent, 1);
  }
  
  /*buffer_write_page(table_id, neighbor_page_num, neighbor, 1);*/

  // parent 와 neighbor 메모리 해제
  if (parent != NULL){
    free(parent);
    parent = NULL;
  }
  if (neighbor != NULL){
    free(neighbor);
    neighbor = NULL;
  }
}



// 최상위 delete 함수
int db_delete(int table_id, int64_t key) {

  header_t head;
  internal_t root;
  leaf_t key_leaf;
  record_t key_record;
  pagenum_t page_num;
  int toReturn;

  head = make_header();
  buffer_read_page(table_id, HEADER_PAGE_NUM, (page_t)head);

  // key 와 일치하는 record 와 leaf 가 있는지 검사
  key_record = find(table_id, key, &page_num, head);

  // leaf 와 record 가 존재한다면 삭제 진행
  if (key_record != NULL) {

    root = make_internal();
    buffer_read_page(table_id, head->root_page_num, (page_t)root);
    key_leaf = make_leaf();
    
    if (page_num == head->root_page_num){
      memcpy(key_leaf, root, sizeof(_page_t));
    }
    else {
      buffer_read_page(table_id, page_num, (page_t)key_leaf);
    }

    // NULL 을 인자로 주는 이유는 adjust_root 함수에서 root 가 왼쪽 page 로 바뀌어야 할 때 해당 페이지를 인자로 넘겨주기 위해서
    delete_entry(table_id, head, root, (page_t)key_leaf, &page_num, key_record->key, NULL, NULL); 

    if (head->root_page_num == 0){
      root_stored = 1;
    }

    else if (head->root_page_num == page_num){
      buffer_write_page(table_id, page_num, (page_t)key_leaf, 1);
      root_stored = 1;
    }

    // num_keys == 0 이 되면, 즉 key_leaf 가 merge 되었다면, free page 로 만들어준다
    else if (key_leaf->num_keys == 0){

      // 버퍼에 저장
      buffer_free_page(table_id, head, page_num);
      is_changed = 1;
      // 디버깅용 출력함수
      /*
       *printf("\n*************************************************\n");
       *printf("\n< Leaf > [ page %ld ] becomes < Free page >\n", page_num);
       */
    }
    // key_leaf 가 free page 가 되지 않았다면 파일에 저장한다
    else if (root->num_keys != 0){
      buffer_write_page(table_id, page_num, (page_t)key_leaf, 1);
    }


    // head 정보가 수정되었다면 파일에 저장한다
    if (is_changed){
      buffer_write_page(table_id, HEADER_PAGE_NUM, (page_t)head, 1);
      is_changed = 0;
    }
    else {
      buffer_write_page(table_id, HEADER_PAGE_NUM, (page_t)head, 0);
    }

    // tree 가 삭제되지 않았다면 (head->root_page_num == 0 이라면 삭제된 것이다) root 를 파일에 저장한다
    if (root_stored == 0){
      buffer_write_page(table_id, head->root_page_num, (page_t)root, 1);
    }
    else {
      root_stored = 0;
    }
    toReturn = 0;

    free(head);
    head = NULL;
    free(root);
    root = NULL;
    free(key_leaf);
    key_leaf = NULL;
  }

  else {
    printf("\n< key : %ld >\n", key);
    printf("is not in a tree\n");
    toReturn = -1;
    buffer_write_page(table_id, HEADER_PAGE_NUM, (page_t)head, 0);
    free(head);
    head = NULL;
  }

  // leaf 가 root 가 아니었을 때 메모리 해제
  if (key_record != NULL){
    free(key_record);
    key_record = NULL;
  }
  return toReturn;
}



/*
 *각종 출력 및 디버깅용 함수
 */



void printing(int table_id, page_t page){
  leaf_t leaf;
  internal_t internal;
  printf("\nparent : [ page %ld ]\n\n", page->parent_page_num);
  if (page->is_leaf){
    leaf = (leaf_t)page;
    for (int i = 0; i < leaf->num_keys; i++){
      printf("key : %ld\t\t\t value : %s\n", leaf->record[i].key, leaf->record[i].value);
    }

  }
  else {
    internal = (internal_t)page;
    printf("\t\t\t\t left_page : %ld\n", internal->left_page_num);
    for (int i = 0; i < internal->num_keys; i++){
      printf("key : %ld\t\t\t page_num : %ld\n", internal->pair[i].key, internal->pair[i].page_num);
    }
  }
  printf("\n");
}

void printing_leaves(int table_id){
  leaf_t leaf;
  pagenum_t page_num;
  header_t head;
  internal_t root;
  head = make_header();
  buffer_read_page(table_id, HEADER_PAGE_NUM, (page_t)head);

  if (head->root_page_num == 0){
    printf("\nTree is Empty\n");
    buffer_write_page(table_id, HEADER_PAGE_NUM, (page_t)head, 0);
    free(head);
    head = NULL;
    return;
  }

  root = make_internal();
  buffer_read_page(table_id, head->root_page_num, (page_t)root);

  if (root->is_leaf){
    leaf = (leaf_t)root;
    page_num = head->root_page_num;
  }
  else {
    leaf = (leaf_t)malloc(sizeof(_leaf_t));
    if (leaf == NULL){
      perror("leaf creation");
      exit(EXIT_FAILURE);
    }
    page_num = root->left_page_num;
    buffer_read_page(table_id, page_num, (page_t)leaf);
    while (!leaf->is_leaf){
      buffer_write_page(table_id, page_num, (page_t)leaf, 0);
      page_num = ((internal_t)leaf)->left_page_num;
      buffer_read_page(table_id, page_num, (page_t)leaf);
    }
  }
  while(leaf->right_leaf_num > 0){
    printf("\n[ page %ld ]\n", page_num);
    printing(table_id, (page_t)leaf);
    buffer_write_page(table_id, page_num, (page_t)leaf, 0);
    page_num = leaf->right_leaf_num;
    buffer_read_page(table_id, leaf->right_leaf_num, (page_t)leaf);
  }
  printf("\n[ page %ld ]\n", page_num);
  printing(table_id, (page_t)leaf);
  buffer_write_page(table_id, page_num, (page_t)leaf, 0);
  if (leaf != (leaf_t)root){
    free(leaf);
    leaf = NULL;
  }
  
  /*buffer_write_page(table_id, head->root_page_num, (page_t)root, 0);*/
  buffer_write_page(table_id, HEADER_PAGE_NUM, (page_t)head, 0);
  printf("\n");
  free(head);
  head = NULL;
  free(root);
  root = NULL;
}

void printing_page(int table_id, pagenum_t page_num){

  header_t head;
  head = make_header();
  buffer_read_page(table_id, HEADER_PAGE_NUM, (page_t)head);
  internal_t root;
  root = make_internal();
  buffer_read_page(table_id, head->root_page_num, (page_t)root);

  if (page_num < 0){
    printf("\nplease input only over 0\n");
  }
  else if (page_num >= head->num_pages){
    printf("\nThere is no [ page %ld ]\n", page_num);
  }
  else if (page_num == 0){
    printf("\n< Header >\n");
    printf("\nnext_free_page : \t%ld\n", head->next_free_page);
    printf("root_page_num : \t%ld\n", head->root_page_num);
    printf("num_pages : \t\t%ld\n", head->num_pages);
  }
  else {
    page_t page = (page_t)malloc(sizeof(_page_t));
    if (page == NULL){
      perror("page creation fail in [ printing_page ] at [ bpt.c ]");
      exit(EXIT_FAILURE);
    }
    buffer_read_page(table_id, page_num, page);
    if (page->num_keys == 0){
      printf("\n< Free page > [ page %ld ]\n", page_num);
      printf("\nnext_free_page : \t%ld\n", ((free_page_t)page)->next_free_page);
    }
    else {
      if (page->is_leaf){
        printf("\n< Leaf > [ page %ld ]\n\n", page_num);
      }
      else {
        printf("\n< Internal > [ page %ld ]\n\n", page_num);
      }
      printing(table_id, page);
    }

    if (head->root_page_num == page_num){
      printf("\n---> It is < Root page >\n");
    }

    printf("\n");
  buffer_write_page(table_id, page_num, (page_t)page, 0);
    free(page);
    page = NULL;
  }
  buffer_write_page(table_id, head->root_page_num, (page_t)root, 0);
  buffer_write_page(table_id, HEADER_PAGE_NUM, (page_t)head, 0);
  free(root);
  root = NULL;
  free(head);
  head = NULL;
}



void check(int table_id){

  header_t head;
  head = make_header();
  buffer_read_page(table_id, HEADER_PAGE_NUM, (page_t)head);

  if (head->root_page_num > head->num_pages){
    perror("head->root_page_num > head->num_pages");
    exit(EXIT_FAILURE);
  }

  int num = 0;
  int check = 0;
  page_t page;
  internal_t internal;
  leaf_t leaf;
  page = (page_t)malloc(sizeof(_page_t));
  for (int i = 1; i < head->num_pages; i++){
    num = 0;
    buffer_read_page(table_id, i, page);
    if (page->is_leaf){
      leaf = (leaf_t)page;
      for (int j = 0; j < MAX_ORDER_IN_LEAF; j++){
        if (leaf->record[j].key > 0){
          num++;
        }
      }
      if (num != leaf->num_keys){
        if (check == 0){
          check = 1;
        }
        printf("< Leaf > [ page %ld ]", (long)i);
        printf("\t\treal num : %d\t\tnum_keys : %d\n", num, leaf->num_keys);
      }
    }
    else {
      internal = (internal_t)page;
      for (int j = 0; j < MAX_ORDER_IN_LEAF; j++){
        if (internal->pair[j].key > 0){
          num++;
        }
      }
      if (num != internal->num_keys){
        if (check == 0){
          check = 1;
        }
        printf("< Internal > [ page %ld ]", (long)i);
        printf("\t\treal num : %d\t\tnum_keys : %d\n", num, leaf->num_keys);
      }
    }
  }
  if (check == 1){
    perror("real num & num_keys mismatch");
    exit(EXIT_FAILURE);
  }
  free(page);
  page = NULL;
  free(head);
  head = NULL;
}

void check_zero(int table_id){
  int i = 0, length = 0;
  pagenum_t arr[200] = {0, };
  free_page_t page;
  int check;
  header_t head;
  head = make_header();
  buffer_read_page(table_id, HEADER_PAGE_NUM, (page_t)head);

  page = (free_page_t)malloc(sizeof(_free_page_t));
  buffer_read_page(table_id, HEADER_PAGE_NUM, (page_t)page);
  while (page->next_free_page != 0){
    arr[i++] = page->next_free_page; 
    length++;
    buffer_read_page(table_id, page->next_free_page, (page_t)page);
    if (((page_t)page)->num_keys != 0){
      perror("free_page_error");
      exit(EXIT_FAILURE);
    }
  }
  for (int j = 1; j < head->num_pages; j++){
    buffer_read_page(table_id, (pagenum_t)j, (page_t)page);
    if (((page_t)page)->num_keys == 0){
      check = 1;
      for (int k = 0; k < length; k++){
        if(arr[k] == j){
          check = 0;
          break;
        }
      }
      if (check == 1) {
        perror("num_keys == 0, but not_free_page");
        exit(EXIT_FAILURE);
      }
    }
  }
}

void check_ascending(int table_id){
  
  leaf_t leaf;
  pagenum_t page_num;
  header_t head;
  internal_t root;

  head = make_header();
  buffer_read_page(table_id, HEADER_PAGE_NUM, (page_t)head);
  root = make_internal();
  buffer_read_page(table_id, head->root_page_num, (page_t)root);

  if (head->root_page_num == 0){
    printf("\nTree is Empty\n");
    return;
  }
  else if (root->is_leaf){
    leaf = (leaf_t)root;
    page_num = head->root_page_num;
  }
  else {
    leaf = (leaf_t)malloc(sizeof(_leaf_t));
    if (leaf == NULL){
      perror("leaf creation IN check_ascending function");
      exit(EXIT_FAILURE);
    }
    page_num = root->left_page_num;
    buffer_read_page(table_id, page_num, (page_t)leaf);
    while (!leaf->is_leaf){
      page_num = ((internal_t)leaf)->left_page_num;
      buffer_read_page(table_id, page_num, (page_t)leaf);
    }
  }

  int64_t key = leaf->record[0].key;

  while (leaf->right_leaf_num != 0){

    for (int i = 0; i < leaf->num_keys; i++){
      if (key > leaf->record[i].key){
        printf("\nAT [ page %ld ]\n", page_num);
        perror("ascending check.");
        exit(EXIT_FAILURE);
      }
      if (i != leaf->num_keys - 1)
        key = leaf->record[i + 1].key;
    }
    page_num = leaf->right_leaf_num;
    buffer_read_page(table_id, page_num, (page_t)leaf);
  }

  free(root);
  root = NULL;
  free(head);
  head = NULL;
}

int static compare (const void* first, const void* second)
{
    if (*(long*)first > *(long*)second)
        return 1;
    else if (*(long*)first < *(long*)second)
        return -1;
    else
        return 0;
}

void sorting(int table_id){
  header_t head;
  head = make_header();
  buffer_read_page(table_id, HEADER_PAGE_NUM, (page_t)head);
  long arr[5000] = {0, };
  int length = 0;
  free_page_t page;
  page = (free_page_t)malloc(sizeof(_free_page_t));
  memcpy(page, head, sizeof(_page_t));
  for (int i = 0; i < head->num_pages; i++){
    arr[i] = page->next_free_page;
    buffer_read_page(table_id, arr[i], (page_t)page);
    buffer_write_page(table_id, arr[i], (page_t)page, 0);
    length++;
    if (page->next_free_page == 0){
      break;
    }
  }
  qsort(arr, length, sizeof(pagenum_t), compare);
  for (int i = 0; i < length - 1; i++){
    printf("%4ld  ",arr[i]);
    if (i != 0 && i % 20 == 0) printf("\n");
    /*
     *if (arr[i] != arr[i + 1] - 1){
     *  printf("%d\n", i);
     *  perror("there is missing free page\n");
     *  exit(EXIT_FAILURE);
     *}
     */
  }
  printf("\n");
  buffer_write_page(table_id, HEADER_PAGE_NUM, (page_t)head, 0);
  free(page);
  page = NULL;
  free(head);
  head = NULL;
}

