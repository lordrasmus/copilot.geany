
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <pthread.h>

#include <geanyplugin.h>

#include "jsonrpc.h"


static int next_id = 0;


static int copilot_stdin[2];
static int copilot_stdout[2];



static pthread_mutex_t copilot_api_send_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t copilot_api_result_mutex = PTHREAD_MUTEX_INITIALIZER;


static json_object* last_result = 0;



json_object* get_last_result( void ){
    return last_result;
}



void lock_copilot_api_mutex( void ){
    //printf("  send api lock\n");fflush(stdout);
    pthread_mutex_lock(&copilot_api_send_mutex);
}

void unlock_copilot_api_mutex( void ){
    //printf("  send api unlock\n");fflush(stdout);
    pthread_mutex_unlock(&copilot_api_send_mutex);
    
}



json_object* jsonreq_init( const char* method ){
    json_object* ret_val = json_object_new_object();    
    json_object_object_add(ret_val,"method",json_object_new_string(method));
    
    return ret_val;
}

json_object* jsonreq_add_param_obj( json_object* json_req, const char * name ){
    
    json_object* params_obj;
    
    if ( json_object_object_get_ex(json_req, "params", &params_obj) == 0 ){
       params_obj = json_object_new_object();  
       json_object_object_add(json_req,"params",params_obj);
    }
    
    json_object* add_obj = json_object_new_object();  
    json_object_object_add(params_obj,name,add_obj);
    
    return add_obj;
    
}

json_object* jsonreq_add_param_array( json_object* json_req, const char * name ){
    
    json_object* params_obj;
    
    if ( json_object_object_get_ex(json_req, "params", &params_obj) == 0 ){
       params_obj = json_object_new_object();  
       json_object_object_add(json_req,"params",params_obj);
    }
    
    json_object* add_obj = json_object_new_array();  
    json_object_object_add(params_obj,name,add_obj);
    
    return add_obj;
    
}



void jsonreq_add_param_str( json_object* json_req, const char * name, const char* format, ... ){
    
    json_object* params_obj;
    
    if ( json_object_object_get_ex(json_req, "params", &params_obj) == 0 ){
       params_obj = json_object_new_object();  
       json_object_object_add(json_req,"params",params_obj);
    }
    
    
    va_list arg;
    
    va_start(arg, format);
    char buffer[vsnprintf(0, 0, format, arg)+1];
    va_end(arg);

    va_start(arg, format);
    vsprintf(buffer, format, arg);
    va_end(arg);
    
    
    json_object_object_add( params_obj ,name ,json_object_new_string( buffer ));
    
}

void jsonreq_add_param_int( json_object* json_req, const char * name, const int value){
    
    json_object* params_obj;
    
    if ( json_object_object_get_ex(json_req, "params", &params_obj) == 0 ){
       params_obj = json_object_new_object();  
       json_object_object_add(json_req,"params",params_obj);
    }
    
    
    json_object_object_add( params_obj ,name ,json_object_new_int( value ));
    
}    

void jsonreq_add_param_null( json_object* json_req, const char * name){
    json_object* params_obj;
    
    if ( json_object_object_get_ex(json_req, "params", &params_obj) == 0 ){
       params_obj = json_object_new_object();  
       json_object_object_add(json_req,"params",params_obj);
    }
    
    
    json_object_object_add( params_obj ,name ,0);
}


void jsonreq_add_obj_str( json_object* json_obj , const char* name, const char* format, ... ){
    
    va_list arg;
    
    va_start(arg, format);
    char buffer[vsnprintf(0, 0, format, arg)+1];
    va_end(arg);

    va_start(arg, format);
    vsprintf(buffer, format, arg);
    va_end(arg);
    
    json_object_object_add( json_obj ,name ,json_object_new_string( buffer ));
}

void jsonreq_add_obj_int( json_object* json_obj , const char* name, const int value ){
    json_object_object_add( json_obj ,name ,json_object_new_int( value ));
}

void jsonreq_add_obj_bool( json_object* json_obj , const char* name, const int value ){
    json_object_object_add( json_obj ,name ,json_object_new_boolean( value ));
}



void jsonreq_send( json_object* ret_val, int async, int no_log ){


    json_object_object_add(ret_val,"jsonrpc",json_object_new_string("2.0"));
    if ( async == 0 ){
        json_object_object_add(ret_val,"id",json_object_new_int(next_id++)); // id darf nicht bei allen requests gesendet werden. muss was mit sync/asny zu tun haben
    }
    
    const char* json_str = json_object_to_json_string(ret_val);
    
    char header_buffer[1000];
    char header_len = 0;
    
    if ( no_log == 0 ){
        if ( async == 1 ){
            printf("\033[01;32msend asnyc\033[00m: %s\n", json_str );
        }else{
           
            printf("\033[01;32msend id %d \033[00m: %s\n", next_id -1, json_str );
        }
    }
        
    header_len = sprintf( header_buffer, "Content-Length: %ld\r\nContent-Type: application/vscode-jsonrpc; charset=utf-8\r\n\r\n", strlen( json_str ) );
    
    
    write( copilot_stdin[1] , header_buffer, header_len );
    write( copilot_stdin[1] , json_str, strlen( json_str ) );
    
    
    if ( async == 0 ){
        pthread_mutex_lock(&copilot_api_result_mutex);
    }
    
    
    json_object_put(ret_val);
    
}

void jsonreq_send_async( json_object* ret_val, int no_log ){
    
    jsonreq_send( ret_val , 1, no_log );
    
}

void jsonreq_send_sync( json_object* ret_val, int no_log ){
    
    jsonreq_send( ret_val , 0, no_log );
    
}

void jsonreq_send_sync_and_free( json_object* ret_val, int no_log ){
    
    jsonreq_send( ret_val , 0, no_log );
    
    json_object_put( last_result );
    last_result = 0;
    
}


/*
 * 
 * Result Functions
 * 
 */

const char* jsonresp_get_string( const char* name ){

    const char* ret = "jsonresp: not found";

    struct json_object *result_obj;
    if ( json_object_object_get_ex( last_result, "result", &result_obj) != 0 ){
        struct json_object *version_obj;
        if ( json_object_object_get_ex(result_obj, name, &version_obj) != 0 ){
            return json_object_get_string(version_obj);
        }
    }
    
    return ret;

}        


void jsonresp_free( void ){
    if ( last_result != 0 ){
        json_object_put( last_result );
    }
    last_result = 0;
}

/*
 * 
 * Handler
 * 
 */        



void handle_async_msg( const json_object *json_obj , const char *json_str){
    
    
    struct json_object *result_obj;
    
    const char *method = 0;
    
    if ( json_object_object_get_ex(json_obj, "method", &result_obj) != 0 ){
        method = json_object_get_string(result_obj);
    }
    
    
    
    if ( 0 == strcmp( method, "LogMessage" ) ){
        
            struct json_object *params_obj;
            if (json_object_object_get_ex(json_obj, "params", &params_obj)) {
                
                struct json_object *message_obj;
                if (json_object_object_get_ex(params_obj, "message", &message_obj)) {
            
                    msgwin_msg_add(COLOR_BLACK, -1, NULL,"Copilot: %s", json_object_get_string(message_obj) );
                    return;
                    
                }
                
            }
            
    }
    
    printf("\033[01;31munhandled recv async\033[00m : %s\n", json_str );
    
        
}

void *copilot_read_thread(void *arg) {
    
    char buffer[10000];
    
    char req_end[4] = { 13,10,13,10};

    while(1){
        // Lese Daten vom Dateideskriptor
        ssize_t bytesRead = read( copilot_stdout[0], buffer, 16);
        buffer[bytesRead] = 0;
        
        if ( 0 != memcmp("Content-Length: ", buffer, 16 ) ){
            printf("Copilot Protokoll Error: not Content-Length  got : >%s< \n",buffer );
            exit(1);
        }
        
        int buffer_off = 0;
        while( buffer_off < 200 ){
            read( copilot_stdout[0], &buffer[ buffer_off ], 1);
            //printf("c: %c ( %d )  %d\n", buffer[ buffer_off ], buffer[ buffer_off ],buffer_off  );
            
            if ( 0 == memcmp( req_end, &buffer[ buffer_off -3 ], 4 ) ){
                break;
            }
            
            buffer_off++;
        }
        
        int Content_Length = atoi( buffer );
        read( copilot_stdout[0], buffer, Content_Length);
        buffer[ Content_Length ] = 0;
        
        
        
        json_object *json_obj = json_tokener_parse( buffer );

        if (json_obj == NULL) {
            fprintf(stderr, "Error parsing JSON string\n");
            exit(1);
        }
        
        struct json_object *id_obj = 0;
        if ( json_object_object_get_ex(json_obj, "id", &id_obj) != 0 ){
        
            int id = json_object_get_int(id_obj);
            
            printf("\033[01;31mrecv id %d\033[00m : %s\n", id, buffer );
            
            last_result = json_obj;
        
            pthread_mutex_unlock(&copilot_api_result_mutex);
        }else{
            
            
            handle_async_msg( json_obj, buffer );      
            
            json_object_put(json_obj);
            
        }
        
        
    }
    
    return NULL;
}


pid_t init_copilot_threads( void ){
    
    pipe( copilot_stdin);
    pipe( copilot_stdout);
    
    //char* node_path = "/usr/bin/node";
    char* node_path = "/home/ramin/src/geany-copilot/node-v20.10.0-linux-x64/bin/node";
      
    //char *start_cmd[] = { node_path , "/home/ramin/src/geany-copilot/engines/1.133.0/index.js", NULL};
    char *start_cmd[] = { node_path , "/home/ramin/src/geany-copilot/engines/1.138.0/index.js", NULL};
    


    // Erstelle einen neuen Prozess
    pid_t pid = fork();

    if (pid == -1) {
        // Fehler beim Erstellen des Prozesses
        perror("Fehler beim fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        
        //setenv("COPILOT_AGENT_VERBOSE", "1", 1);
        
        dup2( copilot_stdin[0],  0);
        dup2( copilot_stdout[1], 1);
        
        // Kindprozess (auszuführender Code)
        execv(start_cmd[0], start_cmd);
    
        // Falls execv fehlschlägt
        perror("Fehler beim execv");
        exit(EXIT_FAILURE);
    } 
    
    pthread_mutex_lock(&copilot_api_result_mutex);
    
    pthread_t my_thread; // Thread identifier
    if (pthread_create(&my_thread, NULL, copilot_read_thread, NULL) != 0) {
        fprintf(stderr, "Error creating thread\n");
        exit(EXIT_FAILURE);
    }
    
    return pid;
    
}
