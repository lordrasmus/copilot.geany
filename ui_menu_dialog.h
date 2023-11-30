
#ifndef COPILOT_UI_MENU_H
#define COPILOT_UI_MENU_H

void copilot_menu_item_activate_cb(GtkMenuItem *menuitem, gpointer user_data);

void copilot_node_download_dialog(GeanyPlugin *plugin, char* node_version, char* node_path, char* node_url );

#endif
