#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include "bpt.h"

// 한 페이지의 크기
#define PAGE_SIZE 4096

// 파일에서 header_page 의 위치 
#define HEADER_PAGE_NUM 0

// free page 리스트로부터 페이지 할당하여 반환
pagenum_t file_alloc_page(int table_id, header_t head);

// 해당 페이지를 free page 로 만들기
void file_free_page(int table_id, header_t head, pagenum_t page_num);

//  해당 페이지를 하드로부터 dest 로 읽어 들이기
int file_read_page(int table_id, pagenum_t page_num, page_t dest);

// 해당 페이지를 src 로부터 하드로 쓰기
int file_write_page(int table_id, pagenum_t page_num, page_t src);

// 맨 처음 실행할 때 파일을 여는 함수
int open_table(char* pathname);

// tree 를 맨 처음 실행할 때 header_page 를 생성하는 함수
header_t make_header();

// buffer 의 close_table 함수가 호출 될 때 해당 fd 를 닫는 함수
int close_fd(int table_id);



#endif /*__FILE_MANAGER_H__*/
