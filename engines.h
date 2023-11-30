#ifndef COPILOT_ENGINES_H
#define COPILOT_ENGINES_H


typedef struct{
  char* version;
  unsigned char* bin;
  unsigned int *size;
}engine_info;


engine_info* get_engines( void );

void engines_write_engine_files( GeanyPlugin *plugin, engine_info* version );
char* engines_get_path( GeanyPlugin *plugin, engine_info* version );

#endif

