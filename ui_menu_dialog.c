
#include <geanyplugin.h>
#include <curl/curl.h>

#include "jsonrpc.h"
#include "lsp.h"


    
void copilot_menu_item_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    
    #if 1
    char version[100] = { 0 };
    char runtimeVersion[100] = { 0 };
    char username[100] = { 0 };
    int signed_in = 0;
    
    
    if ( copilot_engine_running() == 1 ){
    
        send_checkStatus( &signed_in, username, sizeof( username ) );
    
        send_getVersion( version, runtimeVersion );
    }
    
    char show_text[1000];
    
    int off = 0;
    
    off += snprintf( &show_text[off], sizeof(show_text) - off  , "Copilot Info\n\n");
    
    
    
    
    off += snprintf( &show_text[off], sizeof(show_text) - off  , "Signed in   : ");
    
    if ( signed_in == 1 ){
        off += snprintf( &show_text[off], sizeof(show_text) - off  , "Yes\n");
    }else{
        off += snprintf( &show_text[off], sizeof(show_text) - off  , "No\n");
    }
    
    off += snprintf( &show_text[off], sizeof(show_text) - off  , "Username : %s\n", username);
    off += snprintf( &show_text[off], sizeof(show_text) - off  , "Version     : %s\n", version);
    off += snprintf( &show_text[off], sizeof(show_text) - off  , "Runtime    : %s\n", runtimeVersion);
        
    
    dialogs_show_msgbox(GTK_MESSAGE_INFO,"%s", show_text);
    
    #else
    
    char *welcome_text = "Copilot Info";
    
    GtkWidget *dialog;
	GeanyPlugin *plugin = user_data;
	GeanyData *geany_data = plugin->geany_data;

	dialog = gtk_message_dialog_new( GTK_WINDOW(geany_data->main_widgets->window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		"%s", welcome_text);
        
    gtk_window_set_title(GTK_WINDOW(dialog), "Copilot Infos");
    
     // Fortschrittsanzeige erstellen
    GtkWidget *progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0); // Initialer Fortschritt

    // Dialog-Content hinzufügen
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content_area), progress_bar);

    // Callback für Dialogantworten hinzufügen
    //g_signal_connect(dialog, "response", G_CALLBACK(dialog_response_callback), NULL);

    // Timer für Fortschrittsaktualisierung
    //gdouble progress = 0.0;
    //GtkWidget *timer = g_timeout_add(100, (GSourceFunc)gtk_progress_bar_pulse, progress_bar);
    
    plugin_timeout_add( plugin, 100, copilot_progress_run, progress_bar );


        
	//gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), _("(From the %s plugin)"), plugin->info->name);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
    
    #endif
    
}




typedef struct {
    GtkWidget *progress_bar;
    GtkWidget *dialog;
    GtkWidget *label;
    gchar* file_name;
    gchar* node_path;
    //FILE *file;
} DownloadData;

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    DownloadData *data = (DownloadData *)clientp;
    GtkWidget *progress_bar = data->progress_bar;
    GtkWidget *label = data->label;

    gdouble fraction = 0;

    if (dltotal > 0) {
        fraction = dlnow / dltotal;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), fraction);
        
        gchar *text = g_strdup_printf("Download %s : %.0f%%",data->file_name,  fraction * 100);
        gtk_label_set_text(GTK_LABEL(label), text);
        g_free(text);   

        // Aktualisieren Sie die GTK-Anzeige
        while (gtk_events_pending()) {
            gtk_main_iteration();
        }
    }
    
    

    return 0;
}

gboolean download_file(const gchar *url,  DownloadData *download_data) {
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        
        char* output_file = g_build_filename( download_data->node_path, download_data->file_name , NULL );
        
        FILE *file = fopen(output_file, "wb");

        if (file) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
            curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, download_data);

            res = curl_easy_perform(curl);

            fclose(file);

            if (res != CURLE_OK) {
                g_printerr("Download error: %s\n", curl_easy_strerror(res));
            }
            
            
            progress_callback( download_data, 1.0, 1.0, 0, 0 );
            
            

            curl_easy_cleanup(curl);
            return TRUE;
        }
    }

    return FALSE;
}

void copilot_node_download_dialog(GeanyPlugin *plugin , char* node_file, char* node_path, char* node_url){
    
    
    GtkWidget *window = plugin->geany_data->main_widgets->window;

    
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Copilot download node ",
                                                    GTK_WINDOW(window),
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    0,
                                                    GTK_RESPONSE_NONE,
                                                    NULL);
    
    // Größe des Dialogs auf 300 x 200 Pixel setzen
    gtk_widget_set_size_request(dialog, 500, 50);
    
    // Setzen Sie die Position des Dialogs auf die Mitte des Bildschirms
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    
    const gchar *css_data = "progress, trough {  min-height: 30px;}";
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css_data, -1, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_widget_get_style_context(content_area);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                                              


    // Fortschrittsanzeige erstellen
    GtkWidget *progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0); // Initialer Fortschritt
    
    // GtkLabel für den Text über der Fortschrittsleiste hinzufügen
    GtkWidget *label = gtk_label_new("Fortschritt: 0%");
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);         // Packen Sie das Label oben hinzu
    gtk_box_pack_start(GTK_BOX(box), progress_bar, FALSE, FALSE, 0);  // Packen Sie die Fortschrittsleiste darunter hinzu
    gtk_container_add(GTK_CONTAINER(content_area), box);
    
    
    gtk_widget_show_all(dialog);
    
    
    utils_mkdir( node_path, TRUE );
    DownloadData download_data = {progress_bar, dialog, label, node_file, node_path};
    
    download_file(node_url, &download_data);
    
    gchar *text = g_strdup_printf("Extract %s ",node_file );
    gtk_label_set_text(GTK_LABEL(label), text);
    g_free(text);  
    
    gtk_widget_queue_draw(dialog);  // Erzwingt das Zeichnen der Oberfläche

    // Aktualisieren Sie die GTK-Anzeige
    while (gtk_events_pending()) {
           gtk_main_iteration();
    }
    sleep(1);
    char cmd[2000];
    sprintf( cmd, "tar -xaf %s/%s -C %s", node_path, node_file, node_path);
    //printf("cmd : %s\n", cmd );
    system( cmd );
    
    //GError *error = NULL;
    //g_spawn_command_line_async(cmd, &error);
    

    gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_NONE);
    
    // Dialog zerstören
    gtk_widget_destroy(dialog);
    
    
}

