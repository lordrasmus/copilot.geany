
#ifndef GEANY_JSON_RPC_H
#define GEANY_JSON_RPC_H

#include <json-c/json.h>


GSubprocess* init_copilot_threads( GeanyPlugin *plugin, char* node_bin, char* engine_path );


void lock_copilot_api_mutex( void );
void unlock_copilot_api_mutex( void );

json_object* get_last_result( void );


json_object* jsonreq_init( const char* method );

/*
 *  Parameter Functions
 */
json_object* jsonreq_add_param_obj( json_object* json_req, const char * name );
json_object* jsonreq_add_param_array( json_object* json_req, const char * name );

void         jsonreq_add_param_str( json_object* json_req, const char * name, const char* format, ... );
void         jsonreq_add_param_int( json_object* json_req, const char * name, const int value);
void         jsonreq_add_param_null( json_object* json_req, const char * name);


/*
 *  Standart Dict Funktions
 */
void jsonreq_add_obj_str( json_object* json_obj , const char* name, const char* format, ... );
void jsonreq_add_obj_int( json_object* json_obj , const char* name, const int value );
void jsonreq_add_obj_bool( json_object* json_obj , const char* name, const int value );


/*
 *  Send Funktions
 */
void jsonreq_send_async( json_object* ret_val, int no_log );
void jsonreq_send_sync( json_object* ret_val, int no_log );
void jsonreq_send_sync_and_free( json_object* ret_val, int no_log );


/*
 * Result Functions
 */

const char* jsonresp_get_string( const char* name );

void jsonresp_free( void );

#endif
