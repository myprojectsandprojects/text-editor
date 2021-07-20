#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <assert.h>

#include "declarations.h"

extern char root_dir[100];

extern const char *file_icon_path;
extern const char *folder_icon_path;

enum {
	COLUMN_ICON,
	COLUMN_BASENAME,
	N_COLUMNS
};

void on_item1_activate(GtkMenuItem *item, gpointer data)
{
	g_print("menu item1 activate!\n");

	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeView *tree_view = (GtkTreeView *) data;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(tree_view);
	gboolean result = gtk_tree_selection_get_selected(selection, &model, &iter);

	if(result == TRUE) { // there is a selection
		char *name;
		gtk_tree_model_get(model, &iter, COLUMN_BASENAME, &name, -1);
		g_print("menu item 1 with \"%s\"\n", name);
	}
}

gboolean on_tree_view_button_press_event(GtkWidget *tree_view, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_PRESS) {
		GdkEventButton *button_event = (GdkEventButton *) event;
		//g_print("event type: GDK_BUTTON_PRESS\n");
		//g_print("button: %d\n", button_event->button); // left: 1, middle: 2, right: 3

		if(button_event->button == 3) { // right button
			g_print("right-button clicked!\n");
			GtkWidget *menu = gtk_menu_new();
			GtkWidget *item1 = gtk_menu_item_new_with_label("Hello world!");
			gtk_menu_attach(GTK_MENU(menu), item1, 0, 1, 0, 1);
			g_signal_connect(item1, "activate", G_CALLBACK(on_item1_activate), tree_view);
			GtkWidget *item2 = gtk_menu_item_new_with_label("Hello universe!");
			gtk_menu_attach(GTK_MENU(menu), item2, 0, 1, 1, 2);
			GtkWidget *item3 = gtk_menu_item_new_with_label("Hello multiverse!");
			gtk_menu_attach(GTK_MENU(menu), item3, 0, 1, 2, 3);
			gtk_widget_show_all(menu);
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button_event->button, button_event->time);
		}
	} else {
		g_print("event type: unknown\n");
	}
	return FALSE;
}

GtkTreeIter append_node_to_store(GtkTreeStore *store, GtkTreeIter *parent, const char *node_icon, const char *node_text)
{
	GtkTreeIter iter;
	//GdkPixbuf *icon = gdk_pixbuf_new_from_file(node_icon, NULL);
	gtk_tree_store_append(store, &iter, parent);
	if (node_icon != NULL) {
		GdkPixbuf *icon = gdk_pixbuf_new_from_file(node_icon, NULL);
		gtk_tree_store_set(store, &iter, COLUMN_ICON, icon, COLUMN_BASENAME, node_text, -1);
	}
	else
		gtk_tree_store_set(store, &iter, COLUMN_BASENAME, node_text, -1);
	return iter;
}

void create_nodes_for_directory(GtkTreeStore *store, GtkTreeIter *parent, const char *dir_path, int max_depth)
{
	//printf("create_nodes_for_directory()\n");
	--max_depth;

	DIR *dir = opendir(dir_path);
	assert(dir != NULL); // permissions

	char *basenames[1000];
	int i = 0; // points at the end of the list (after last element)
	struct dirent *fs_node;
	while ((fs_node = readdir(dir)) != NULL) {
		assert(i <= 999);

		if (strcmp(fs_node->d_name, ".") == 0 || strcmp(fs_node->d_name, "..") == 0) continue;
		if (fs_node->d_name[0] == '.') continue; // Ignore hidden files/directories

		int len = strlen(fs_node->d_name);
		basenames[i] = malloc(len + 1);
		strcpy(basenames[i], fs_node->d_name);
		i += 1;
		//free(fs_node); // Thats an error
	}
	closedir(dir);

	int compare(const void *a, const void *b) {
		char *str1 = *((char **) a);
		char *str2 = *((char **) b);
		int d;
		int i = 0;
		while (str1[i] != 0 && str2[i] != 0) {
			if ((d = str1[i] - str2[i]) != 0) return d;
			i += 1;
		}
		return 0;
	}

	qsort(basenames, i, sizeof(char *), compare);

	/*int j;
	for (j = 0; j < i; ++j) {
		printf("node basename: %s\n", basenames[j]);
	}*/

	int j;
	for (j = 0; j < i; ++j) {
		char entry_path[1000];
		sprintf(entry_path, "%s/%s", dir_path, basenames[j]);

		struct stat entry_info;
		if (lstat(entry_path, &entry_info) == -1)
			fprintf(stderr, "Failed to lstat \"%s\"\n", basenames[j]); // @error handling
		
		if (S_ISDIR(entry_info.st_mode)) {
			GtkTreeIter this_node = append_node_to_store(store, parent, folder_icon_path, basenames[j]);
			if (max_depth > 0)
				create_nodes_for_directory(store, &this_node, entry_path, max_depth);
		} else {
			append_node_to_store(store, parent, file_icon_path, basenames[j]);
		}
	}

	/*dir = opendir(dir_path);
	assert(dir != NULL); // permissions

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
		//g_print("basename: %s\n", dir_entry->d_name);

		if (entry->d_name[0] == '.') continue; // Ignore hidden files/directories
		
		char entry_path[1000];
		sprintf(entry_path, "%s/%s", dir_path, entry->d_name);

		struct stat entry_info;
		if (lstat(entry_path, &entry_info) == -1)
			fprintf(stderr, "Failed to lstat \"%s\"\n", entry->d_name); // @error handling
		
		if (S_ISDIR(entry_info.st_mode)) {
			GtkTreeIter this_node = append_node_to_store(store, parent, "icons/colors/folder.png", entry->d_name);
			if (max_depth > 0)
				create_nodes_for_directory(store, &this_node, entry_path, max_depth);
		} else {
			append_node_to_store(store, parent, "icons/colors/file.png", entry->d_name);
		}
	}
	closedir(dir);*/
}

GtkTreeStore *create_tree_store()
{
	LOG_MSG("create_tree_store()\n");
	LOG_MSG("create_tree_store(): root_dir: %s\n", root_dir);
	GtkTreeStore *store = gtk_tree_store_new(N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	create_nodes_for_directory(store, NULL, root_dir, 2);
	return store;
}

GtkWidget *create_tree_view()
{
	//getcwd(root_dir, sizeof(root_dir));

	GtkTreeStore *store = create_tree_store();
	GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));


	GtkTreeViewColumn *col = gtk_tree_view_column_new();

	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", COLUMN_ICON, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_BASENAME, NULL);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

	return tree_view;
}

void on_tree_view_row_expanded(GtkTreeView *tree_view, GtkTreeIter *iter, GtkTreePath *path, gpointer data)
{
	g_print("row expanded!\n");

	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);

	gboolean result;
	GtkTreeIter child;
	for (result = gtk_tree_model_iter_children(model, &child, iter);
			result == TRUE; result = gtk_tree_model_iter_next(model, &child)) {

		char *child_name;
		gtk_tree_model_get(model, &child, COLUMN_BASENAME, &child_name, -1);
		//g_print("node: %s\n", child_name);

		GtkTreePath *local_path = gtk_tree_path_copy(path);
		//GtkTreeIter *local_iter = gtk_tree_iter_copy(iter);

		// Get the full path of the child
		char child_full_path[100], ancestors_buffer[100];
		child_full_path[0] = 0; ancestors_buffer[0] = 0;

		do {
			GtkTreeIter local_iter;
			//g_print("path depth: %d\n", gtk_tree_path_get_depth(local_path));
			gtk_tree_model_get_iter(model, &local_iter, local_path);
			char *parent_name;
			gtk_tree_model_get(model, &local_iter, COLUMN_BASENAME, &parent_name, -1);
			char temp_buffer[100];
			strcpy(temp_buffer, ancestors_buffer);
			sprintf(ancestors_buffer, "/%s/%s", parent_name, temp_buffer);
		} while (gtk_tree_path_up(local_path) == TRUE && gtk_tree_path_get_depth(local_path) > 0);

		//g_print("ancestors: %s\n", ancestors_buffer);
		sprintf(child_full_path, "%s/%s/%s", root_dir, ancestors_buffer, child_name);
		//printf("row expanded: child full path: %s\n", child_full_path);

		struct stat entry_info;
		int result = lstat(child_full_path, &entry_info); // lstat doesnt seem to care if we mess up and have triple-duple slashes...
		if (result == -1) g_print("lstat failed!");
		if (S_ISDIR(entry_info.st_mode)) {

			// We might have already done this, so make sure all nodes under "child" are gone. Whatever
			GtkTreeIter grandchild;
			if (gtk_tree_model_iter_children(model, &grandchild, &child) == TRUE) {
				g_print("Suppousedly deleting all nodes under %s\n", child_name);
				while (gtk_tree_store_remove(GTK_TREE_STORE(model), &grandchild) == TRUE);
			}
			create_nodes_for_directory(GTK_TREE_STORE(model), &child, child_full_path, 1);
		}
	}
}

void prepend_string(char *buffer, const char *str)
{
	char temp[100];

	sprintf(temp, "%s", buffer);
	sprintf(buffer, "%s%s", str, temp);
}

void on_tree_view_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
	printf("ROW ACTIVATED!\n");

	GtkTreeIter iter;
	char *node_base_name;
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);

	char node_rel_path[100]; node_rel_path[0] = 0;
	char *node_full_path = malloc(100); node_full_path[0] = 0; //@ free?

	while (gtk_tree_path_get_depth(path) > 0) // 1 -- root node, 0 -- nowhere
	{
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, COLUMN_BASENAME, &node_base_name, -1);
		//printf("node base-name: %s\n", node_base_name);
		prepend_string(node_rel_path, node_base_name);
		prepend_string(node_rel_path, "/");
		gtk_tree_path_up(path);
	}

	sprintf(node_full_path, "%s%s", root_dir, node_rel_path);

	//printf("full path of the activated node: %s\n", node_full_path);

	struct stat fs_node_info;
	if (lstat(node_full_path, &fs_node_info) == -1) {
		printf("Error: Cant lstat \"%s\"!\n", node_full_path);
		return;
	}

	if (S_ISDIR(fs_node_info.st_mode)) {
		//printf("\"%s\" is a directory\n", node_full_path);
		set_root_dir(node_full_path);
	} else if(S_ISREG(fs_node_info.st_mode)) {
		//printf("\"%s\" is a regular file\n", node_full_path);
		//@ cant open a file which is not a text file. well get gtk errors for now
		create_tab(node_full_path);
	} else {
		printf("\"%s\" is an unknown thing\n", node_full_path);
	}
}

GtkWidget *create_file_browser_widget()
{
	LOG_MSG("create_file_browser_widget()\n");

	GtkWidget *tree_view = create_tree_view();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);
	//gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(tree_view), 9);
	g_signal_connect(tree_view, "row-activated", G_CALLBACK(on_tree_view_row_activated), NULL);
	g_signal_connect(tree_view, "row-expanded", G_CALLBACK(on_tree_view_row_expanded), NULL);
	g_signal_connect(tree_view, "button-press-event", G_CALLBACK(on_tree_view_button_press_event), NULL);

	return tree_view;
}


