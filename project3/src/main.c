#include "bpt.h"
#include "file_manager.h"
#include "buffer.h"

// MAIN

int main( int argc, char ** argv ) {

    char * input_file;
    FILE * fp;
    int64_t key;
    int range2;
    char instruction;
    char license_part;
    char* value;
    int table_id;
    int buf_num;

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

    value = (char*)malloc(sizeof(char) * VALUE_SIZE);

    printf("\nHow many buffers do you need?\n");
    scanf("%d", &buf_num);
    while(getchar() != '\n');
    init_db(buf_num);

    /*license_notice();*/

    usage();
    printf("\n=================================================\n");

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        /*
         *case 'b':
         *    printing_buffer();
         *    break;
         *case 's':
         *    scanf("%d", &table_id);
         *    sorting(table_id);
         *    break;
         */
        case 'd':
            scanf("%d %ld", &table_id, &key);
            db_delete(table_id, key);
            break;
        case 'i':
            scanf("%d %ld %s", &table_id, &key, value);
            db_insert(table_id, key, value);
            break;
        case 'f':
            scanf("%d %ld", &table_id, &key);
            db_find(table_id, key, value);
            break;
        case 'p':
            scanf("%d %ld", &table_id, &key);
            printing_page(table_id, key);
            break;
        case 'l':
            scanf("%d", &table_id);
            printing_leaves(table_id);
            break;
        case 'q':
            while (getchar() != (int)'\n');
            free(value);
            shutdown_db();
            /*sorting();*/
            /*destroy_tree();*/
            return EXIT_SUCCESS;
            break;
        case 'o' :
            scanf("%s", input_file);
            open_table(input_file);            
            break;
        case 'c' :
            scanf("%d", &table_id);
            close_table(table_id);            
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

    return EXIT_SUCCESS;
}
