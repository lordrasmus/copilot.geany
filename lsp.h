#ifndef COPILOT_LSP_H
#define COPILOT_LSP_H

#include "jsonrpc.h"


void send_init_msg( void );
void send_initialized( void );

void send_getVersion( char* version, char* runtimeVersion );
        
void send_setEditorInfo( void );

void send_checkStatus( int* signed_in, char* username, int username_length );

void send_textDocument_didOpen( char* path, char* content );

void send_textDocument_didChange( const char* path, int version, int line, int character, const char* text );

void send_getCompletions( const char* path,int version, int line, int character, int indent_width, char** suggestion );
void send_getPanelCompletions( const char* path, int version, int line, int character, int indent_width,  char** suggestion );

#endif
