#include "list_utils.h" // <--- VERY IMPORTANT: Include the header file!
#include <stdio.h>      // For printf (if you use it in these functions)
#include <stdlib.h>     // For calloc, malloc, free
#include <string.h>     // For strcmp

// Function implementations (the actual code - with curly braces {})

void list_init(string_list *list)
{
    list->list = calloc(1, sizeof(char *));
    list->length = 0;
}

void append_item(string_list *list, char *string)
{
    char *temp_array[list->length + 1];
    for (int i = 0; i < list->length; i++)
    {
        temp_array[i] = list->list[i];
    }
    temp_array[list->length] = string;
    free(list->list);
    list->list = calloc(list->length + 1, sizeof(char *));
    for (int i = 0; i < list->length + 1; i++)
    {
        list->list[i] = temp_array[i];
    }
    list->length++;
}

void remove_item(string_list *list, char *string)
{
    int found = 0;
    char *temp_array[list->length];
    for (int i = 0; i < list->length; i++)
    {
        if (strcmp(list->list[i], string) != 0)
        {
            temp_array[i] = list->list[i];
        }
        else
        {
            found = 1;
        }
    }

    if (found)
    {
        free(list->list);
        list->list = calloc(list->length - 1, sizeof(char *));
        for (int i = 0; i < list->length - 1; i++)
        {
            list->list[i] = temp_array[i];
        }
        list->length--;
    }
}