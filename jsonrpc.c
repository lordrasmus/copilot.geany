
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <geanyplugin.h>

#include "jsonrpc.h"


static int jsonrpc_log;

static int next_id = 0;


//static int copilot_stdin[2];
//static int copilot_stdout[2];

//static GIOStream *stream;
GInputStream *input_stream;
GOutputStream *output_stream;


static pthread_mutex_t copilot_api_send_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t copilot_api_result_mutex = PTHREAD_MUTEX_INITIALIZER;


static json_object* last_result = 0;

static int next_sync_no_log = 0;


json_object* get_last_result( void ){
    return last_result;
}



void write_to_log( char* format, ... ){
    
    va_list arg;
    
    va_start(arg, format);
    char buffer[vsnprintf(0, 0, format, arg)+1];
    va_end(arg);

    va_start(arg, format);
    int len = vsprintf(buffer, format, arg);
    va_end(arg);
    
    
    write( jsonrpc_log, buffer, len + 1 );
    
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



static void jsonreq_send( json_object* ret_val, int async, int no_log ){


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
        
    header_len = sprintf( header_buffer, "Content-Length: %lld\r\nContent-Type: application/vscode-jsonrpc; charset=utf-8\r\n\r\n", strlen( json_str ) );
    
    
    //write( copilot_stdin[1] , header_buffer, header_len );
    //write( copilot_stdin[1] , json_str, strlen( json_str ) );
    
    GError *error = NULL;
    
    g_output_stream_write(output_stream, header_buffer, header_len, NULL, &error);
    g_output_stream_write(output_stream, json_str, strlen( json_str ), NULL, &error);
    
    
    /*if ((bytes_written = g_output_stream_write(output_stream, data_to_write, data_size, NULL, &error)) == -1) {
        // Fehler beim Schreiben
        g_print("Error writing to stdin: %s\n", error->message);
        g_error_free(error);
    } else {
        // Schreiben erfolgreich
    }*/
    
    
    if ( async == 1 ){
        write_to_log( "--------------------------- send async  ----------------------------\n%s\n",   json_object_to_json_string_ext( ret_val, JSON_C_TO_STRING_PRETTY) );
    }else{
        write_to_log( "--------------------------- send sync %d   ----------------------------\n%s\n", next_id -1,  json_object_to_json_string_ext( ret_val, JSON_C_TO_STRING_PRETTY) );
    }
    
    if ( async == 0 ){
        pthread_mutex_lock(&copilot_api_result_mutex);
    }
    
    
    json_object_put(ret_val);
    
}

void jsonreq_send_async( json_object* ret_val, int no_log ){
    
    jsonreq_send( ret_val , 1, no_log );
    
}

void jsonreq_send_sync( json_object* ret_val, int no_log ){
    
    next_sync_no_log = no_log;
    
    jsonreq_send( ret_val , 0, no_log );
    
}

void jsonreq_send_sync_and_free( json_object* ret_val, int no_log ){
    
    next_sync_no_log = no_log;
    
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
    
    GError *error = NULL;

    while(1){
        // Lese Daten vom Dateideskriptor
        //ssize_t bytesRead = read( copilot_stdout[0], buffer, 16);
        
        ssize_t bytesRead = g_input_stream_read(input_stream, buffer, 16, NULL, &error);
        buffer[bytesRead] = 0;
        
        //printf("read %ld\n", bytesRead );
        
        if ( 0 != memcmp("Content-Length: ", buffer, 16 ) ){
            printf("Copilot Protokoll Error: not Content-Length  got : >%s< \n",buffer );
            exit(1);
        }
        
        int buffer_off = 0;
        while( buffer_off < 200 ){
            
            g_input_stream_read(input_stream, &buffer[ buffer_off ], 1, NULL, &error);
            
            //read( copilot_stdout[0], &buffer[ buffer_off ], 1);
            //printf("c: %c ( %d )  %d\n", buffer[ buffer_off ], buffer[ buffer_off ],buffer_off  );
            
            if ( 0 == memcmp( req_end, &buffer[ buffer_off -3 ], 4 ) ){
                break;
            }
            
            buffer_off++;
        }
        
        int Content_Length = atoi( buffer );
        //read( copilot_stdout[0], buffer, Content_Length);
        g_input_stream_read(input_stream, buffer, Content_Length, NULL, &error);
        buffer[ Content_Length ] = 0;
        
        
        
        json_object *json_obj = json_tokener_parse( buffer );

        if (json_obj == NULL) {
            fprintf(stderr, "Error parsing JSON string\n");
            exit(1);
        }
        
        struct json_object *id_obj = 0;
        if ( json_object_object_get_ex(json_obj, "id", &id_obj) != 0 ){
        
            int id = json_object_get_int(id_obj);
            
            if ( next_sync_no_log == 0 ){
                printf("\033[01;31mrecv id %d\033[00m : %s\n", id, buffer );
            }
            
            
            write_to_log( "--------------------------- recv sync %d   ----------------------------\n%s\n", id,  json_object_to_json_string_ext( json_obj, JSON_C_TO_STRING_PRETTY) );
    
            
            last_result = json_obj;
        
            pthread_mutex_unlock(&copilot_api_result_mutex);
        }else{
            
            write_to_log( "--------------------------- recv async   ----------------------------\n%s\n",  json_object_to_json_string_ext( json_obj, JSON_C_TO_STRING_PRETTY) );
            
            handle_async_msg( json_obj, buffer );      
            
            json_object_put(json_obj);
            
        }
        
        
    }
    
    return NULL;
}



static void process_stopped(GObject *source_object, GAsyncResult *res, gpointer data){
        printf("process_stopped\n");
}

GSubprocess* init_copilot_threads( GeanyPlugin *plugin, char* node_path,  char* engine_path  ){
    
    //pipe( copilot_stdin);
    //pipe( copilot_stdout);
    
    
    utils_mkdir( g_build_filename(plugin->geany_data->app->configdir, "plugins", "copilot", "log", NULL ), TRUE);
    
    jsonrpc_log = open( g_build_filename(plugin->geany_data->app->configdir, "plugins","copilot", "log", "jsonrpc.log", NULL ) , O_RDWR | O_CREAT, S_IRUSR | S_IWUSR );
   
    char start_log[1000];
    sprintf( start_log, "---------------------------- json start ----------------------------\n");
    write( jsonrpc_log, start_log, strlen( start_log ) );
   
    
    /*
    char *start_cmd[] = { node_path , engine_path , NULL};
    

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
    }*/
    
    
    pthread_mutex_lock(&copilot_api_result_mutex);
     
    GSubprocess *process;
	
    
    GError *error = NULL;
    GeanyFiletypeID filetype_id = GEANY_FILETYPES_C;
    
    gint flags = G_SUBPROCESS_FLAGS_STDIN_PIPE | G_SUBPROCESS_FLAGS_STDOUT_PIPE;
    
    gchar ** argv;
    
    char cmd[1000];
    sprintf( cmd, "%s %s", node_path, engine_path );
    
    argv = g_strsplit_set( cmd, " ", -1);
    
    process = g_subprocess_newv((const gchar * const *)argv, flags, &error);
    
    g_subprocess_wait_async( process, NULL, process_stopped, GINT_TO_POINTER(filetype_id));

	input_stream = g_subprocess_get_stdout_pipe( process);
	output_stream = g_subprocess_get_stdin_pipe( process);
	//stream = g_simple_io_stream_new(input_stream, output_stream);
    
    printf("started\n");
    
     pthread_t my_thread; // Thread identifier
    if (pthread_create(&my_thread, NULL, copilot_read_thread, NULL) != 0) {
        fprintf(stderr, "Error creating thread\n");
        exit(EXIT_FAILURE);
    }
    
    
    return process;
    
}
