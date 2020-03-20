#include "buffer_structure.h"

int hash_function(hash_t hash, pagenum_t x){
  return x % hash->table_size;
}

hash_t make_hash(int table_size){
  // hash 생성
 hash_t hash = (hash_t)malloc(sizeof(_hash_t));
 if (hash == NULL){
   perror("hash creation failure [ buffer_structure.c ], < make_hash >");
   exit(EXIT_FAILURE);
 }

 // hash 테이블 크기와 테이블 할당 
 hash->table_size = table_size;
 hash->table = (buffer_t*)malloc(sizeof(buffer_t) * (hash->table_size));
 if (hash->table == NULL){
   perror("hash->size/table creation fail [ buffer_structure.c ], < make_hash >");
   exit(EXIT_FAILURE);
 }

 // 테이블의 각 칸을 NULL 로 초기화
 for (int i = 0; i < hash->table_size; i++) {
   hash->table[i] = NULL;
 }
 return hash;
}

void swap_page(page_t* x, page_t* y){
  page_t tmp = *x;
  *x = *y;
  *y = tmp;
}

buffer_t find_tree(buffer_t root, pagenum_t page_num){
  if (root == NULL){
    return NULL;
  }
  else if (root->page_num < page_num){
    return find_tree(root->right, page_num);
  }
  else if (root->page_num > page_num){
    return find_tree(root->left, page_num);
  }
  else {
    return root;
  }
}

buffer_t insert_tree(buffer_t t, buffer_t x){
	if (t == NULL){
    t = x;
		t->left = NULL;
		t->right = NULL;
		t->height = 0;
	}	
	else {

    if (x->page_num < t->page_num){
      t->left = insert_tree(t->left, x);
    }
    else if (x->page_num > t->page_num){
      t->right = insert_tree(t->right, x);
    }
    else {
      return t;
    }

    changeHeight(t);

    int balance = get_balance(t);

    if (balance > 1 && x->page_num < t->left->page_num){
      return SingleRotateWithLeft(t);
    }
    else if (balance > 1 && x->page_num > t->left->page_num){
      return DoubleRotateWithLeft(t);
    }
    else if (balance < -1 && x->page_num > t->right->page_num){
      return SingleRotateWithRight(t);
    }
    else if (balance < -1 && x->page_num < t->right->page_num){
      return DoubleRotateWithRight(t);
    }

    return t;

  }
}

buffer_t find_min(buffer_t t){
  if (t == NULL){
    return t;
  }
  while (t->left != NULL){
    t = t->left; 
  }
  return t;
}

buffer_t delete_tree(buffer_t t, buffer_t x, int* index){

  buffer_t right_min = NULL;
  int balance;

  if (t == NULL){
    /*printf("There is no %d\n", x);*/
    printf("\nt->table_id : %d\tt->page_num : %ld\n", t->table_id, t->page_num);
    printf("\nx->table_id : %d\tx->page_num : %ld\n\n", x->table_id, x->page_num);
    perror("find error at [ buffer_structure.c ], < delete >");
    exit(EXIT_FAILURE);
  }
  else if (x->page_num > t->page_num){
    t->right = delete_tree(t->right, x, index);
  }
  else if (x->page_num < t->page_num) {
    t->left = delete_tree(t->left, x, index);
  }
  else {

    if (t->left == NULL || t->right == NULL){

      t->table_id = 0;
      t->is_dirty = 0;
      t->page_num = 0;
      t->is_pinned = 0;
      t->referenced = 0;
      *index = t->index;

      if (t->left == NULL) {
        t = t->right;
      }
      else {
        t = t->left;
      }
    }
    else {
      right_min = find_min(t->right); 
      // t 에 right_min 의 정보 복사
      copy_buffer(t, right_min);
      t->right = delete_tree(t->right, t, index);
    }
  }

  if (t == NULL){
    return t;
  }

  changeHeight(t);
  
  balance = get_balance(t);

  if (balance > 1 && get_balance(t->left) >= 0){
    return SingleRotateWithLeft(t);
  }
  else if (balance > 1 && get_balance(t->left) < 0){
    return DoubleRotateWithLeft(t);
  }
  else if (balance < -1 && get_balance(t->right) <= 0){
    return SingleRotateWithRight(t);
  }
  else if (balance < -1 && get_balance(t->right) > 0){
    return DoubleRotateWithRight(t);
  }

  return t;
}

void copy_buffer(buffer_t dst, buffer_t src){
  swap_page(&dst->frame, &src->frame);
  dst->page_num = src->page_num;
  dst->is_dirty = src->is_dirty;
  dst->is_pinned = src->is_pinned;
  dst->referenced = src->referenced;
}

void file_write_inorder(buffer_t t){
  if (t == NULL){
    return;
  }
  file_write_inorder(t->left);
  if (t->is_dirty){
    file_write_page(t->table_id, t->page_num, t->frame);
  }
  t->table_id = 0;
  t->page_num = 0;
  t->is_dirty = 0;
  t->referenced = 0;
  t->is_pinned = 0;
  file_write_inorder(t->right);
}

// 왼쪽 자식이 부모가 됨
buffer_t SingleRotateWithLeft(buffer_t node){
	buffer_t tmp = node->left;
	node->left = tmp->right;
	tmp->right = node;

	changeHeight(node);
	changeHeight(tmp);

	return tmp;
}

// 오른쪽 자식이 부모가 됨
buffer_t SingleRotateWithRight(buffer_t node){
	buffer_t tmp = node->right;
	node->right = tmp->left;
	tmp->left = node;

	changeHeight(node);
	changeHeight(tmp);

	return tmp;
}

buffer_t DoubleRotateWithLeft(buffer_t node){
	node->left = SingleRotateWithRight(node->left);
	return SingleRotateWithLeft(node);
}

buffer_t DoubleRotateWithRight(buffer_t node){
	node->right = SingleRotateWithLeft(node->right);
	return SingleRotateWithRight(node);
}


int get_balance(buffer_t t){
  if (t == NULL){
    return 0;
  }
  int leftHeight = (t->left == NULL) ? -1 : t->left->height;
  int rightHeight = (t->right == NULL) ? -1 : t->right->height;
  return leftHeight - rightHeight;
}

void changeHeight(buffer_t t){
		int leftHeight = (t->left == NULL) ? -1 : t->left->height;
		int rightHeight = (t->right == NULL) ? -1 : t->right->height;
		t->height = max(leftHeight, rightHeight) + 1;
}

int max(int x, int y){
	return (x > y) ? x : y;
}

