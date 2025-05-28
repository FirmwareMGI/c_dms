#include <stdio.h>
#include <stdlib.h>
#include <mariadb/mysql.h>

char *hostname = "localhost";
char *username = "global";
char *password = "12345678";
char *database = "dms";
int port = 3306;

int main() {
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return EXIT_FAILURE;
    }

    if (mysql_real_connect(conn, hostname, username, password, database, 3306, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\nError: %s\n", mysql_error(conn));
        mysql_close(conn);
        return EXIT_FAILURE;
    }
    
    if (mysql_query(conn, "SELECT * FROM network")) {
        fprintf(stderr, "SELECT failed. Error: %s\n", mysql_error(conn));
        mysql_close(conn);
        return EXIT_FAILURE;
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "mysql_store_result() failed. Error: %s\n", mysql_error(conn));
        mysql_close(conn);
        return EXIT_FAILURE;
    }

    int num_fields = mysql_num_fields(res);

    while ((row = mysql_fetch_row(res))) {
        for(int i = 0; i < num_fields; i++) {
            printf("%s\t", row[i] ? row[i] : "NULL");
        }
        printf("\n");
    }

    mysql_free_result(res);
    mysql_close(conn);

    return EXIT_SUCCESS;
}
