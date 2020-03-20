#include "transaction.h"
#include "log.h"

int trx_num = 1;

std::unordered_map<int, trx_t> trx_pool;

table_slot_t lock_hash_table[10];

pthread_mutex_t access_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_table_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trx_table_mutex = PTHREAD_MUTEX_INITIALIZER;

int begin_trx(){

  pthread_mutex_lock(&trx_table_mutex);  // lock

  if (trx_num == 1){
    for (int i = 0; i < 10; i++){
      lock_hash_table[i] = NULL;
    }
  }
  
  trx_t trx = new _trx_t();

  if (trx == NULL){
    perror("cannot make trx [ transaction.cpp ] < make_trx >");
    exit(EXIT_FAILURE);
  }

  trx->tid = trx_num++;

  trx_pool.insert(std::pair<int, trx_t>(trx->tid, trx));

  log_buffer_write(BEGIN, trx->tid, -1, 0, 0, NULL, NULL); 

  pthread_mutex_unlock(&trx_table_mutex); // unlock

  trx->wait_lock = NULL;
  trx->cond = PTHREAD_COND_INITIALIZER;

  trx->have_mutex = false;

  return trx->tid;
}

int end_trx(int tid){

  trx_t trx;
  lock_t lock;

  pthread_mutex_lock(&lock_table_mutex);

  log_buffer_write(COMMIT, tid, -1, 0, 0, NULL, NULL);

  std::unordered_map<int, trx_t>::iterator it = trx_pool.find(tid);
  if (it == trx_pool.end()){
    printf("There is no transaction < tid : %d > [ end_trx ] < transaction.cpp >\n", tid);
    return 0;
  }
  trx = it->second;

  for (std::list <lock_t>::iterator lit = trx->holding_locks.begin(); lit != trx->holding_locks.end(); lit = trx->holding_locks.erase(lit)){
    // lit 가 free 되는 것 아닌지 확인해볼것
    lock_t target = (*lit)->record_lock[1];
    int table_id = (*lit)->table_id;
    unlocking(*lit);    
    delete (*lit);
    (*lit) = NULL;
    while (target != NULL) {
      // lock 뒤의 모든 S 를 다 깨워준다
      pthread_cond_signal(&target->owner->cond);

      if (target->record_lock[1] != NULL && target->mode == S && target->record_lock[1]->mode == S){
        target = target->record_lock[1];
      }
      else {
        break;
      }
    }
  }

  pthread_mutex_unlock(&lock_table_mutex);

  pthread_mutex_lock(&trx_table_mutex);


  trx_pool.erase(tid);

  if (trx != NULL){
    /*
     *for (std::list<log_t>::iterator it = trx->log.begin(); it != trx->log.end(); it = trx->log.erase(it)){
     *  free((*it)->old_value);
     *  free((*it)->new_value);
     *  free((*it));
     *}  
     */
    delete (trx);
    trx = NULL;
  }

  pthread_mutex_unlock(&trx_table_mutex);

  //printf("\n(!!!! tid : %d   commit!!!!)\n", tid);

  return tid;
}

// 함수 호출자가 mutex 를 걸어준다
int abort_trx(int tid) {

  trx_t trx;
  lock_t lock;

  //pthread_mutex_lock(&lock_table_mutex);
  std::unordered_map<int, trx_t>::iterator it = trx_pool.find(tid);

  if (it == trx_pool.end()){
    printf("There is no transaction < tid : %d > [ abort ] < transaction.cpp >\n", tid);
    exit(EXIT_FAILURE);
  }
  trx = it->second;

  if (trx->have_mutex == false){
    pthread_mutex_lock(&lock_table_mutex);
  }

  printf("\n(!!!! tid : %d   abort!!!!)\n", tid);

  abort(tid);


  // *************************************************   end_trx   ************************************************************


  for (std::list <lock_t>::iterator lit = trx->holding_locks.begin(); lit != trx->holding_locks.end(); lit = trx->holding_locks.erase(lit)){
    // lit 가 free 되는 것 아닌지 확인해볼것
    lock_t target = (*lit)->record_lock[1];
    int table_id = (*lit)->table_id;
    unlocking(*lit);    
    delete (*lit);
    (*lit) = NULL;
    while (target != NULL) {
      // lock 뒤의 모든 S 를 다 깨워준다
      //pthread_mutex_lock(&target->owner->mutex);
      pthread_cond_signal(&target->owner->cond);
      //pthread_mutex_unlock(&target->owner->mutex);

      if (target->record_lock[1] != NULL && target->mode == S && target->record_lock[1]->mode == S){
        target = target->record_lock[1];
      }
      else {
        break;
      }
    }
  }

  pthread_mutex_unlock(&lock_table_mutex);

  /*
   *for (std::list<log_t>::iterator it = trx->log.begin(); it != trx->log.end(); it = trx->log.erase(it)){
   *  free((*it)->old_value);
   *  free((*it)->new_value);
   *  free((*it));
   *}  
   */







  // *************************************************   end_trx   ************************************************************

/*
 *  end_trx(tid);
 *
 *  pthread_exit((void*)&tid);
 */

  return 0;
}

// transaction_lock 과 record_lock 은 할당하지 않고 NULL 로 초기화한다. 할당은 insert_lock() 에서 한다.
lock_t make_lock(int table_id, int64_t key, lock_mode mode, int tid) {

  //lock_t lock = (lock_t)malloc(sizeof(_lock_t));
  lock_t lock = new _lock_t();
  lock->table_id = table_id;
  lock->key = key;
  lock->mode = mode;
  lock->index = get_index_from_slot(table_id, key, &(lock->slot));
  lock->page_num = lock->slot->page_num;
  
  std::unordered_map<int, trx_t>::iterator it = trx_pool.find(tid);
  if (it == trx_pool.end()){
    printf("There is no transaction < tid : %d > [make_lock] < transaction.cpp >\n", tid);
    exit(EXIT_FAILURE);
  }

  lock->owner = it->second;

  for (int i = 0; i < 2; i++){
    lock->record_lock[i] = NULL;
  }

  lock_t tmp = lock->slot->tail[lock->index];
  if (tmp != NULL) {
    bool is_S = (tmp->mode == S) ? true : false;
    // [ record ] ... <-- ...S...
    if (is_S){
      while (tmp != NULL && tmp->mode == S){
        if (lock->mode == X && lock->owner != tmp->owner){
          lock->wait_lock.push_front(tmp);
        }
        tmp = tmp->record_lock[0];
      }
      if (tmp != NULL && lock->mode == S){
        lock->wait_lock.push_front(tmp);
      }
    }
    // [ record ] ... <-- X(A) 
    else {
      lock->wait_lock.push_front(tmp);
    }
  }

  return lock;
}


// return 이 1이면 cycle 이 있는 것 | return 이 0이면 cycle 이 없다
int cycle_detection(lock_t target_lock, lock_t exist_lock, bool start){

  int ret_val;

  if (start){
    exist_lock = target_lock->slot->tail[target_lock->index];

    // [ record ] <-- nothing
    if (exist_lock == NULL){
      return SUCCESS;
    }

    else if (exist_lock == target_lock->slot->head[target_lock->index] && exist_lock->owner == target_lock->owner){
      exist_lock->owner->holding_locks.remove(exist_lock);
      target_lock->slot->head[target_lock->index] = NULL;  
      target_lock->slot->tail[target_lock->index] = NULL;  
      delete exist_lock;
      exist_lock = NULL;
      return SUCCESS;
    }
    

    while (exist_lock != NULL && exist_lock->mode == S){
      if (exist_lock->owner == target_lock->owner){
        // [ record ] ... <-- S(A) <-- ...S... <-- X(A)
        if (target_lock->mode == X){
          //return DEADLOCK;
        }
        // [ record ] ... <-- S(A) <-- ...S... <-- S(A)
        else {
          return NO_APPEND;
        }
      }
      exist_lock = exist_lock->record_lock[0];
    }
    // [ record ] ...S... <-- S(A)
    if (exist_lock == NULL && target_lock->mode == S){
      return SUCCESS;
    }

    // [ record ] ... <-- X(A) <-- ...S... <-- X(A)/S(A)
    else if (exist_lock != NULL && exist_lock->owner == target_lock->owner){
      return NO_APPEND; 
    }
    // [ record ]...S... <-- X(A)
    else if (exist_lock == NULL){
      exist_lock = target_lock->slot->tail[target_lock->index];
    }

    // [ record ] ... <-- X(A) <-- ...S... <-- X(B)
    else if (exist_lock != NULL && exist_lock->owner != target_lock->owner){
    }

    // 오류 -------------------------------------------------------------------------
    else {
      lock_t tmp = target_lock->slot->tail[target_lock->index];
      while(tmp->record_lock[1] != NULL){
        printf("%c(%d)  <--  ", (tmp->mode == S) ? 'S' : 'X', tmp->owner->tid);
        tmp = tmp->record_lock[1];
      }
      printf("%c(%d)\n", (tmp->mode == S) ? 'S' : 'X', tmp->owner->tid);
      exit(EXIT_FAILURE);
    }
    // 오류 -------------------------------------------------------------------------

    start = false;

    for (std::list<lock_t>::iterator it = target_lock->wait_lock.begin(); it != target_lock->wait_lock.end(); it++){
      ret_val = cycle_detection(target_lock, (*it), start);
      if (ret_val == DEADLOCK){
        return DEADLOCK;
      }
    }
    return CONFLICT;
  }

  else {
    if (exist_lock->wait_lock.empty()){
      std::list<lock_t>::iterator it = std::find(exist_lock->owner->holding_locks.begin(), exist_lock->owner->holding_locks.end(), exist_lock);
      if ((++it) != exist_lock->owner->holding_locks.end()){
        ret_val = cycle_detection(target_lock, (*it), start);
        if (ret_val == DEADLOCK){
          return DEADLOCK;
        }
      }
    }
    else {
      for (std::list<lock_t>::iterator it = exist_lock->wait_lock.begin(); it != exist_lock->wait_lock.end(); it++){
        if (target_lock->owner == (*it)->owner){
          return DEADLOCK;
        }
        ret_val = cycle_detection(target_lock, (*it), start);
        if (ret_val == DEADLOCK){
          return DEADLOCK;
        }
      }
    }
    return CONFLICT;
  }
}

// sleep 해야하면 true, sleep 안해도 되면 false
void insert_lock(lock_t lock, table_slot_t slot, int index){
  lock_t tmp_lock;
  // 제일 처음 insert 하는 경우
  if (slot->head[index] == NULL){
    slot->head[index] = lock;
  }
  // lock 의 앞을 기존 마지막으로 설정
  lock->record_lock[0] = slot->tail[index];
  
  // 기존 마지막의 뒤를 새로 insert 하려는 lock 으로 설정
  if (slot->tail[index] != NULL){
    slot->tail[index]->record_lock[1] = lock;
  }
  // 가장 마지막을 lock 으로 설정
  slot->tail[index] = lock;

  // transaction_lock 할당
  (lock->owner->holding_locks).push_back(lock);
  
}


/*
 *void recover(trx_t trx){
 *  
 *  record_t record;
 *  leaf_t leaf;
 *  std::list <log_t>::iterator it;
 *  table_slot_t slot = NULL;
 *  int index;
 *
 *  for (it = (trx->log).begin(); it != (trx->log).end(); it++){
 *    while ((leaf = (leaf_t)buffer_read_page((*it)->table_id, (*it)->page_num)) == NULL);
 *    index = search_record_index(leaf->record, 0, leaf->num_keys - 1, (*it)->key, 0);
 *    if (index == leaf->num_keys){
 *      printf("[ transaction.cpp ] < recover >\n");
 *      exit(EXIT_FAILURE);
 *    }
 *    record = &leaf->record[index];
 *    if (record != NULL){
 *
 *      printf("\n(!!! < tid : %d > recover!!!)  [ table %d ]'s [ page %ld ]\t\t key : %ld\t\t < value :    %s", trx->tid, (*it)->table_id, (*it)->page_num, record->key, record->value);
 *
 *      strcpy(record->value, (*it)->old_value);
 *
 *      printf("    ------>    %s     >\n", record->value);
 *
 *      buffer_write_page((*it)->table_id, (*it)->page_num, (page_t)leaf, 1);
 *    }
 *    else {
 *      printf("There is no [ table %d ], < key : %ld >\n", (*it)->table_id, (*it)->key);
 *      exit(EXIT_FAILURE);
 *    }
 *  }
 *}
 *
 *
 */

int get_index_from_slot(int table_id, int64_t key, table_slot_t* slot){

  int index = 0;

  *slot = get_slot(&lock_hash_table[table_id - 1], table_id, key);

  index = search_record_index((*slot)->key, 0, (*slot)->num_keys - 1, key, 0);
  if (index == (*slot)->num_keys){
    printf("There is no record [ table %d ] < key %ld > || [ transaction.cpp ] < get_record_from_slot >\n", table_id, key);
    return index;
  }
  else {
    return index;
  }
}

table_slot_t get_slot(table_slot_t* slot, int table_id, int64_t key){
  
  if (*slot == NULL){
    *slot = make_slot(table_id, key);
  }
  else if (key < (*slot)->key[0]){
    (*slot)->left = get_slot(&(*slot)->left, table_id, key);
  }
  else if (key > (*slot)->key[(*slot)->num_keys - 1]){
    (*slot)->right = get_slot(&(*slot)->right, table_id, key);
  }

  while (true){
    if (key < (*slot)->key[0] && (*slot)->left != NULL){
      (*slot) = (*slot)->left;
    }
    else if (key > (*slot)->key[(*slot)->num_keys - 1] && (*slot)->right != NULL){
      (*slot) = (*slot)->right;
    }
    else {
      break;
    }
  }

  return *slot;
}

table_slot_t make_slot(int table_id, int64_t key){

  leaf_t leaf;
  table_slot_t slot = (table_slot_t)malloc(sizeof(_table_slot_t));
  leaf = find_leaf(table_id, key, &(slot->page_num));
  slot->table_id = table_id;
  slot->head = (lock_t*)malloc(sizeof(lock_t) * 31);
  slot->tail = (lock_t*)malloc(sizeof(lock_t) * 31);
  slot->key = (int64_t*)malloc(sizeof(int64_t) * 31);
  if (slot->head == NULL || slot->tail == NULL || slot->key == NULL){
    printf("malloc error [ transaction.cpp ] < make_slot >\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < 31; i++){
    slot->head[i] = NULL;
    slot->tail[i] = NULL;
    slot->key[i] = leaf->record[i].key;
  }
  slot->num_keys = leaf->num_keys;
  buffer_write_page(table_id, slot->page_num, (page_t)leaf, 0);
  slot->left = NULL;
  slot->right = NULL;
  return slot;
}

// slot 의 head, tail 바꿔주기 | 뒤쪽 lock 의 wait_lock 에서 자기자신 제거 
void unlocking(lock_t lock){

  // 한 record 에 lock 이 하나밖에 없었던 경우
  if (lock->slot->head[lock->index] == lock->slot->tail[lock->index]){
    lock->slot->tail[lock->index] = NULL;
    lock->slot->head[lock->index] = NULL;
    return;
  }

  // 두 개 이상의 lock 이 있었을 경우
  else {
    lock_t tmp = lock->record_lock[1];
    // X(A) <-- ...
    if (lock->mode == X){
      // X(A) <-- X(B)
      if (tmp->mode == X){
        tmp->wait_lock.remove(lock);
      } 
      // X(A) <-- S(B) <-- S(C) <-- ...
      else {
        while(tmp!= NULL && tmp->mode == S){
          tmp->wait_lock.remove(lock);
          tmp = tmp->record_lock[1];
        }
      }
    }
    // S(A) <-- ...
    else {
      while (tmp != NULL && tmp->mode == S) {
        tmp = tmp->record_lock[1];
      }
      // S(A) <-- ... <-- S(B) <-- X(C) 에서 X(C) 의 wait_lock 에서 A 제외
      if (tmp != NULL){
        tmp->wait_lock.remove(lock);  
      }
    }

    if (lock->record_lock[0] != NULL){
      lock->record_lock[0]->record_lock[1] = lock->record_lock[1];
    }
    if (lock->record_lock[1] != NULL){
      lock->record_lock[1]->record_lock[0] = lock->record_lock[0];
    }
    if (lock->record_lock[0] != NULL){
      lock->slot->tail[lock->index] = lock->record_lock[0];
      lock->slot->head[lock->index] = lock->record_lock[0];
      while (lock->slot->head[lock->index]->record_lock[0] != NULL){
        lock->slot->head[lock->index] = lock->slot->head[lock->index]->record_lock[0];
      }
      while (lock->slot->tail[lock->index]->record_lock[1] != NULL){
        lock->slot->tail[lock->index] = lock->slot->tail[lock->index]->record_lock[1];
      }
    } 
    else if (lock->record_lock[1] != NULL){
      lock->slot->tail[lock->index] = lock->record_lock[1];
      lock->slot->head[lock->index] = lock->record_lock[1];
      while (lock->slot->head[lock->index]->record_lock[0] != NULL){
        lock->slot->head[lock->index] = lock->slot->head[lock->index]->record_lock[0];
      }
      while (lock->slot->tail[lock->index]->record_lock[1] != NULL){
        lock->slot->tail[lock->index] = lock->slot->tail[lock->index]->record_lock[1];
      }
    } 
  }
}

int is_unlock_record(lock_t lock){
  if (lock->mode == X){
    if (lock->record_lock[0] == NULL){
      return SUCCESS;
    }
    // [ record ] S(A) <-- X(A)
    else if (lock->record_lock[0]->owner == lock->owner && lock->record_lock[0]->record_lock[0] == NULL){
      lock->owner->holding_locks.remove(lock->record_lock[0]);
      lock->slot->head[lock->index] = lock;
      delete lock->record_lock[0];
      lock->record_lock[0] = NULL;
      return SUCCESS;
    }
    else {
      return CONFLICT;
    }
  }
  else {
    while (lock != NULL && lock->mode != X) {
      lock = lock->record_lock[0]; 
    }
    if (lock == NULL){
      return SUCCESS;
    }
    else {
      return CONFLICT;
    }
  }
}

int acquire_record_lock(int table_id, int64_t key, lock_mode mode, int tid, bool is_first){

  int index, result;
  table_slot_t slot;
  lock_t lock;
  trx_t trx;
  
  pthread_mutex_lock(&lock_table_mutex);

  std::unordered_map<int, trx_t>::iterator it = trx_pool.find(tid);
  trx = it->second;

  trx->have_mutex = true;

  index = get_index_from_slot(table_id, key, &slot); 

  if (index == slot->num_keys){
    pthread_mutex_unlock(&lock_table_mutex);
    trx->have_mutex = false;
    return NO_RECORD;
  }

  if (is_first){
    lock = make_lock(table_id, key, mode, tid); 
    result = cycle_detection(lock, NULL, true);
  }
  else {
    lock = trx->wait_lock;
    result = is_unlock_record(lock);
  }

  if (result == SUCCESS || result == CONFLICT) {
    if (is_first){
      insert_lock(lock, slot, lock->index);
    }
    if (result == CONFLICT){
      trx->wait_lock = lock;
    }
    else {
      trx->wait_lock = NULL; 
    }
  }
  else if (result == NO_APPEND || result == DEADLOCK){
    delete lock;
    lock = NULL;
  }
  
  //pthread_mutex_unlock(&lock_table_mutex);
  return result;
}

table_slot_t find_min_node(table_slot_t slot){
  if (slot->left != NULL){
    return slot->left;
  }
  else {
    return slot;
  }
}


void copy_node(table_slot_t to, table_slot_t from){
  to->key = from->key;
  to->num_keys = from->num_keys;
  to->page_num = from->page_num;
  to->head = from->head;
  to->tail = from->tail;
}

/*
 *table_slot_t delete_slot(table_slot_t slot, int64_t key){
 *
 *  table_slot_t tmp;
 *
 *  if (slot == NULL){
 *    printf("[ transaction.cpp ] < delte_slot >\n");
 *    exit(EXIT_FAILURE);
 *  }
 *  else if (key < slot->key[0]){
 *    slot->left = delete_slot(slot->left, key);
 *  }
 *  else if (key > slot->key[0]){
 *    slot->right = delete_slot(slot->right, key);
 *  }
 *  else if (slot->left != NULL && slot->right != NULL){
 *    tmp = find_min_node(slot->right);
 *    copy_node(slot, tmp);
 *    slot->right = delete_slot(slot->right, slot->key[0]); 
 *  }
 *  else {
 *    tmp = slot;
 *    if (slot->left == NULL){
 *      slot = slot->right;
 *    }
 *    else {
 *      slot = slot->left;
 *    }
 *    free(slot);
 *    slot = NULL;
 *  }
 *
 *  return slot;
 *}
 */

void delete_slot(){
  for (int i = 0; i < 10; i++){
    destroy_slot(lock_hash_table[i]);
  }
}

void destroy_slot(table_slot_t slot){
  if (slot == NULL){
    return;
  }
  destroy_slot(slot->left);
  destroy_slot(slot->right);
  free(slot->key);
  delete (slot->head);
  delete (slot->tail);
  free(slot);
}

void unlock_mutex(int tid){
  std::unordered_map<int, trx_t>::iterator it = trx_pool.find(tid);

  it->second->have_mutex = false;
  pthread_mutex_unlock(&lock_table_mutex);
}

void sleep_trx(int tid){
  
  std::unordered_map<int, trx_t>::iterator it = trx_pool.find(tid);

  if (it != trx_pool.end()){
    //pthread_mutex_lock(&(it->second->mutex));
    printf("\n< tid : %d > sleep\n", tid);
    pthread_cond_wait(&(it->second->cond), &lock_table_mutex);
    //pthread_mutex_unlock(&(it->second->mutex));
    pthread_mutex_unlock(&lock_table_mutex);
    it->second->have_mutex = false;
  }
  else {
    printf("There is no %d [ transaction.cpp ] < sleep_trx >\n", tid);
    exit(EXIT_FAILURE);
  }

}

/*
 *void write_log(int table_id, pagenum_t page_num, int64_t key, char* old_value, char* values, int tid) {
 *  
 *  std::unordered_map<int, trx_t>::iterator it = trx_pool.find(tid);
 *  log_t log = NULL;
 *
 *  if (it != trx_pool.end()){
 *    log = (log_t)malloc(sizeof(_log_t));
 *    if (log == NULL){
 *      printf("malloc error [ transaction.cpp ] < write_log >\n");
 *    }
 *    log->old_value = (char*)malloc(sizeof(char) * VALUE_SIZE);
 *    log->new_value = (char*)malloc(sizeof(char) * VALUE_SIZE);
 *    log->table_id = table_id;
 *    log->key = key;
 *    log->page_num = page_num;
 *    strcpy(log->old_value, old_value);
 *    strcpy(log->new_value, values);
 *    it->second->log.push_front(log);
 *  }
 *  else {
 *    printf("There is no %d [ transaction.cpp ] < write_log >\n", tid);
 *    exit(EXIT_FAILURE);
 *  }
 *}
 */
