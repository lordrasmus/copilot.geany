
#include <stdio.h>
//#include <zlib.h>
#include "miniz.h"
#include <geanyplugin.h>

#include "engines.h"

#include "engine_list.h"


engine_info* get_engines( void ){
    
    return engines;
    
    
}

char* engines_get_path( GeanyPlugin *plugin, engine_info* version ){
    
    return g_build_filename(plugin->geany_data->app->configdir, "plugins","copilot", "engines", version->version, NULL );
    
}

void engines_write_engine_files( GeanyPlugin *plugin, engine_info* version ){

    const gchar *engine_path = g_build_filename(plugin->geany_data->app->configdir,"plugins", "copilot", "engines", NULL );
    utils_mkdir(engine_path, TRUE);
    
    mz_zip_archive zip;
    mz_bool success;

    memset(&zip, 0, sizeof(zip));

    success = mz_zip_reader_init_mem(&zip, version->bin, *version->size, 0);
    if (!success) {
        fprintf(stderr, "Fehler beim Initialisieren des ZIP-Archivs aus dem Speicher.\n");
        return;
    }

    for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip); ++i) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip, i, &file_stat)) {
            fprintf(stderr, "Fehler beim Abrufen von Dateiinformationen im ZIP-Archiv.\n");
            break;
        }

        char filePath[1024];
        snprintf(filePath, sizeof(filePath), "%s/%s", engine_path, file_stat.m_filename);
        
        if ( file_stat.m_is_directory == MZ_TRUE ){
            utils_mkdir(filePath, TRUE);
            continue;
        }

        void *file_data = malloc((size_t)file_stat.m_uncomp_size);
        if (!file_data) {
            fprintf(stderr, "Fehler beim Allozieren von Speicher für die Datei.\n");
            break;
        }

        if (mz_zip_reader_extract_to_mem(&zip, i, file_data, (size_t)file_stat.m_uncomp_size, 0) != MZ_TRUE) {
            fprintf(stderr, "Fehler beim Extrahieren der Datei aus dem ZIP-Archiv.\n");
            free(file_data);
            break;
        }

        FILE *outputFile = fopen(filePath, "wb");
        if (!outputFile) {
            fprintf(stderr, "Fehler beim Öffnen der Ausgabedatei %s.\n", filePath);
            free(file_data);
            break;
        }

        fwrite(file_data, 1, (size_t)file_stat.m_uncomp_size, outputFile);

        fclose(outputFile);
        free(file_data);
    }

    mz_zip_reader_end(&zip);
    
}
