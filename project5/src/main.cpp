#include "bpt.h"
#include "file_manager.h"
#include "buffer.h"
#include <time.h>
#include "transaction.h"
#include <pthread.h>
#include <sys/resource.h>

// MAIN

void* thread1(void* args){
  char ret_val[500];
  int tid = begin_trx();
  db_find(1, 500, ret_val, tid);
  db_find(1, 497, ret_val, tid);
  sleep(2);
  db_find(1, 499, ret_val, tid);
  sleep(4);
  db_update(1, 500, (char*)"A-->500", tid);
  sleep(1);
  db_update(1, 500, (char*)"A-->500", tid);
  end_trx(tid);
  return NULL;
}

void* thread2(void* args){
  char ret_val[500];
  sleep(1);
  int tid = begin_trx();
  db_update(1, 499, (char*)"B-->499", tid);

  sleep(4);

  db_update(1, 498, (char*)"B-->498", tid);

  sleep(1);
  db_update(1, 500, (char*)"B-->500", tid);
  sleep(2);
  db_find(1, 500, ret_val, tid);
  end_trx(tid);
  return NULL;
}

void* thread3(void* args) {
  char ret_val[500];
  sleep(3);
  int tid = begin_trx();
  db_find(1, 497, ret_val, tid);
  db_find(1, 498, ret_val, tid);
  db_update(1, 500, (char*)"C-->500", tid);
  end_trx(tid);
  return NULL;
}

void* thread4(void* args) {
  char ret_val[500];
  sleep(4);
  int tid = begin_trx();
  db_update(1, 499, (char*)"D-->499", tid);
  end_trx(tid);
  return NULL;
}

/*
 *void* thread1(void* args){
 *  char ret_val[500];
 *  int tid = begin_trx();
 *  db_update(1, 500, (char*)"(1)A-->500", tid);
 *  //db_find(1, 497, ret_val, tid);
 *  sleep(2);
 *  db_update(1, 500, (char*)"(2)A-->500", tid);
 *  db_update(1, 500, (char*)"(3)A-->500", tid);
 *  end_trx(tid);
 *  return NULL;
 *}
 *
 *void* thread2(void* args){
 *  char ret_val[500];
 *  sleep(1);
 *  int tid = begin_trx();
 *  db_update(1, 500, (char*)"(1)B-->500", tid);
 *
 *  //sleep(4);
 *
 *  //db_update(1, 498, (char*)"B-->498", tid);
 *  end_trx(tid);
 *  return NULL;
 *}
 */


int main( int argc, char ** argv ) {

  /*
    char input_file[100];
    FILE * fp;
    int64_t key;
    int range2;
    char instruction;
    char license_part;
    char* value;
    int table_id_1, table_id_2;
    int buf_num;

    clock_t start, end;
    float delay;

    open_table((char*)"1");
    open_table((char*)"2");
    open_table((char*)"3");
    open_table((char*)"4");
    open_table((char*)"5");
    open_table((char*)"6");
    open_table((char*)"7");
    open_table((char*)"8");
    open_table((char*)"9");
    open_table((char*)"10");



    value = (char*)malloc(sizeof(char) * VALUE_SIZE); 
    printf("\nHow many buffers do you need?\n");
    scanf("%d", &buf_num);
    while(getchar() != '\n');
    init_db(buf_num);

    usage();
    printf("\n=================================================\n");

    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'b':
            printing_buffer();
            printing_head();
            break;
        case 's':
            scanf("%d", &table_id_1);
            sorting(table_id_1);
            break;
        case 'd':
            scanf("%d %ld", &table_id_1, &key);
            db_delete(table_id_1, key);
            break;
        case 'i':
            scanf("%d %ld %s", &table_id_1, &key, value);
            db_insert(table_id_1, key, value);
            break;
        case 'f':
            scanf("%d %ld", &table_id_1, &key);
            db_find(table_id_1, key, value);
            break;
        case 'p':
            scanf("%d %ld", &table_id_1, &key);
            printing_page(table_id_1, key);
            break;
        case 'l':
            scanf("%d", &table_id_1);
            printing_leaves(table_id_1);
            break;
        case 'q':
            free(value);
            shutdown_db();
            printf("\n");
            free(value);
            return EXIT_SUCCESS;
            break;
        case 'o' :
            scanf("%s", input_file);
            open_table(input_file);            
            break;
        case 'c' :
            scanf("%d", &table_id_1);
            close_table(table_id_1);            
            break;
        case 'j' :
            printf("output file name >> ");
            scanf("%s", input_file);
            printf("tables >> ");
            scanf("%d %d", &table_id_1, &table_id_2);
            printf("\n\n");
            join_table(table_id_1, table_id_2, input_file);
            file_read_for_join(input_file);
            break;
        case 'z' :
            start = clock();
            break;
        case 'y' :
            end = clock();
            printf("\n==================================\n");
            printf("delay : %lf\n", (float)(end - start)/CLOCKS_PER_SEC);
            printf("==================================\n");
            break;
        case '?' : 
            usage();
            break;
        default:
            break;
        }
    }
    printf("\n");
    free(value);


    return EXIT_SUCCESS;
    */

    /*
     *  struct rlimit rlim;
     *getrlimit(RLIMIT_STACK, &rlim);
     *rlim.rlim_cur = (1024 * 1024 * 32);
     *rlim.rlim_max = (1024 * 1024 * 32);
     *setrlimit(RLIMIT_STACK, &rlim);
     */

      
    open_table((char*)"1");
    init_db(10);

   pthread_t thread[4]; 
  long ret_val;

  pthread_create(&thread[0], NULL, thread1, NULL);
  pthread_create(&thread[1], NULL, thread2, NULL);
  pthread_create(&thread[2], NULL, thread3, NULL);
  pthread_create(&thread[3], NULL, thread4, NULL);

   pthread_join(thread[0], (void**)&ret_val);
   pthread_join(thread[1], (void**)&ret_val);
   pthread_join(thread[2], (void**)&ret_val);
   pthread_join(thread[3], (void**)&ret_val);

   shutdown_db();

   printf("\nend\n");
    
   return 0;
}
