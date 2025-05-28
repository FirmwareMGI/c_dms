#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void store_in_db(const char* value) {
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[1024];

    // Initialize MySQL connection
    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return;
    }

    // Connect to MySQL database
    if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        return;
    }

    float float_value = atof(value);

    // Prepare SQL query
    snprintf(query, sizeof(query), "INSERT INTO %s (x_float) VALUES ('%f')", DB_TABLE, float_value);

    // Execute query
    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT failed. Error: %s\n", mysql_error(conn));
    } else {
        printf("Data inserted into database\n");
    }

    // Close the connection
    mysql_close(conn);
}

int read_flag_config(){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[1024];

    conn = mysql_init(NULL); 
    if (!conn) return -1;
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0)){
        mysql_close(conn); 
        return -1;
    }

    snprintf(query, sizeof(query), "SELECT * FROM flag_config");

    if (mysql_query(conn, query)){
        /* ... error handling ... */ 
        mysql_close(conn);
        return -1;
    }
    res = mysql_store_result(conn);
    if (!res){
        /* ... error handling ... */ 
        mysql_close(conn);
        return -1;
    }

    // debug
    // printf("Data from " DB_NAME ".network:\n");
    // int num_fields = mysql_num_fields(res);
    // while ((row = mysql_fetch_row(res))){
    //     for (int i = 0; i < num_fields; i++){
    //         printf("%s ", row[i] ? row[i] : "NULL");
    //     } 
    //     printf("\n"); 
    // }
    
    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}
int read_network(){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[1024];

    conn = mysql_init(NULL); 
    if (!conn) return -1;
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0)){
        mysql_close(conn); 
        return -1;
    }

    snprintf(query, sizeof(query), "SELECT * FROM network");

    if (mysql_query(conn, query)){
        /* ... error handling ... */ 
        mysql_close(conn);
        return -1;
    }
    res = mysql_store_result(conn);
    if (!res){
        /* ... error handling ... */ 
        mysql_close(conn);
        return -1;
    }

    // debug
    // printf("Data from " DB_NAME ".network:\n");
    // int num_fields = mysql_num_fields(res);
    // while ((row = mysql_fetch_row(res))){
    //     for (int i = 0; i < num_fields; i++){
    //         printf("%s ", row[i] ? row[i] : "NULL");
    //     } 
    //     printf("\n"); 
    // }
    
    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}

int read_network_mqtt(){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[1024];

    conn = mysql_init(NULL); 
    if (!conn) return -1;
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0)) { mysql_close(conn); return -1; }

    snprintf(query, sizeof(query), "SELECT ipserver, mqtt_server, mqtt_username, mqtt_pass, mqtt_port FROM network b");

    if (mysql_query(conn, query)) { /* ... error handling ... */ mysql_close(conn); return -1; }
    res = mysql_store_result(conn);
    if (!res) { /* ... error handling ... */ mysql_close(conn); return -1; }

    // debug
    // printf("Data from " DB_NAME ".network (mqtt):\n");
    // int num_fields = mysql_num_fields(res);
    // while ((row = mysql_fetch_row(res))){
    //     for (int i = 0; i < num_fields; i++){
    //         printf("%s ", row[i] ? row[i] : "NULL");
    //     } 
    //     printf("\n"); 
    // }

    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}

int read_device_list(){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[1024];

    conn = mysql_init(NULL); 
    if (!conn) return -1;
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0)) { mysql_close(conn); return -1; }

    snprintf(query, sizeof(query), "SELECT * FROM device_list");

    if (mysql_query(conn, query)){
        mysql_close(conn);
        return -1;
    }
    res = mysql_store_result(conn);
    if (!res) {
        mysql_close(conn);
        return -1;
    }

    // debug
    // printf("Data from " DB_NAME ".device_list:\n");
    // int num_fields = mysql_num_fields(res);
    // while ((row = mysql_fetch_row(res))){
    //     for (int i = 0; i < num_fields; i++){
    //         printf("%s ", row[i] ? row[i] : "NULL");
    //     } 
    //     printf("\n"); 
    // }

    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}

int read_device_list_by_mode(){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[1024];

    conn = mysql_init(NULL); 
    if (!conn) return -1;
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0)) { mysql_close(conn); return -1; }

    snprintf(query, sizeof(query), "SELECT * FROM device_list WHERE mode = 2");

    if (mysql_query(conn, query)) { /* ... error handling ... */ mysql_close(conn); return -1; }
    res = mysql_store_result(conn);
    if (!res) {
        mysql_close(conn);
        return -1;
    }

    // debug
    // printf("Data from " DB_NAME ".device_list (mode 2):\n");
    // int num_fields = mysql_num_fields(res);
    // while ((row = mysql_fetch_row(res))){
    //     for (int i = 0; i < num_fields; i++){
    //         printf("%s ", row[i] ? row[i] : "NULL");
    //     } 
    //     printf("\n"); 
    // }

    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}

// --- Example Implementation: read_dir_temp_flag_zero ---
int read_file_dr_fail() {
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[1024];

    conn = mysql_init(NULL); /* ... connection setup and error handling ... */
    if (!conn) return -1;
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0)) { mysql_close(conn); return -1; }

    snprintf(query, sizeof(query), "SELECT * FROM dir_temp WHERE flag = 0 AND time_stamp <= DATE_SUB(NOW(), INTERVAL 10 MINUTE)");

    if (mysql_query(conn, query)) { /* ... error handling ... */ mysql_close(conn); return -1; }
    res = mysql_store_result(conn);
    if (!res) { /* ... error handling ... */ mysql_close(conn); return -1; }

    // debug
    // printf("Data from read_dir_temp_flag_zero:\n");
    // int num_fields = mysql_num_fields(res);
    // while ((row = mysql_fetch_row(res))) { for (int i = 0; i < num_fields; i++) { printf("%s ", row[i] ? row[i] : "NULL"); } printf("\n"); }

    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}

// --- Example Implementation: read_dir_temp_flag_one ---
int read_file_dr_last() {
    MYSQL *conn; /* ... (similar structure as read_dir_temp_flag_zero, but with different query) ... */
    MYSQL_RES *res;
    char query[1024];

    conn = mysql_init(NULL); /* ... connection setup ... */
    if (!conn) return -1;
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0)) { mysql_close(conn); return -1; }

    snprintf(query, sizeof(query), "SELECT * FROM dir_temp WHERE flag = 1 AND time_stamp <= DATE_SUB(NOW(), INTERVAL 1 MONTH)"); // Different query

    if (mysql_query(conn, query)) { /* ... error handling ... */ mysql_close(conn); return -1; }
    res = mysql_store_result(conn); /* ... result processing ... */
    if (!res) { /* ... error handling ... */ mysql_close(conn); return -1; }
    
    // ... process result set ...
    // printf("Data from read_dir_temp_flag_one:\n");
    // int num_fields = mysql_num_fields(res);
    // while ((row = mysql_fetch_row(res))) { for (int i = 0; i < num_fields; i++) { printf("%s ", row[i] ? row[i] : "NULL"); } printf("\n"); }

    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}

int read_m_mesin(){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[1024];

    conn = mysql_init(NULL); 
    if (!conn) return -1;
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0)){
        mysql_close(conn); 
        return -1;
    }

    snprintf(query, sizeof(query), "SELECT * FROM m_mesin");

    if (mysql_query(conn, query)){
        /* ... error handling ... */ 
        mysql_close(conn);
        return -1;
    }
    res = mysql_store_result(conn);
    if (!res){
        /* ... error handling ... */ 
        mysql_close(conn);
        return -1;
    }

    // debug
    // printf("Data from " DB_NAME ".network:\n");
    // int num_fields = mysql_num_fields(res);
    // while ((row = mysql_fetch_row(res))){
    //     for (int i = 0; i < num_fields; i++){
    //         printf("%s ", row[i] ? row[i] : "NULL");
    //     } 
    //     printf("\n"); 
    // }
    
    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}

int read_m_file_iec_active(){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[1024];

    // Prepare SQL query (CROSS JOIN example)
    snprintf(query, sizeof(query), "SELECT a.*, b.address FROM file_sta a CROSS JOIN network b WHERE active = 1");

    // conn = mysql_init(NULL);
    // Execute query and process results (same as Example 1)
    if (mysql_query(conn, query)) { /* ... error handling ... */ }
    res = mysql_store_result(conn);
    if (res == NULL) { /* ... error handling ... */ }
    int num_fields = mysql_num_fields(res);
    while ((row = mysql_fetch_row(res))) {
        for (int i = 0; i < num_fields; i++) {
            printf("%s ", row[i] ? row[i] : "NULL");
        }
        printf("\n");
    }
    mysql_free_result(res);
}

int read_update_m_file_iec_active(){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[1024];

    // Prepare SQL query (CROSS JOIN example)
    snprintf(query, sizeof(query), "SELECT a.*, b.address FROM file_sta a CROSS JOIN network b WHERE active = 1");

    // conn = mysql_init(NULL);
    // Execute query and process results (same as Example 1)
    if (mysql_query(conn, query)) { /* ... error handling ... */ }
    res = mysql_store_result(conn);
    if (res == NULL) { /* ... error handling ... */ }
    int num_fields = mysql_num_fields(res);
    while ((row = mysql_fetch_row(res))) {
        for (int i = 0; i < num_fields; i++) {
            printf("%s ", row[i] ? row[i] : "NULL");
        }
        printf("\n");
    }
    mysql_free_result(res);
}



// --- Implementations for other query functions (select_file_sta_network_active, get_device_list_port_type_two, update_fileDR_temp_flag, insert_fileDR_temp, delete_it_file_iec_by_domain_id) would follow a similar pattern ---



// ... (Implementations for other functions, each with its specific query) ...