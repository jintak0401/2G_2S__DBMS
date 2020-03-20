#ifndef __BPT_H__
#define __BPT_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


// leaf_page 의 value 크기
#define VALUE_SIZE 120

// leaf_page 가 가질 수 있는 record 개수
#define MAX_ORDER_IN_LEAF 31

// internal_page 가 가질 수 있는 key 개수
#define MAX_ORDER_IN_INTERNAL 248

// 한 페이지의 크기
#define PAGE_SIZE 4096

// 파일에서 header_page 의 위치 
#define HEADER_PAGE_NUM 0

extern int use_free_page;

typedef int64_t pagenum_t;

// leaf page 에 들어갈 record
typedef struct _record_t* record_t;
typedef struct _record_t {
    int64_t key;
    char value[VALUE_SIZE];
} _record_t;

// internal page 에 들어갈 < key, page_num > 쌍
typedef struct _pair_t* pair_t;
typedef struct _pair_t{
  int64_t key;
  pagenum_t page_num;
} _pair_t;

// 0 ~ 4095 영역에 위치한 헤더 페이지
typedef struct _header_t* header_t;
typedef struct _header_t{
  pagenum_t next_free_page;  //  free page 가 있다면 해당 page_num, 없다면 0
  pagenum_t root_page_num;  //  root page 의 page_num
  int64_t num_pages;  // 파일 내의 페이지 개수
  char reserved[PAGE_SIZE - 24];  // 24 ~ 4095 영역 : 다음 과제를 위해 4072 byte 잡아놓음
} _header_t;

// free_page 형식
typedef struct _free_page_t* free_page_t;
typedef struct _free_page_t {
  pagenum_t next_free_page;
  char not_used[PAGE_SIZE - 8]; // next_free_page 를 제외한 나머지 영역 : 8 ~ 4095
} _free_page_t;


// internal 과 leaf 를 모두 아우를 수 있는 구조체 
typedef struct _page_t* page_t; 
typedef struct _page_t {
    pagenum_t parent_page_num;  // 
    int is_leaf;  // leaf page 이면 1, 아니면 0
    int num_keys;

    char reserved_8[8];

    long page_LSN;

    char reserved[88];  // 16 ~ 120 영역 : 다음 과제를 위해 104 byte 잡아 놓음

    pagenum_t other_page_num;  // internal page 일 경우 = 가장 오른쪽 leaf page | leaf page 일 경우 = 오른쪽 leaf page
    char child[PAGE_SIZE - 128];  // 128 ~ 4095 영역 : internal page 일 경우 = pair | leaf page 일 경우 = record
} _page_t;

// leaf_page
typedef struct _leaf_t* leaf_t;
typedef struct _leaf_t {
    pagenum_t parent_page_num;
    int is_leaf;
    int num_keys;
    char reserved_8[8];

    long page_LSN;

    char reserved[88];  // 16 ~ 120 영역 : 다음 과제를 위해 104 byte 잡아 놓음
    pagenum_t right_leaf_num;  
    _record_t record[MAX_ORDER_IN_LEAF];  // MAX_ORDER_IN_LEAF = 31
} _leaf_t;

// internal 구조체
typedef struct _internal_t* internal_t;
typedef struct _internal_t{
    pagenum_t parent_page_num;
    int is_leaf;
    int num_keys;
    char reserved_8[8];

    long page_LSN;

    char reserved[88];  // 16 ~ 120 영역 : 다음 과제를 위해 104 byte 잡아 놓음
    pagenum_t left_page_num;  
    _pair_t pair[MAX_ORDER_IN_INTERNAL];  // MAX_ORDER_IN_INTERNAL = 248
} _internal_t;


// free page 리스트로부터 페이지 할당하여 반환

void usage( void );


// root 를 생성하는 함수
void make_root(page_t page);

// find 가장 상위 함수. find 를 호출하여 있으면 0, 없으면 -1 반환
int db_find(int table_id, int64_t key, char* ret_val, int trx_id); 
int db_update(int table_id, int64_t key, char* values, int trx_id);

// find 가장 하위 함수, find 에게 호출되며 해당 key 가 있을 '수' 있는 leaf_page 반환 & 해당 page_num 저장
leaf_t find_leaf(int table_id, int64_t key, pagenum_t* page_num);

// db_find 에게 호출되며, find_leaf 를 호출하는 함수. find_leaf 로 얻은 leaf 에 key 가 있으면 해당 record 반환 & 해당 page_num 저장
record_t find(int table_id, int64_t key, pagenum_t* page_num, leaf_t* leaf);

int cut( int length );

int search_page_position(pair_t pair, int start, int end, int64_t key);
int search_record_index(record_t record, int start, int end, int64_t key, int for_input);
int search_record_index(int64_t* record, int start, int end, int64_t key, int for_input);


 /*
  *Insertion.
  */


record_t make_record(int64_t key, char* value);
pair_t make_pair(int64_t key, pagenum_t page_num);

// 인자로 받은 left_page_num 이 존재하는 parent->pair 의 인덱스 반환 | 만약 parent->left_page_num 이라면 -1 반환
int get_left_index(internal_t parent, pagenum_t left_page_num);

// split 없이 leaf 에 record 추가 && 결과 file 에 저장
void insert_into_leaf(leaf_t leaf, pagenum_t page_num, record_t record);

// leaf 를 split 시키면서 record 집어 넣기 | 이 함수 내에서 file 에 반영한다 
void insert_into_leaf_after_splitting(int table_id, leaf_t old_leaf, pagenum_t old_leaf_page_num, record_t record);

// split 없이 internal 에 pair 추가 && 결과 file 에 저장
void insert_into_internal(internal_t n, pagenum_t n_page_num, int left_index, pair_t right);

// internal_page 를 split 시키면서 pair 집어 넣기 | 이 함수 내에서 file 에 반영한다 
void insert_into_internal_after_splitting(int table_id, internal_t old_internal, pagenum_t old_internal_page_num,
    int left_index, pair_t pair, page_t left, page_t right, pagenum_t left_page_num, pagenum_t right_page_num);

// ***_after_splitting 함수들에게 불리는 함수로 split 결과를 parent 에 반영하는 함수
void insert_into_parent(int table_id, page_t left, pagenum_t left_page_num, pair_t pair, page_t right, pagenum_t right_page_num);

// 기존 root 가 split 됐을 경우 새로운 root 를 생성하는 함수
void insert_into_new_root(int table_id, page_t left, pagenum_t left_page_num, page_t right, pair_t right_pair);

// root == NULL 일 때 실행한다
void start_new_tree(int table_id, record_t record);

// insertion 최상위 함수, 상황에 따라 start_new_tree, insert_into_new_root, insert_into_leaf, insert_into_leaf_after_splitting 를 호출한다 
int db_insert(int table_id, int64_t key, char* value );

// 인자로 받는 parent 의 모든 자식의 parent_page_num 을 page_num 으로 바꾸어서 파일에 저장한다
void change_parent_of_child(int table_id, internal_t parent, pagenum_t page_num/*, pagenum_t left, pagenum_t right*/);


 /*
  *Deletion.
  */



// page 의 왼쪽 page 인덱스 반환, 가장 왼쪽일 경우 그 오른쪽 page 인덱스 반환 | parent & neighbor_page_num 을 할당한다
int get_neighbor_index(page_t page, pagenum_t* page_num, internal_t parent, pagenum_t* neighbor_page_num );

// 삭제가 root 에서 일어났을 경우 | 인자로 받는 page 는 root 에서 삭제되어야 할 것이 삭제된 page 이다. root 를 재 할당한다 
int adjust_root(int table_id, page_t page, page_t most_left_page);

// page 의 가 neighbor 과 통합한다. page 가 leaf 였을 경우 record 가 모두 사라졌을 때 통합한다.
void coalesce_page(int table_id, page_t page, page_t neighbor, internal_t parent, int neighbor_index, int64_t k_prime,
    pagenum_t* page_num, pagenum_t* neighbor_page_num, pagenum_t* parent_page_num, page_t left_child, page_t right_child);

// 인자로 받는 page 에서 key 에 해당하는 pair 혹은 record 를 제거한다
void delete_entry(int table_id, page_t page, pagenum_t* page_num, int64_t key , page_t most_left_page, page_t right_child);

// deletion 최상위 함수로 인자로 받는 key 에 해당하는 record 가 있으면 delete_entry 를 호출한다
int db_delete(int table_id, int64_t key );


/*
 *Join
 */

int join_table(int table_id_1, int table_id_2, char* pathname);


// 디버깅용 임시 함수들
void check(int table_id);
void check_zero(int table_id);
void check_ascending(int table_id);
void sorting(int table_id);

// 출력용 함수들
void printing(int table_id, page_t page);
void printing_page(int table_id, pagenum_t page_num);
void printing_leaves(int table_id);

#endif /* __BPT_H__*/
