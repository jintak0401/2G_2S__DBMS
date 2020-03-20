#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 500
#define BUFFER 10

void make_str(char str[]){
  int length = random() % 52 + 1;
  char tmp[60];
  char c;
  int i;
  for (i = 0; i < length; i++){
    if (i < 26){
      c = 'a' + i;
    }
    else {
      c = 'A' + i - 26;
    }
    tmp[i] = c;
  }
  tmp[i] = '\0';
  strcpy(str, tmp);
}

int main(){

  int size;


  char insertion_name[100];
  char deletion_name[100];
  sprintf(insertion_name, "%d_i.txt", SIZE);
  sprintf(deletion_name, "%d_d.txt", SIZE);

  FILE* fp = fopen(insertion_name, "w");
  FILE* fp2 = fopen(deletion_name, "w");

  fprintf(fp, "%d\n", BUFFER);
  fprintf(fp2, "%d\n", BUFFER);

  srand(time(NULL));
  int random = 0;
  char str[60];

  long* arr = (long*)malloc(sizeof(long) * SIZE);
  int* file_num = (int*)malloc(sizeof(int) * SIZE);
  
  for (int i = 1; i < SIZE + 1; i++){
    /*arr[i - 1] = rand() % SIZE + 1;*/
    arr[i - 1] = i;
    /*file_num[i - 1] = (rand() % 10 + 1);*/
    file_num[i - 1] = 4;
    /*
     *fprintf(fp, "i %d %ld %ld\n", file_num[i - 1], arr[i - 1], (long)i);
     *fprintf(fp3, "i %ld %ld\n", arr[i - 1], (long)i);
     */
  }
  for (int i = 0; i < SIZE; i++){
    int tmp = rand() % SIZE;
    int swap = arr[i];
    arr[i] = arr[tmp];
    arr[tmp] = swap;
  }
  fprintf(fp, "z\n");
  for (int i = 0; i < SIZE; i++){
    make_str(str);
    fprintf(fp, "i %d %ld %s\n", file_num[i], arr[i], str);
  }
  fprintf(fp, "y\nq\n\n");

  for (int i = 0; i < SIZE; i++){
    int tmp = rand() % SIZE;
    int swap = arr[i];
    arr[i] = arr[tmp];
    arr[tmp] = swap;
    swap = file_num[i];
    file_num[i] = file_num[tmp];
    file_num[tmp] = swap;
  }
  fprintf(fp2, "z\n");
  for (int i = 0; i < SIZE; i++){
    fprintf(fp2, "d %d %ld\n", file_num[i], arr[i]);
  }
  fprintf(fp2, "y\nq\n");
  fclose(fp);
  fclose(fp2);
  free(arr);

}
