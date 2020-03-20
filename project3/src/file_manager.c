#include "bpt.h"
#include "file_manager.h"


// free_page 를 사용하여 new page 를 만들었는지 여부
int use_free_page = 0;


int fd[11] = {0, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3};
char fd_name[10][100];

int fd_size = 0;
int fd_last_position = 0;



// 반환되는 page_num 의 첫 8바이트에 적힌 값이 리스트의 가장 마지막 free_page
pagenum_t file_alloc_page(int table_id, header_t head){

  pagenum_t page_num;
  free_page_t free_page;
  int do_read;

  if (fd[table_id] == -3){
    perror("calling not exist table_id");
    exit(EXIT_FAILURE);
  }

  // page_num == 0 일 경우 file 의 가장 마지막 위치 반환 | 그 외 가장 마지막 free_page_num 반환
  page_num = head->next_free_page;

  // free page 가 있었을 경우 head 의 다음 next_free_page 을 고쳐줌
  if (page_num != 0){

    use_free_page = 1;

    free_page = (free_page_t)malloc(sizeof(_free_page_t));
    if (free_page == NULL){
      perror("free_page_creation.\n");
      exit(EXIT_FAILURE);
    }
    file_read_page(table_id, page_num, (page_t)free_page);
    head->next_free_page = free_page->next_free_page;
    free(free_page);
    free_page = NULL;

  }
  return (page_num == 0) ? head->num_pages : page_num;
}

// 인자로 받은 page_num 을 free 페이지로 만들고 해당 페이지를 파일에 저장한다
void file_free_page(int table_id, header_t head, pagenum_t page_num) {

  if (fd[table_id] == -3){
    perror("calling not exist table_id");
    exit(EXIT_FAILURE);
  }

  // 인자의 page_num 에 해당하는 파일에 free page 집어넣기
  free_page_t recent_free = (free_page_t)calloc(1, sizeof(_free_page_t));
  if (recent_free == NULL){
    perror("free_page creation.");
    exit(EXIT_FAILURE);
  }

  // head 가 가리키는 free_page 가 되도록 next_free_page 최신화
  recent_free->next_free_page = head->next_free_page;  
  file_write_page(table_id, page_num, (page_t)recent_free);
  

  head->next_free_page = page_num;

  // head 최신화
  /*file_write_page(table_id, HEADER_PAGE_NUM, (page_t)head);*/

  free(recent_free);
  recent_free = NULL;
}

// return 값 -> 1 : 성공적으로 읽은 경우 | 0 : 파일의 끝이어서 못 읽은 경우 | -1 : 에러
int file_read_page(int table_id, pagenum_t page_num, page_t dest){

  if (fd[table_id] == -3){
    perror("calling not exist table_id");
    exit(EXIT_FAILURE);
  }

  ssize_t toReturn = pread(fd[table_id], dest, PAGE_SIZE, PAGE_SIZE * page_num);
  return (toReturn > 0) ? 1 : (toReturn == 0) ? 0 : -1;
}

// return 값 -> 1 : 성공적으로 쓴 경우 | 0 : 에러
int file_write_page(int table_id, pagenum_t page_num, page_t src){

  if (fd[table_id] == -3){
    perror("calling not exist table_id");
    exit(EXIT_FAILURE);
  }

  ssize_t toReturn = pwrite(fd[table_id], src, PAGE_SIZE, PAGE_SIZE * page_num);
  return (toReturn > 0) ? 1 : 0;
}

// fd 설정해줌 && header_page 읽음(만약 없다면 만들어준다)
int open_table(char* pathname){
  int table_id = -1;
  header_t head = 0; 

  // 이미 10개의 파일을 생성해서 추가로 생성할 수 없을 때
  if (fd_size == 10){
    printf("\nThere are already 10 files\n");
    return -1;
  }
  // 이미 pathname 으로 생성된 파일이 있을 때
  for (int i = 0; i <= fd_last_position; i++){
    if (strcmp(fd_name[i], pathname) == 0){
      printf("\nThere is already < %s > file.  Its table id is < %d >\n", pathname, i + 1);
      return i + 1;
    }
    else if (fd[i + 1] == -3 && table_id == -1){
      table_id = i + 1;
      if (i == fd_last_position){
        fd_last_position++;
      }
    }
  }

  if (table_id == -1){
    table_id = ++fd_last_position;
  }

  fd_size++;
  fd[table_id] = open(pathname, O_RDWR | O_CREAT, 0644);
  strcpy(fd_name[table_id - 1], pathname);
  head = make_header();
  if (fd[table_id] == -1){
    perror("fd alloc faile in < open_table > at [ file_manager.c ]");
    exit(EXIT_FAILURE);
  }
  // 0 ~ 4095 가 비어있을 때 == db 데이터가 없을 때
  else if (file_read_page(table_id, HEADER_PAGE_NUM, (page_t)head) <= 0){
    file_write_page(table_id, HEADER_PAGE_NUM, (page_t)head);
  }
  // header_page 가 있을 때
  else {
    /*file_read_page(table_id, HEADER_PAGE_NUM, (page_t)head);*/
    // root_page 가 이미 있을 때만 root 를 읽어들인다. 없다면 insert 함수 중에 생성된다
    /*
     *if (head->root_page_num > 0){
     *  root = (internal_t)calloc(1, sizeof(_internal_t));
     *  if (root == NULL){
     *    perror("reading root.");
     *    exit(EXIT_FAILURE);
     *  }
     *  file_read_page(head->root_page_num, (page_t)root);
     *}
     */
  }

  printf("\n[ %s ] is [ table %d ]\n", pathname, table_id);

  free(head);
  head = NULL;

  return table_id;
}

// header_page 가 없을 때 만드는 함수
header_t make_header(){
  header_t head = (header_t)calloc(1, sizeof(_header_t));
  if (head == NULL){
    perror("head creation fail in < make_header > at [ file_manager.c ]");
    exit(EXIT_FAILURE);
  }
  head->next_free_page = 0;

  // make_header 함수를 쓰는 때는 root 도 없을 것이므로 root_page_num 을 0 로 초기화한다.
  head->root_page_num = 0;
  head->num_pages = 1;

  return head;
}

int close_fd(int table_id){
  if (table_id > 10 || table_id < 1){
    printf("\nThere are only [ page 1 ] ~ [ page 10 ]");
    return -1;
  }
  if (fd[table_id] == -3){
    printf("\nThere is no [ table %d ]\n", table_id);
    return -1;
  }
  else if (fd_last_position == table_id){
    fd_last_position--;
  }
  printf("\n[ table %d ] (= [ %s ] ) is closed\n", table_id, fd_name[table_id - 1]);
  fd_size--;
  fd[table_id] = -3;  
  strcpy(fd_name[table_id - 1], "");
}
