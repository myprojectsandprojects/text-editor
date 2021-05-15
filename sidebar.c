#include <stdio.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


GtkTreeStore *create_tree_store();
GtkWidget *root_selection;
int root_selection_index; // index of the last item in root-selection. we need to know that because each time user selects a new root by double-clicking on a directory well set the last item to that directory.

GtkWidget *search_results_scroll;
GtkWidget *search_results;
GtkWidget *list;

GtkWidget *tree_view;
char root_dir[100];

enum {
	COLUMN_ICON,
	COLUMN_BASENAME,
	N_COLUMNS
};


void root_selection_changed(GtkComboBoxText *root_selection, gpointer data)
{
	const char *text = gtk_combo_box_text_get_active_text(root_selection);
	if (text == NULL) return;
	g_print("text: %s\n", text);

	sprintf(root_dir, "%s", text);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL(create_tree_store()));
}


void on_selection_changed(GtkTreeSelection *selection, gpointer data)
{
	g_print("selection changed!\n");
}


void prepend_string(char *buffer, const char *str)
{
	char temp[100];

	sprintf(temp, "%s", buffer);
	sprintf(buffer, "%s%s", str, temp);
}


void on_tree_view_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
	g_print("ROW ACTIVATED!\n");

	GtkTreeIter iter;
	char *node_base_name;
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);

	char node_rel_path[100]; node_rel_path[0] = 0;
	char *node_full_path = malloc(100); node_full_path[0] = 0;

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
		printf("\"%s\" is a directory\n", node_full_path);
		sprintf(root_dir, "%s", node_full_path);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL(create_tree_store()));
		printf("removing an item at index %d\n", root_selection_index);
		gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(root_selection), root_selection_index);
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(root_selection), NULL, node_full_path);
		gtk_combo_box_set_active(GTK_COMBO_BOX(root_selection), root_selection_index);
	} else if(S_ISREG(fs_node_info.st_mode)) {
		printf("\"%s\" is a regular file\n", node_full_path);
		//@ cant open a file which is not a text file. well get gtk errors for now
		create_tab(node_full_path); //@ we havent seen the declaration
	} else {
		printf("\"%s\" is an unknown thing\n", node_full_path);
	}
}


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
	g_print("create_nodes_for_directory called!\n");
	--max_depth;

	DIR *dir = opendir(dir_path);
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
			GtkTreeIter this_node = append_node_to_store(store, parent, "folder.png", entry->d_name);
			if (max_depth > 0)
				create_nodes_for_directory(store, &this_node, entry_path, max_depth);
		} else {
			append_node_to_store(store, parent, "file.png", entry->d_name);
		}
	}
	closedir(dir);
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
		g_print("row expanded: child full path: %s\n", child_full_path);

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
			g_print("calling create_nodes_for_directory!\n");
			create_nodes_for_directory(GTK_TREE_STORE(model), &child, child_full_path, 1);
		}
	}
}


GtkTreeStore *create_tree_store()
{
	GtkTreeStore *store = gtk_tree_store_new(N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	create_nodes_for_directory(store, NULL, root_dir, 2);
	return store;
}


GtkWidget *create_tree_view()
{
	//getcwd(root_dir, sizeof(root_dir));
	sprintf(root_dir, "%s", "/home/eero");

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

void on_search_button_clicked(GtkButton *search_button, gpointer data)
{
	GtkSearchEntry *search_entry = (GtkSearchEntry *) data;
	const char *text = gtk_entry_get_text(GTK_ENTRY(search_entry));

	/*
	grep:
		-I -> ignore binary files (?)
		-n -> display line numbers
		-i -> do insensitive search
	*/
	char command[1000]; command[0] = 0;
	sprintf(command, "find %s ! -regex '.*/\\..*' -type f | xargs grep -Ini \"%s\"", root_dir, text);
	printf("command: %s\n", command);
	FILE *fd = popen(command, "r");
	if (fd == NULL) {
		printf("ERROR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		return;
	}

	/*printf("sleeping for 3 secs\n");
	sleep(3);
	printf("done sleeping\n");*/

	long int size = 10000000;
	char *contents = malloc(size+1);
	if (contents == NULL) {
		printf("malloc() failed!!!!!!!!!!!!!!!!!\n");
	}

	long int i = 0;
	while((contents[i] = fgetc(fd)) != -1) {/*printf("%c ", contents[i]);*/ i++;}
	contents[i] = 0;

	printf("Search results:\n");
	printf("%s\n", contents);

	GList *previous_results, *p;
	previous_results = gtk_container_get_children(GTK_CONTAINER(list));
	for (p = previous_results; p != NULL; p = p->next) {
		//printf("previous search result...\n");
		gtk_widget_destroy(p->data);
	}
	g_list_free(previous_results); //@ are we freeing everything?

	char *line;
	char *get_slice_by(char **p_s, char ch); //@ ...
	while ((line = get_slice_by(&contents, '\n')) != NULL) {
		//printf("line: %s\n", line);
		char *file_path = get_slice_by(&line, ':');
		char *line_number = get_slice_by(&line, ':');
		printf("file path: %s\n", file_path);
		printf("line number: %s\n", line_number);
		printf("remainder: %s\n\n", line);

		GtkWidget *search_result = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

		int root_length = strlen(root_dir);
		GtkWidget *file_path_label = gtk_label_new(&file_path[root_length + 1]);
		char line_text[100];
		char *p = line;
		while (*p == ' ' || *p == '\t') ++p;
		sprintf(line_text, "%s:%s", line_number, p);
		//GtkWidget *line_number_label = gtk_label_new(line_number);
		GtkWidget *line_text_label = gtk_label_new(line_text);

		gtk_widget_set_halign(file_path_label, GTK_ALIGN_START);
		//gtk_widget_set_halign(line_number_label, GTK_ALIGN_START);
		gtk_widget_set_halign(line_text_label, GTK_ALIGN_START);

		gtk_container_add(GTK_CONTAINER(search_result), file_path_label);
		//gtk_container_add(GTK_CONTAINER(search_result), line_number_label);
		gtk_container_add(GTK_CONTAINER(search_result), line_text_label);

		gtk_style_context_add_class (gtk_widget_get_style_context(file_path_label), "search-result");

		gtk_list_box_insert(GTK_LIST_BOX(list), search_result, -1);
	}

	gtk_widget_show_all(list);

	pclose(fd);
}

void on_list_row_selected(GtkListBox *list, GtkListBoxRow *row, gpointer data)
{
	printf("on_list_row_selected() called..\n");

	gtk_widget_set_size_request(search_results_scroll, 1000, 300);
}

gboolean on_search_results_scroll_focus()
{
	printf("FOCUS!\n");
	return FALSE;
}


GtkWidget* create_sidebar()
{
	tree_view = create_tree_view();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);
	//gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(tree_view), 9);
	g_signal_connect(tree_view, "row-activated", G_CALLBACK(on_tree_view_row_activated), NULL);
	g_signal_connect(tree_view, "button-press-event", G_CALLBACK(on_tree_view_button_press_event), NULL);
	g_signal_connect(tree_view, "row-expanded", G_CALLBACK(on_tree_view_row_expanded), NULL);
	/*GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	g_signal_connect(selection, "changed", G_CALLBACK(on_selection_changed), NULL);*/

	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_vexpand(scrolled_window, TRUE);

	/*uid_t real_uid = getuid();
	uid_t effective_uid = geteuid();
	printf("real uid: %d, effective uid: %d\n", real_uid, effective_uid);*/

	root_selection = gtk_combo_box_text_new();
	if (getuid() == 0) { // We have root-privileges
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(root_selection), NULL, "/");
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(root_selection), NULL, "/home/eero");
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(root_selection), NULL, "/home/eero");
		root_selection_index = 2;
	} else {
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(root_selection), NULL, "/home/eero");
		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(root_selection), NULL, "/home/eero");
		root_selection_index = 1;
	}
	g_signal_connect(G_OBJECT(root_selection), "changed", G_CALLBACK(root_selection_changed), NULL);
	//gtk_style_context_add_class (gtk_widget_get_style_context(root_selection), "root-selection");
	gtk_combo_box_set_active(GTK_COMBO_BOX(root_selection), 0);

	GtkWidget *notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_BOTTOM);
	GtkWidget *page_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	//gtk_container_add(GTK_CONTAINER(page_container), root_selection);
	gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
	gtk_container_add(GTK_CONTAINER(page_container), scrolled_window);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_container, gtk_label_new("Browse Files"));

	GtkWidget *search_entry = gtk_search_entry_new();
	GtkWidget *search_button = gtk_button_new_with_label("Search");
	g_signal_connect(search_button, "clicked", G_CALLBACK(on_search_button_clicked), search_entry);

	GtkWidget *page_container2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	list = gtk_list_box_new();
	//g_signal_connect(list, "row-selected", G_CALLBACK(on_list_row_selected), NULL);
	//search_results = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	search_results_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_vexpand(search_results_scroll, TRUE);
	//gtk_scrolled_window_set_policy(search_results_scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(page_container2), search_entry);
	gtk_container_add(GTK_CONTAINER(page_container2), search_button);
	gtk_container_add(GTK_CONTAINER(search_results_scroll), list);
	gtk_container_add(GTK_CONTAINER(page_container2), search_results_scroll);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_container2, gtk_label_new("Search in Files"));

	GtkWidget *sidebar_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(sidebar_container), root_selection);
	gtk_container_add(GTK_CONTAINER(sidebar_container), notebook);
	//gtk_container_add(file_browser_container, sidebar_container);

	//gtk_widget_show_all(GTK_WIDGET(file_browser_container));
	gtk_widget_show_all(GTK_WIDGET(sidebar_container));

	return sidebar_container;
}





