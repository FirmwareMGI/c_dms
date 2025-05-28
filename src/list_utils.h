#ifndef LIST_UTILS_H
#define LIST_UTILS_H

// Structure definition for string_list
typedef struct 
{
    char **list;
    int length;
} string_list;

// Function prototypes (declarations)
void list_init(string_list *list);
void append_item(string_list *list, char *string);
void remove_item(string_list *list, char *string);

#endif // LIST_UTILS_H