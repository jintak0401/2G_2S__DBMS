#include "bpt.h"
#include "file_manager.h"
#include "buffer.h"
#include <time.h>

// MAIN

int main( int argc, char ** argv ) {

    char input_file[100];
    FILE * fp;
    int64_t key;
    int range2;
    char instruction;
    char license_part;
    char* value;
    int table_id_1, table_id_2;
    int buf_num;

    /*time_t start, end;*/
    clock_t start, end;
    float delay;
/*
 *    if (argc > 1){
 *      printf("open table with path name < %s >\n\n", argv[1]);
 *      open_table(argv[1]);
 *    }
 *
 *    else {
 *      printf("open table with path name < file >\n\n");
 *      open_table("file"); 
 *    }
 */

    /*
     *open_table("jintak_1");
     *open_table("jintak_2");
     *open_table("jintak_3");
     *open_table("jintak_4");
     *open_table("jintak_5");
     *open_table("jintak_6");
     *open_table("jintak_7");
     *open_table("jintak_8");
     *open_table("jintak_9");
     *open_table("jintak_10");
     */


    open_table("1");
    open_table("2");
    open_table("3");
    open_table("4");
    open_table("5");
    open_table("6");
    open_table("7");
    open_table("8");
    open_table("9");
    open_table("10");


    fp = fopen(argv[1], "r");


    /*
     *value = (char*)malloc(sizeof(char) * VALUE_SIZE); 
     *printf("\nHow many buffers do you need?\n");
     *scanf("%d", &buf_num);
     *while(getchar() != '\n');
     *init_db(buf_num);
     */

    value = (char*)malloc(sizeof(char) * VALUE_SIZE); 
    printf("\nHow many buffers do you need?\n");
    fscanf(fp, "%d", &buf_num);
    while(getchar() != '\n');
    init_db(buf_num);
    /*
     *start = time(NULL);
     *printf("start : %ld\n", start);
     */
    /*license_notice();*/

    usage();
    printf("\n=================================================\n");

    printf("> ");
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
            while (getchar() != (int)'\n');
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
            while(getchar() != (int)'\n');
            printf("\n\n");
            join_table(table_id_1, table_id_2, input_file);
            file_read_for_join(input_file);
            break;
        case 'z' :
            /*start = time(NULL);*/
            start = clock();
            break;
        case 'y' :
            /*end = time(NULL);*/
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
        while (getchar() != (int)'\n');
        printf("> ");
        /*
         *check_zero();
         *check();
         *check_ascending();
         */
    }
    printf("\n");
    free(value);

    /*
     *end = time(NULL);
     *printf("end : %ld\n", end);
     *printf("\n==================================\n");
     *printf("delay : %ld\n", (end - start)) ;
     *printf("==================================\n");
     */

    return EXIT_SUCCESS;
}
