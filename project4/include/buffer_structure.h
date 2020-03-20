#ifndef __BUFFER_STRUCTURE_H__
#define __BUFFER_STRUCTURE_H__

#include <stdlib.h> 
#include <stdio.h>
#include "file_manager.h"

/*
 *typedef struct _AVL_Tree* AVL_Tree;
 *typedef int ElementType;
 *typedef struct _AVL_Tree{
 *  ElementType element;
 *  AVL_Tree left;
 *  AVL_Tree right;
 *  int height;
 *}_AVL_Tree;
 */

typedef struct _buffer_t* buffer_t;
typedef struct _buffer_t {
  page_t frame;
  int table_id;
  pagenum_t page_num;
  int is_dirty;
  int is_pinned;
  int referenced;
  int index;
  buffer_t left;
  buffer_t right;
  int height;
}_buffer_t;

typedef struct _hash_t* hash_t;
typedef struct _hash_t {
  buffer_t* table;
  int table_size;
} _hash_t;


hash_t make_hash(int table_size);
int hash_function(hash_t hash, pagenum_t x);
//void delete_hash(hash_t hash);
void swap_page(page_t* x, page_t* y);

buffer_t find_tree(buffer_t root, pagenum_t page_num);
// x 를 t 에 집어 넣는다
buffer_t insert_tree(buffer_t t, buffer_t x);
// x 를 t 에서 제거한다
buffer_t delete_tree(buffer_t t, buffer_t x, int* index);
void file_write_inorder(buffer_t t);
//void deleteTree(buffer_t t);
buffer_t SingleRotateWithLeft(buffer_t node);
buffer_t SingleRotateWithRight(buffer_t node);
buffer_t DoubleRotateWithLeft(buffer_t node);
buffer_t DoubleRotateWithRight(buffer_t node);
int get_balance(buffer_t t);
void changeHeight(buffer_t t);
int max(int x, int y);
void copy_buffer(buffer_t dst, buffer_t src);

#endif
