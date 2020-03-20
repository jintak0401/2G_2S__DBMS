#include "bpt.h"

// MAIN

int main( int argc, char ** argv ) {

    char * input_file;
    FILE * fp;
    int64_t key;
    int range2;
    char instruction;
    char license_part;
    char* value;

    if (argc > 1){
      printf("open table with path name < %s >\n\n", argv[1]);
      open_table(argv[1]);
    }

    else {
      printf("open table with path name < file >\n\n");
      open_table("file"); 
    }

    value = (char*)malloc(sizeof(char) * VALUE_SIZE);

    /*license_notice();*/

    usage();
    printf("\n=================================================\n");

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            scanf("%ld", &key);
            db_delete(key);
            break;
        case 'i':
            scanf("%ld %s", &key, value);
            db_insert(key, value);
            break;
        case 'f':
            scanf("%ld", &key);
            db_find(key, value);
            break;
        case 'p':
            scanf("%ld", &key);
            printing_page(key);
            break;
        case 'l':
            printing_leaves();
            break;
        case 'q':
            while (getchar() != (int)'\n');
            free(value);
            /*sorting();*/
            /*destroy_tree();*/
            return EXIT_SUCCESS;
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
