#include <stdio.h>
#include <mariadb/mysql.h>
#include "comms.h"

int connection(); // Forward declaration

int main(){
    printf("hello\n");
    connection();
    return 0;
}

int connection(){
    char query[2000];
    MYSQL *conn;
    MYSQL_RES *res; /* holds the result set */
    MYSQL_ROW row;
    int num_fields;

    // Initialize the MySQL connection
    conn = mysql_init(NULL);

    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return 1;
    }

    // Connect to the MySQL database
    if (mysql_real_connect(conn, "localhost", "global", "12345678", "dms", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        return 1;
    }

    // Query to select all rows from fileDR table
    if (mysql_query(conn, "SELECT * FROM fileDR")) {
        fprintf(stderr, "SELECT * FROM fileDR failed. Error: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    // Store the result from the query
    res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "mysql_store_result() failed. Error: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    // Get the number of fields in the result
    num_fields = mysql_num_fields(res);

    // Print the column headers (optional)
    printf("Column headers:\n");
    MYSQL_FIELD *field;
    while ((field = mysql_fetch_field(res))) {
        printf("%s\t", field->name);  // Print column name
    }
    printf("\n");

    // Iterate over the result set and print each row
    while ((row = mysql_fetch_row(res))) {
        for (int i = 0; i < num_fields; i++) {
            if (row[i] != NULL) {
                printf("%s\t", row[i]);
            } else {
                printf("NULL\t");
            }
        }
        printf("\n");
    }

    // Free the result set and close the connection
    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}
