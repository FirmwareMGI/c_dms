#include <stdio.h> // for printf
#include <stdlib.h> // for calloc, malloc and free
#include <string.h> // for strcmp

// using struct to package all information into a single variable
typedef struct
{
    char **list;
    int length;
} string_list;

// if you are planning on using several lists, it's better to create an init function
void list_init(string_list *list)
{
    list->list = calloc(1, sizeof(char *));
    list->length = 0;
}

void append_item(string_list *list, char *string) // note that I am using "string_list *" instead of just "string_list".
{
    // it is okay to use a fixed-sized array here because it is a temporary array
    char *temp_array[list->length + 1]; // + 1 is for a new element

    // adding the list->list elements to the temp_array
    for (int i = 0; i < list->length; i++)
    {
        temp_array[i] = list->list[i];
    }

    // adding the new element to the temp_array
    temp_array[list->length] = string; // 0 is the first number, that's why "list->length" instead of "list->length + 1"

    // freeing the list->list (it should be allocated to make this function work)
    free(list->list);

    // allocating memory for list->list
    list->list = calloc(list->length + 1, sizeof(char *)); // I am using calloc instead of malloc here because it just makes the code cleaner, apart from that there is no difference between them

    // adding elements from temp_array to list->list
    for (int i = 0; i < list->length + 1; i++)
    {
        list->list[i] = temp_array[i];
    }

    list->length++;
}

void remove_item(string_list *list, char *string) // note that I am using "string_list *" instead of just "string_list".
{
    // it is okay to use a fixed-sized array here because it is a temporary array
    char *temp_array[list->length]; // without "-1" because it is uncertain if the list includes the element
    // a variable for tracking, whether or not the list includes the element; set to "false" by default
    int found = 0;

    // adding all elements from list->list to temp_array, except the one that should be removed
    for (int i = 0; i < list->length; i++)
    {
        // if the element is not the one that should be removed
        if (strcmp(list->list[i], string) != 0)
        {
            temp_array[i] = list->list[i];
        }
        else
        {
            found = 1;
        }
    }

    // if not found there is no point of doing the following
    if (found)
    {
        // freeing the list->list (it should be allocated to make this function work)
        free(list->list);

        // allocating memory for list->list
        list->list = calloc(list->length - 1, sizeof(char *)); // I am using calloc instead of malloc here because it just makes the code cleaner, apart from that there is no difference between them

        // adding elements from temp_array to list->list
        for (int i = 0; i < list->length - 1; i++)
        {
            list->list[i] = temp_array[i];
        }

        list->length--;
    }
}

/*
int main(void)
{
    string_list list;

     // note that I am passing to the functions the pointer to the list, not the list itself
    init(&list);

    append_item(&list, "hello, world");
    append_item(&list, "hello again");
    append_item(&list, "another string");

    for (int i = 0; i < list.length; i++)
    {
        printf("%s\n", list.list[i]);
    }

    printf("----------------\n");

    remove_item(&list, "another string");

    for (int i = 0; i < list.length; i++)
    {
        printf("%s\n", list.list[i]);
    }

    // don't forget to free the list after usage
    free(list.list);

    return 0;
}
*/