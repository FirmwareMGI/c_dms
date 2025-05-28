#ifndef DATABASE_H
#define DATABASE_H

#include <mariadb/mysql.h>

#define DB_HOST "localhost"
#define DB_USER "global"
#define DB_PASSWORD "12345678"
#define DB_NAME "dms"
#define DB_TABLE "relay"

// Function declaration for DB store
void store_in_db(const char *value);

// read operations
// int read_data_table(table);
int read_flag_config();
int read_network();
int read_network_mqtt();
int read_device_list();
int read_device_list_by_mode();
int read_file_dr_fail();
int read_file_dr_last();
int read_m_mesin();
int read_m_file_iec_active();
int read_update_m_file_iec_active();

// int read_dir_temp_flag_zero();
// int read_dir_temp_flag_one();
// int select_file_sta_network_active();
// int get_device_list_port_type_two();
// int update_fileDR_temp_flag(int id_to_update); // Parameterized update
// int insert_fileDR_temp(const char *port_device, int id_device, const char *status, int flag, const char *nama); // Parameterized insert
// int delete_it_file_iec_by_domain_id(const char *domainId); // Parameterized delete


#endif // DATABASE_H