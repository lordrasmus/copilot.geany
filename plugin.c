
#include <geanyplugin.h>

#include "lsp.h"
#include "engines.h"
#include "ui_menu_dialog.h"

static GtkWidget *main_menu_item;

static pid_t node_pid;


static GeanyPlugin *plugin_me;

/*
 * URLs : "https://github.com/login/device?userCode=3E52-FFFF"
 * 
 * https://www.scintilla.org/ScintillaDoc.html
 * 
 */
 
static void getCompletions_func();




static void on_document_open(GObject *obj, GeanyDocument *doc, gpointer user_data)
{
    
    GeanyPlugin *plugin = user_data;
     
    gint   len  = sci_get_length(doc->editor->sci) + 1;
    gchar* text = sci_get_contents(doc->editor->sci, len);
        
    plugin_set_document_data( plugin , doc, "lsp_version", 0 );
    
    send_textDocument_didOpen( DOC_FILENAME(doc) , text );
    
}


static int last_complitition_pos = -1;

static void getCompletions_func(){


    GeanyPlugin *plugin = plugin_me;

    GeanyDocument *doc = document_get_current();
    GeanyEditor *editor = doc->editor;
    const GeanyIndentPrefs *indents = editor_get_indent_prefs( doc->editor );
    
    int line = sci_get_current_line( editor->sci );
    int pos =  sci_get_current_position( editor->sci );
    int line_pos = sci_get_col_from_position( editor->sci, pos );
    
    if ( last_complitition_pos == pos ){
        return;
    }
    
    long int version = (long)plugin_get_document_data( plugin , editor->document, "lsp_version");
        
    char *suggestion = 0;
    int start_line;
    int start_char;
    
    send_getCompletions( DOC_FILENAME(editor->document), version, line , line_pos, indents->width , &suggestion, &start_line, &start_char );
    
    last_complitition_pos = pos;
    
    if ( suggestion != 0 ){
        
        int suggestion_len = strlen( suggestion );
        int suggestion_add =0;
        
        printf("    suggestion   %d    : >%s<\n", suggestion_len , suggestion );
        printf("    start_line: %d\n", start_line);
        printf("    start_char: %d\n", start_char);
        
        printf("    line    : %d\n", line);
        printf("    pos     : %d\n", pos);
        printf("    line_pos: %d\n", line_pos);
        
        
        gint   len  = sci_get_length(doc->editor->sci) + 1;
        gchar* text = sci_get_contents(doc->editor->sci, len);
    
        char *p = suggestion;
        // sometimes the start of the suggestion is not the start of the line in the editor ( mostly 1 char ?? )
        char *cur = &text[ pos - line_pos - 1 ];
        
        //printf(" cur text : >%s\n", cur );
        
        if ( start_char == 0 ){
            
            // this should find the first char in the current line which matches the suggestion
            for ( int i = 0; i < 5 ; i++ ){
                if ( *suggestion == cur[i] ){
                    cur = &cur[i];
                    printf( "   fixed cur text in editor position by %d bytes\n", i);
                    break;
                }
            }
            
            for( int i = 0 ; suggestion[i] == cur[i] ; i++ ){
                p++;
            }
        }
        
        suggestion_len = strlen( p );
        
        
        // kleiner hack bis ich rausgefunden habe warum copilot da immer 2 tabs am anfang macht
        /*while( 1 ){
            if ( *p == ' ' ){ p++ ; continue; }
            if ( *p == '\t' ){ p++ ; continue; }
            break;
        }*/
        
        sci_start_undo_action( editor->sci );
        editor_insert_text_block( editor, p, pos, 0, -1, 0 );
        //editor_insert_snippet( editor, pos, suggestion );
        //editor_insert_snippet( editor, pos, suggestion );
        sci_end_undo_action( editor->sci );
        
        // kleiner hack weil der text select pro zeile um 1 zeichen zu kurz ist
        for ( int i = 0 ; i < suggestion_len; i++ ){
            if ( suggestion[i] == '\n' ){
                suggestion_add++;
            }
        } 
        
        #define SSM(s, m, w, l) scintilla_send_message(s, m, w, l)
        SSM( editor->sci, SCI_SETSEL, pos + suggestion_len + suggestion_add , pos);
        
    
        free( suggestion );
    }else{
         msgwin_status_add("Copilot: no completition offered");
    }
    
    //send_getPanelCompletions( DOC_FILENAME(editor->document), version, line , line_pos, indents->width , &suggestion );
    
   

}

static int copilot_completition_marker = -1;
int copilot_completition_run( void *){

    
    if( copilot_completition_marker == -1 ) return 1;
        
    if ( copilot_completition_marker-- <= 0 ){
        copilot_completition_marker = -1;
        getCompletions_func();
    }

    return 1;
}


static void kb_run_completition(G_GNUC_UNUSED guint key_id){
     msgwin_status_add("Copilot: requesting copilot completition");
     copilot_completition_marker = 0;
     last_complitition_pos = -1;
     
}


static gboolean on_editor_notify(GObject *object, GeanyEditor *editor, SCNotification *nt, gpointer data){
    //printf("on_editor_notify\n");
    
    
    GeanyPlugin *plugin = data;
    
     switch (nt->nmhdr.code)
        {
            case SCN_UPDATEUI:
                return FALSE;
                
            case SCN_CHARADDED:
                #if 0
                printf("SCN_CHARADDED\n");
                /* For demonstrating purposes simply print the typed character in the status bar */
                //ui_set_statusbar(FALSE, _("Typed character: %c"), nt->ch);
                
                int line = sci_get_current_line( editor->sci );
                int pos2 = sci_get_col_from_position( editor->sci, sci_get_current_position( editor->sci ) );
                
                printf("line: %d  pos: %d\n", line, pos2 );
                
                gint   len  = sci_get_length( editor->sci) + 1;
                gchar* text = sci_get_contents( editor->sci, len);
                
                printf("new text: %s", text );
                #endif
                break;
            
            case SCN_MODIFIED:
                 //printf("SCN_MODIFIED\n");
                 
                int modificationType = nt->modificationType;
                
                if (modificationType & SC_MOD_INSERTTEXT){
                    //printf("   SC_MOD_INSERTTEXT\n");
                    
                    modificationType &= ~SC_MOD_INSERTTEXT;
                }
                
                if (modificationType & SC_MOD_DELETETEXT){
                   // printf("   SC_MOD_DELETETEXT\n");
                    
                    modificationType &= ~SC_MOD_DELETETEXT;
                }
                
                if ( modificationType != 0 ){
                    //printf("   SCN_MODIFIED unhandled mod : 0x%X\n",  modificationType );
                }
                
                int line = sci_get_current_line( editor->sci );
                int pos =  sci_get_current_position( editor->sci );
                int line_pos = sci_get_col_from_position( editor->sci, pos );
                
                #if 0
                printf("    editor line     : %d\n", line );
                printf("    editor pos      : %d\n", pos );
                printf("    editor line_pos : %d\n", line_pos );
                
                printf("    position   : %ld\n", nt->position );
                printf("    length     : %ld\n", nt->length );
                printf("    linesAdded : %ld\n", nt->linesAdded );
                printf("    line       : %ld\n", nt->line );
                #endif
                
                if (nt->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)){
                
                    char *mod_text = malloc( nt->length +1 );
                    memcpy( mod_text, nt->text, nt->length);
                    mod_text[nt->length] = 0;
                    
                    //printf("    text       : %s\n", mod_text );
                    
                    free( mod_text );
                    
                    gint   len  = sci_get_length( editor->sci) + 1;
                    gchar* text = sci_get_contents( editor->sci, len);
                    
                    long int version = (long)plugin_get_document_data( plugin , editor->document, "lsp_version");
                    version++;
                    
                    plugin_set_document_data( plugin , editor->document, "lsp_version", (gpointer*)version );
                    
                    //printf("version : %ld\n", version );
                    
                    //send_textDocument_didChange( DOC_FILENAME(editor->document), version, line, line_pos, mod_text );
                    
                    send_textDocument_didChange( DOC_FILENAME(editor->document), version, line, line_pos, text );
                    
                    copilot_completition_marker = 15;
                    
                   
                }
                
                
                
                break;
        }
        
    return FALSE;
}
 
static gboolean copilot_init(GeanyPlugin *plugin, gpointer pdata)
{
    msgwin_status_add("Copilot: Hello World from copilot plugin!");
 
 
    
    
    // Create a new menu item and show it
    main_menu_item = gtk_menu_item_new_with_mnemonic("Copilot");
    gtk_widget_show(main_menu_item);
 
    // Attach the new menu item to the Tools menu
    gtk_container_add(GTK_CONTAINER(plugin->geany_data->main_widgets->tools_menu), main_menu_item);
 
    // Connect the menu item with a callback function
    // which is called when the item is clicked
    g_signal_connect(main_menu_item, "activate", G_CALLBACK(copilot_menu_item_activate_cb), plugin);
    
    geany_plugin_set_data(plugin, plugin, NULL);
 
    
    char *node_version = "node-v20.10.0-linux-x64";
    char *node_file    = "node-v20.10.0-linux-x64.tar.xz";
    char *node_url     = "https://nodejs.org/download/release/v20.10.0/node-v20.10.0-linux-x64.tar.xz";
    
    char* node_path = g_build_filename(plugin->geany_data->app->configdir, "plugins", "copilot", "node" , NULL );
    char* node_bin  = g_build_filename(plugin->geany_data->app->configdir, "plugins", "copilot", "node" , node_version, "bin", "node", NULL );
    
    printf("Copilot node path : %s\n", node_path );
    if ( 0 != access( node_bin, R_OK ) ){    
        copilot_node_download_dialog( plugin, node_file, node_path, node_url );
    }
 
    
     
    engine_info* engines = get_engines( );
    
    printf("Copilot engine list : \n");
    
    for ( int i = 0; engines[i].version != 0 ; i++ ){
        printf("  %s\n", engines[i].version );
    }
    
    engine_info* selectet_engine = &engines[0];
    
    printf("Copilot engine used : %s\n", selectet_engine->version);
    msgwin_msg_add(COLOR_BLACK, -1, NULL,"Copilot: init engine %s", selectet_engine->version  );
    
    
    printf("Copilot engine extracting ... \n");fflush(stdout);
    engines_write_engine_files( plugin, selectet_engine );
    printf("Copilot engine extracted\n");fflush(stdout);
 
 
    char engine_path[1024];
    snprintf( engine_path, sizeof( engine_path ), "%s/index.js", engines_get_path( plugin, selectet_engine ));
    
    
    node_pid = init_copilot_threads( plugin, node_bin, engine_path );
    
    
    send_init_msg();
    
    send_setEditorInfo();
    
    send_initialized();
    
    
    
    return TRUE;
}


static PluginCallback copilot_callbacks[] =
{
	/* Set 'after' (third field) to TRUE to run the callback @a after the default handler.
	 * If 'after' is FALSE, the callback is run @a before the default handler, so the plugin
	 * can prevent Geany from processing the notification. Use this with care. */
    { "document-open", (GCallback) &on_document_open, FALSE, NULL },
	{ "editor-notify", (GCallback) &on_editor_notify, FALSE, NULL },
	{ NULL, NULL, FALSE, NULL }
};
 
static void copilot_cleanup(GeanyPlugin *plugin, gpointer pdata)
{
    
    
    GtkWidget *main_menu_item = (GtkWidget *) pdata;
    gtk_widget_destroy(main_menu_item);
    
        
    printf("Bye Copilot :-(\n");
        
    kill(node_pid, SIGTERM);
    wait(NULL); 
    
}


 
G_MODULE_EXPORT
void geany_load_module(GeanyPlugin *plugin)
{
    
    
   
   
    
    
    plugin_me = plugin;
    
    /* Step 1: Set metadata */
    plugin->info->name = "Copilot";
    plugin->info->description = "Github Copilot Geany Plugin";
    plugin->info->version = "0.1";
    plugin->info->author = "Ramin Moussavi <lordrasmus@gmail.com>";
 
    /* Step 2: Set functions */
    plugin->funcs->init = copilot_init;
    plugin->funcs->cleanup = copilot_cleanup;
    plugin->funcs->callbacks = copilot_callbacks;

    GeanyKeyGroup *key_group = plugin_set_key_group(plugin, "copilot", 1, NULL);
    keybindings_set_item(key_group, 0, kb_run_completition, 0, 0, "copilot run completition", _("Run the Copilot completition"),   main_menu_item);
 
    /* Step 3: Register! */
    GEANY_PLUGIN_REGISTER(plugin, 225);
    /* alternatively:
    GEANY_PLUGIN_REGISTER_FULL(plugin, 225, data, free_func); */
    
    plugin_timeout_add( plugin, 100, copilot_completition_run, 0 );
    

}



 
