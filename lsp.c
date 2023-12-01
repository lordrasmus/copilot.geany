
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>

#include <sys/types.h>

#include <bsd/string.h>

#include <geanyplugin.h>



#include "lsp.h"


int copilot_signed_in = 0;

/*
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/
 * 
 * 
 */




void send_init_msg( void ){
    
    lock_copilot_api_mutex();
    
    json_object* req = jsonreq_init("initialize");
    
    jsonreq_add_param_str( req, "method",     "initialize");
    jsonreq_add_param_str( req, "trace",      "on");
    jsonreq_add_param_int( req, "processId"   ,getpid());
   
    jsonreq_add_param_null(req, "rootUri");
    jsonreq_add_param_null(req, "rootPath");
    jsonreq_add_param_null(req, "workspaceFolders");
    jsonreq_add_param_obj( req, "capabilities");
    
    // https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#initialize
    
    
    json_object* clientInfo = jsonreq_add_param_obj( req, "clientInfo" );
    jsonreq_add_obj_str( clientInfo, "name",       "Geany");
    jsonreq_add_obj_str( clientInfo, "version",    "2.0.0");
    
    jsonreq_send_sync_and_free( req, 0 );
    
    unlock_copilot_api_mutex();
    
}

void send_getVersion( char* version, char* runtimeVersion ){
    
    lock_copilot_api_mutex();
    
    json_object* ret_val = jsonreq_init("getVersion");
    
    // just to get a params entry in the request
    json_object* dummy = jsonreq_add_param_obj( ret_val, "dummy" );
    dummy = dummy;
    
    jsonreq_send_sync( ret_val, 0 );
    
    strcpy( version , jsonresp_get_string( "version" ) );
    strcpy( runtimeVersion , jsonresp_get_string( "runtimeVersion" ) );
    
    jsonresp_free();
    
    unlock_copilot_api_mutex();
}

void send_setEditorInfo( void ){
    
    #if 0

{   "id":2,
    "jsonrpc":"2.0",
    "params":{
        "editorPluginInfo":{
            "version":"1.11.4",
            "name":"copilot.vim"
        },
        "editorConfiguration":{
            "disabledLanguages":[
                {"languageId":"."},
                {"languageId":"cvs"},
                {"languageId":"gitcommit"},
                {"languageId":"gitrebase"},
                {"languageId":"help"},
                {"languageId":"hgcommit"},
                {"languageId":"markdown"},
                {"languageId":"svn"},
                {"languageId":"yaml"}],
            "enableAutoCompletions":true
        },
        "editorInfo":{
            "version":"0.7.2",
            "name":"Neovim"
        }
    },
    "method":"setEditorInfo"
}

#endif

    lock_copilot_api_mutex();

    json_object* ret_val = jsonreq_init( "setEditorInfo" );
    
    json_object* params = json_object_new_object();  
    json_object_object_add(ret_val,"params",params);
    
    json_object* editorPluginInfo = jsonreq_add_param_obj( ret_val, "editorPluginInfo" );
    jsonreq_add_obj_str( editorPluginInfo, "name","GeanyCopilot" );
    jsonreq_add_obj_str( editorPluginInfo, "version","0.0.1");
    
    
    json_object* editorInfo = jsonreq_add_param_obj( ret_val, "editorInfo" );
    jsonreq_add_obj_str( editorInfo,"name", "Geany" );
    jsonreq_add_obj_str( editorInfo,"version","2.0.0");
    
    
    json_object* editorConfiguration = jsonreq_add_param_obj( ret_val, "editorConfiguration" );
    jsonreq_add_obj_bool( editorConfiguration, "enableAutoCompletions", 1);
    
    json_object* disabledLanguages = json_object_new_array();  
    json_object_object_add(editorConfiguration,"disabledLanguages",disabledLanguages);
    
    json_object* languageId = json_object_new_object();  
    json_object_array_add(disabledLanguages,languageId );
    json_object_object_add(languageId,"languageId",json_object_new_string("."));
    
    languageId = json_object_new_object();  
    json_object_array_add(disabledLanguages,languageId );
    json_object_object_add(languageId,"languageId",json_object_new_string("cvs"));
    
    
    
    jsonreq_send_sync_and_free( ret_val, 1 );
    
    unlock_copilot_api_mutex();

}


void send_getCompletions( const char* path, int version, int line, int character, int indent_width,  char** suggestion, int* start_line, int* start_char ){
    #if 0
    
    

{   "id":3,
    "jsonrpc":"2.0",
    "params":{
        "position":{
            "line":16,
            "character":6
            },
        "doc":{
            "uri":"file:///home/ramin/test.c",
            "version":0,
            "indentSize":8,
            "tabSize":8,
            "insertSpaces":false,
            "relativePath":"test.c",
            "position":{
                "line":16,
                "character":6
                }
            },
        "textDocument":{
            "uri":"file:///home/ramin/test.c","relativePath":"test.c",
            "version":0
        }
    },
    "method":"getCompletions"
}

{
  "method": "getCompletions",
  "params": {
    "position": {
      "line": 5,
      "character": 7
    },
    "doc": {
      "line": 3,
      "version": 0,
      "indentSize": 8,
      "tabSize": 8,
      "insertSpaces": false,
      "uri": "file:///home/ramin/test.c",
      "relativePath": "/home/ramin/test.c",
      "position": {
        "line": 5,
        "character": 7
      }
    }
  },
  "jsonrpc": "2.0",
  "id": 2
}


#endif

    lock_copilot_api_mutex();

    json_object* ret_val = jsonreq_init("getCompletions");
    
    
    json_object* position = jsonreq_add_param_obj( ret_val, "position" );
    jsonreq_add_obj_int( position, "line"     , line);
    jsonreq_add_obj_int( position, "character", character );
    
    
    json_object* doc = jsonreq_add_param_obj( ret_val, "doc" );
    jsonreq_add_obj_int( doc,"line",          line);
    jsonreq_add_obj_int( doc,"version",       version);
    jsonreq_add_obj_int( doc,"indentSize",    indent_width);
    jsonreq_add_obj_int( doc,"tabSize",       4);
    jsonreq_add_obj_bool(doc,"insertSpaces",  0);
    jsonreq_add_obj_str( doc, "uri",          "file://%s", path);
    jsonreq_add_obj_str( doc, "relativePath", path );
    
    position = json_object_new_object();  
    json_object_object_add(doc,"position",position);
    
    jsonreq_add_obj_int( position,"line",      line );
    jsonreq_add_obj_int( position,"character", character);
    
    
    
    json_object* textDocument = jsonreq_add_param_obj( ret_val, "textDocument" );
    jsonreq_add_obj_str(textDocument,"uri",   "file://%s", path);
    jsonreq_add_obj_int(textDocument,"version", version);
    
    
    jsonreq_send_sync( ret_val, 0 );
    
    *suggestion = 0;
    
    char uuid[100];
    
    struct json_object *result_obj;
    if ( json_object_object_get_ex(get_last_result(), "result", &result_obj) != 0 ){
        
        struct json_object *completions_obj;
        if ( json_object_object_get_ex(result_obj, "completions", &completions_obj) != 0 ){
            
            int array_length = json_object_array_length(completions_obj);
            
            for (int i = 0; i < array_length; ++i) {
                
                struct json_object *array_element = json_object_array_get_idx(completions_obj, i);
                
                struct json_object *uuid_obj;
                if ( json_object_object_get_ex(array_element, "uuid", &uuid_obj) != 0 ){
                    
                    strcpy( uuid , json_object_get_string(uuid_obj));
                }
                
                struct json_object *text_obj;
                if ( json_object_object_get_ex(array_element, "text", &text_obj) != 0 ){
                    
                    const char* p = json_object_get_string(text_obj);
                    
                    *suggestion = malloc( strlen( p ) + 1 );
                    strcpy( *suggestion, p );
                    
                }
                
                struct json_object *range_obj;
                if ( json_object_object_get_ex(array_element, "range", &range_obj) != 0 ){
                    
                    struct json_object *start_obj;
                    if ( json_object_object_get_ex(range_obj, "start", &start_obj) != 0 ){
                        
                        struct json_object *line_obj;
                        if ( json_object_object_get_ex(start_obj, "line", &line_obj) != 0 ){
                            *start_line = json_object_get_int( line_obj );
                        }   
                        
                        struct json_object *character_obj;
                        if ( json_object_object_get_ex(start_obj, "character", &character_obj) != 0 ){
                            *start_char = json_object_get_int( character_obj );
                        }   
                    
                    }   
                }
                
            }
        }
    }
    
    json_object_put(get_last_result());
    
    
    #if 0
    {"params":{"uuid":"484a83b0-6b16-43e0-8eca-44129eb6d386"},"method":"notifyShown","jsonrpc":"2.0","id":10}
    #endif
    
    
    if ( *suggestion != 0 ){
    
        ret_val = jsonreq_init("notifyShown");
        
        jsonreq_add_param_str( ret_val, "uuid", uuid );
        
        jsonreq_send_sync_and_free( ret_val, 0 );
    }
    
    
    unlock_copilot_api_mutex();

}

void send_getPanelCompletions( const char* path, int version, int line, int character, int indent_width,  char** suggestion ){
    #if 0
     {"params":{"doc":{"position":{"line":0,"character":19},"uri":"file:\\/\\/\\/home\\/ramin\\/test.c","insertSpaces":false,"indentSize":8,"tabSize":8,"version":0,"relativePath":"test.c"},"panelId":"copilot:\\/\\/\\/1","position":{"line":0,"character":19},"textDocument":{"version":0,"uri":"file:\\/\\/\\/home\\/ramin\\/test.c","relativePath":"test.c"}},"method":"getPanelCompletions","jsonrpc":"2.0","id":5}
     
     {
      "params": {
        "doc": {
          "position": {
            "line": 0,
            "character": 19
          },
          "uri": "file:\\/\\/\\/home\\/ramin\\/test.c",
          "insertSpaces": false,
          "indentSize": 8,
          "tabSize": 8,
          "version": 0,
          "relativePath": "test.c"
        },
        "panelId": "copilot:\\/\\/\\/1",
        "position": {
          "line": 0,
          "character": 19
        },
        "textDocument": {
          "version": 0,
          "uri": "file:\\/\\/\\/home\\/ramin\\/test.c",
          "relativePath": "test.c"
        }
      },
      "method": "getPanelCompletions",
      "jsonrpc": "2.0",
      "id": 5
    }

     #endif
    
}

void send_textDocument_didOpen( char* path, char* content ){
    
    
    json_object* ret_val = jsonreq_init( "textDocument/didOpen" );
    
    json_object* textDocument = jsonreq_add_param_obj( ret_val, "textDocument" );
    jsonreq_add_obj_str( textDocument,"uri",        "file://%s", path);
    jsonreq_add_obj_int( textDocument,"version",    0);
    jsonreq_add_obj_str( textDocument,"languageId", "c");
    jsonreq_add_obj_str( textDocument,"text",       "%s", content );

    
    jsonreq_send_async( ret_val, 1 );
    
}

void send_textDocument_didChange( const char* path, int version, int line, int character, const char* text ){
    #if 0
        {
            "params":{
                "contentChanges":[
                    {   "rangeLength":0,
                        "range":{
                            "start":{"line":3,"character":11},
                            "end":{"line":3,"character":11}
                            },
                        "text":"\\n\\t"
                    }
                ],
                "textDocument":{
                    "uri":"file:\\/\\/\\/home\\/ramin\\/test.c",
                    "version":6
                }
            },
            "method":"textDocument\\/didChange",
            "jsonrpc":"2.0"
        }
    #endif
    
    
    json_object* ret_val = jsonreq_init( "textDocument/didChange" );
    
    
    json_object* textDocument = jsonreq_add_param_obj( ret_val, "textDocument" );
    
    jsonreq_add_obj_str( textDocument,"uri",        "file://%s", path);
    jsonreq_add_obj_int( textDocument,"version",    version );
    
    
    json_object* contentChanges = jsonreq_add_param_array( ret_val, "contentChanges" );
    
    json_object* contentChange = json_object_new_object();
    json_object_array_add( contentChanges, contentChange );
    
    
    // man kann auch einfach das ganze dokument senden statt der änderungen
    
    /*json_object* range = json_object_new_object();
    json_object_object_add(contentChange,"range",range);
    
    json_object* start = json_object_new_object();
    json_object_object_add(range,"start",start);
    json_object_object_add(start,"line",json_object_new_int(line));
    json_object_object_add(start,"character",json_object_new_int(character));
    
    json_object* end = json_object_new_object();
    json_object_object_add(range,"end",end);
    json_object_object_add(end,"line",json_object_new_int(line));
    json_object_object_add(end,"character",json_object_new_int(character));
    */
    
    json_object_object_add(contentChange,"text",json_object_new_string(text));
    
    
    
    jsonreq_send_async( ret_val, 1);
}

void send_initialized( ){
    #if 0
   {"params":{},"method":"initialized","jsonrpc":"2.0"}
   #endif
   
   
   json_object* ret_val = jsonreq_init( "initialized" );
       
   json_object* params = json_object_new_object();
   json_object_object_add(ret_val,"params",params);
    
   jsonreq_send_async( ret_val, 0 );
}

void send_checkStatus( int* signed_in, char* username, int username_length ){
    #if 0
    {"params":{"options":{"localChecksOnly":true}},"method":"checkStatus","jsonrpc":"2.0","id":3}
    #endif
    
    lock_copilot_api_mutex();
    
    
    username[0] = 0;
    
    
    json_object* ret_val = json_object_new_object();    
    json_object_object_add(ret_val,"method",json_object_new_string("checkStatus"));
    
    json_object* params = json_object_new_object();
    json_object_object_add(ret_val,"params",params);
    
    json_object* options = json_object_new_object();
    json_object_object_add(params,"options",options);
    
    // mit der Option kommt MaybeOK. warscheinlich braucht man das aber garnicht denk ich
    //json_object_object_add(options,"localChecksOnly",json_object_new_boolean(1));
    
    jsonreq_send_sync( ret_val, 0 );
    
    
    copilot_signed_in = 0;
    
    const char *status = jsonresp_get_string( "status" );
    if ( 0 == strcmp( status , "NotSignedIn" ) ){
        copilot_signed_in = 0;
    }
    
    if ( 0 == strcmp( status , "MaybeOK" ) ){
        copilot_signed_in = 1;
    }
    
    if ( 0 == strcmp( status , "OK" ) ){
        copilot_signed_in = 1;
    }
    
    strlcpy( username, jsonresp_get_string( "user" ) ,username_length );
    
    *signed_in = copilot_signed_in;
    
    
    json_object_put(get_last_result());
    
    unlock_copilot_api_mutex();
    
}

void send_signInInitiate(){
    #if 0
    {"params":{},"method":"signInInitiate","jsonrpc":"2.0","id":5}
    #endif
    
    lock_copilot_api_mutex();
    
    
    
    json_object* ret_val = jsonreq_init( "signInInitiate" );
    
    json_object* params = json_object_new_object();
    json_object_object_add(ret_val,"params",params);
    
    jsonreq_send_sync( ret_val, 0);
    
    
    const char *status = jsonresp_get_string(  "status" );
    printf("Status: %s\n", status);
    
    
    const char *userCode = jsonresp_get_string( "userCode" );
    printf("userCode: %s\n", userCode);
    
    
    const char *verificationUri = jsonresp_get_string( "verificationUri" );
    printf("verificationUri: %s\n", verificationUri);
        
        
    if ( 0 == strcmp( "AlreadySignedIn", status ) ){
        
        jsonresp_free();
        unlock_copilot_api_mutex();
        return;
    }
    
    if ( userCode == 0 ){
        printf("something went wrong with signInInitiate.  already signed in ?? \n");
        
        jsonresp_free();
        unlock_copilot_api_mutex();
        return;
    }
    
    
    #if 0
    {"params":{"userCode":"70B1-4746"},"method":"signInConfirm","jsonrpc":"2.0","id":6}
    #endif
    
    
    ret_val = jsonreq_init( "signInConfirm" );
    
    jsonreq_add_param_str( ret_val, "userCode", userCode );
    
    
    // das gehört zum vorherigen request. erst hier freigeben weil wir userCode noch brauchen
    jsonresp_free( );
    
    
    jsonreq_send_sync_and_free( ret_val,  0 );
    
    
    unlock_copilot_api_mutex();
}





