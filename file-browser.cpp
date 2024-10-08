
/*
@ How would a user create a file/folder in "/"?
@ If a user renames a folder, paths of all files and folders contained by that folder will change.. and we dont do that..
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <assert.h>

#include "declarations.h"

GtkWidget *filebrowser;

extern char root_dir[100];

extern const char *file_icon_path;
extern const char *folder_icon_path;

/* filebrowser's model/store columns: */
enum {
	COLUMN_ICON,
	COLUMN_BASENAME,
	COLUMN_FULL_PATH,
	COLUMN_IS_DIR,
	COLUMN_IS_VISITED,
	COLUMN_IS_EDITABLE,
	N_COLUMNS
};


/*
static GtkTreeIter append_node_to_store(GtkTreeStore *store,
												GtkTreeIter *parent,
												const char *icon_path,
												const char *node_basename,
												const char *node_full_path)
{
	//GdkPixbuf *icon = gdk_pixbuf_new_from_file(node_icon, NULL);
	GdkPixbuf *icon = NULL;
	if (icon_path != NULL) {
		icon = gdk_pixbuf_new_from_file(icon_path, NULL); //@ fail?
	}

	GtkTreeIter iter;
	gtk_tree_store_append(store, &iter, parent); // create empty row
	gtk_tree_store_set(store, &iter, // fill in values
		COLUMN_ICON, icon,
		COLUMN_BASENAME, node_basename,
		COLUMN_FULL_PATH, node_full_path,
		COLUMN_IS_EDITABLE, FALSE,
		-1);

	return iter; //@ this is kind of dubious enterprise we're involved in here..?
}
*/

static int compare(const void *a, const void *b)
{
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

void create_nodes_for_dir(GtkTreeStore *store, GtkTreeIter *parent, const char *dir_path, int max_depth)
{
	LOG_MSG("create_nodes_for_dir(): \"%s\"\n", dir_path);

	--max_depth;

	DIR *dir = opendir(dir_path);
	/*
	if (dir == NULL) {
		printf("create_nodes_for_dir(): opendir() error: errno: %d\n", errno);
	}
	*/
	assert(dir != NULL); //@ permissions, Windows shared directory that has disappeared

	char *basenames[1000];
	int i = 0; // points at the end of the list (after last element)
	struct dirent *fs_node;
	while ((fs_node = readdir(dir)) != NULL) {
		assert(i <= 999);

		if (strcmp(fs_node->d_name, ".") == 0 || strcmp(fs_node->d_name, "..") == 0) continue;
		if (fs_node->d_name[0] == '.') continue; // Ignore hidden files/directories (@should be optional)

		int len = strlen(fs_node->d_name);
		basenames[i] = (char *) malloc(len + 1);
		strcpy(basenames[i], fs_node->d_name);
		i += 1;
		//free(fs_node); // Thats an error
	}
	closedir(dir);

	qsort(basenames, i, sizeof(char *), compare);

	int j;
	for (j = 0; j < i; ++j) {
		char entry_path[1000]; //@ buffer bounds
		sprintf(entry_path, "%s/%s", dir_path, basenames[j]);

		//@ lstat()'ing a mounted windows-share that has disappeared fails. This is fine because we just ignore entries we fail to lstat() but it might take a long time for lstat() to fail and this causes UI to become unresponsive.
		struct stat entry_info;
		if (lstat(entry_path, &entry_info) == -1) {
			fprintf(stderr, "Failed to lstat \"%s\"\n", basenames[j]); // @error handling
			continue;
		}
		

		gboolean is_dir = S_ISDIR(entry_info.st_mode) ? TRUE : FALSE;

		const char *icon_path;
		if (is_dir) icon_path = folder_icon_path;
		else icon_path = file_icon_path;
		GdkPixbuf *icon = gdk_pixbuf_new_from_file(icon_path, NULL); //@ fail?

		// for regular files "is visited" is false.. pff
		gboolean is_visited = (is_dir == TRUE && max_depth > 0) ? TRUE : FALSE;
	
		GtkTreeIter iter;
		gtk_tree_store_append(store, &iter, parent); // create empty row
		gtk_tree_store_set(store, &iter,	// fill in values
			COLUMN_ICON, icon,
			COLUMN_BASENAME, basenames[j],
			COLUMN_FULL_PATH, entry_path,
			COLUMN_IS_DIR, is_dir,
			COLUMN_IS_VISITED, is_visited,
			COLUMN_IS_EDITABLE, FALSE,
			-1);

		g_object_unref(G_OBJECT(icon)); // !

		if (is_dir == TRUE && max_depth > 0) create_nodes_for_dir(store, &iter, entry_path, max_depth);
	}

	// we pass all strings in basenames to gtk_tree_store_set()
	// hopefully gtk_tree_store_set() doesnt use these strings directly and makes its own copies of them
	// because we are freeing them here hmm
	// "The value will be referenced by the store if it is a G_TYPE_OBJECT,
	// and it will be copied if it is a G_TYPE_STRING or G_TYPE_BOXED."
	for (int j = 0; j < i; ++j) {
		free(basenames[j]);
	}
}

static void on_filebrowser_row_expanded(GtkTreeView *tree_view,
												GtkTreeIter *iter,
												GtkTreePath *path,
												gpointer data)
{
	LOG_MSG("on_filebrowser_row_expanded()\n");

	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);

	char *dir_full_path;
	gtk_tree_model_get(model, iter, COLUMN_FULL_PATH, &dir_full_path, -1);
	//printf("on_filebrowser_row_expanded(): path: \"%s\"\n", dir_full_path);

	gboolean result;
	GtkTreeIter child;
	for (result = gtk_tree_model_iter_children(model, &child, iter);
			result == TRUE; result = gtk_tree_model_iter_next(model, &child)) {

		char *child_basename, *child_full_path;
		gboolean child_is_dir, child_is_visited;
		gtk_tree_model_get(model, &child,
							COLUMN_BASENAME, &child_basename,
							COLUMN_FULL_PATH, &child_full_path,
							COLUMN_IS_DIR, &child_is_dir,
							COLUMN_IS_VISITED, &child_is_visited,
							-1);

		//printf("on_filebrowser_row_expanded():");
		//printf(" child basename: \"%s\"", child_basename);
		//printf(" child full path: \"%s\"", child_full_path);
		//printf("\n");

		if (child_is_visited == TRUE) break; // reasonable to assume we have nothing to do here?

		if (child_is_dir == TRUE) {
			gtk_tree_store_set(GTK_TREE_STORE(model), &child, COLUMN_IS_VISITED, TRUE, -1);
			create_nodes_for_dir(GTK_TREE_STORE(model), &child, child_full_path, 1);
		}
	}
}

static void prepend_string(char *buffer, const char *str)
{
	char temp[100]; //@ buffer bounds

	sprintf(temp, "%s", buffer);
	sprintf(buffer, "%s%s", str, temp);
}

static void
on_filebrowser_row_doubleclicked
	(GtkTreeView *tree_view,
	GtkTreePath *path,
	GtkTreeViewColumn *column,
	gpointer data)
{
	LOG_MSG("on_filebrowser_row_doubleclicked()\n");

	GtkTreeIter iter;
	char *node_base_name;
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);

	/* @ full path is now part of the node info in store, so we should just query that */
	char node_rel_path[100]; node_rel_path[0] = 0;
	char *node_full_path = (char *) malloc(100); node_full_path[0] = 0; //@ free?

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

static void print_event(struct inotify_event *event)
{
	printf("print_event():");
	printf(" [%d]", event->wd);
	if (event->mask & IN_ISDIR) printf(" IN_ISDIR");
	if (event->mask & IN_ACCESS) printf(" IN_ACCESS");
	if (event->mask & IN_ATTRIB) printf(" IN_ATTRIB");
	if (event->mask & IN_CREATE) printf(" IN_CREATE");
	if (event->mask & IN_DELETE) printf(" IN_DELETE");
	if (event->mask & IN_DELETE_SELF) printf(" IN_DELETE_SELF");
	if (event->mask & IN_CLOSE_NOWRITE) printf(" IN_CLOSE_NOWRITE");
	if (event->mask & IN_CLOSE_WRITE) printf(" IN_CLOSE_WRITE");
	if (event->mask & IN_IGNORED) printf(" IN_IGNORED");
	if (event->mask & IN_MODIFY) printf(" IN_MODIFY");
	if (event->mask & IN_MOVE_SELF) printf(" IN_MOVE_SELF");
	if (event->mask & IN_MOVED_FROM) printf(" IN_MOVED_FROM");
	if (event->mask & IN_MOVED_TO) printf(" IN_MOVED_TO");
	if (event->mask & IN_OPEN) printf(" IN_OPEN");
	if (event->mask & IN_Q_OVERFLOW) printf(" IN_Q_OVERFLOW");
	if (event->mask & IN_UNMOUNT) printf(" IN_UNMOUNT");

	if (event->len > 0) printf(" \"%s\"", event->name);

	printf(" cookie: %d", event->cookie);

	printf("\n");
}

static void *monitor_fs_changes(void *arg)
{
	LOG_MSG("monitor_fs_changes()\n");

	int in_desc = inotify_init();

	int w_desc = inotify_add_watch(in_desc, "/home/eero/test", IN_ALL_EVENTS);
	if (w_desc == -1) {
		printf("monitor_fs_changes(): inotify_add_watch() error!\n");
		return NULL;
	}

	/* seems to be important to have space for multiple event instances.. */
	#define SIZE (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
	char *p;
	char event_buffer[SIZE];
	for (;;) {
		ssize_t n_bytes = read(in_desc, event_buffer, SIZE);
		if (n_bytes < 1) {
			printf("read() returned less than 1..\n");
			break;
		}
		//printf("watch_for_fs_changes(): read() returned: %ld\n", n_bytes);
		
		for (p = event_buffer; p < event_buffer + n_bytes;) {
			struct inotify_event *event = (struct inotify_event *) p;
//			print_event(event);
			p += sizeof(struct inotify_event) + event->len;
		}
	}

	return NULL;
}

static void on_basename_edited(
	GtkCellRendererText *cell,
	gchar *path_str,
	gchar *new_basename,
	gpointer data)
{
	LOG_MSG("on_basename_edited()\n");

	GtkTreeModel *model;
	GtkTreeIter iter;
	char *old_basename, *node_dirname, *old_pathname, new_pathname[256];

	//model = (GtkTreeModel *) data;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(filebrowser));

	gboolean exists = gtk_tree_model_get_iter_from_string(model, &iter, path_str);
	assert(exists == TRUE); // we assume that the row edited "just now" exists..
	gtk_tree_model_get(model, &iter,
		COLUMN_BASENAME, &old_basename,
		COLUMN_FULL_PATH, &old_pathname,
		-1);

	char *copy_old_pathname = strdup(old_pathname); // seems that dirname() modifies the string given
	node_dirname = dirname(old_pathname);

	LOG_MSG("on_basename_edited(): old dirname: \"%s\", old basename: \"%s\", new basename: \"%s\"\n",
		node_dirname, old_basename, new_basename);

	snprintf(new_pathname, 256, "%s/%s", node_dirname, new_basename);
	LOG_MSG("on_basename_edited(): old pathname: \"%s\", new pathname: \"%s\"\n",
		copy_old_pathname,
		new_pathname);

	if ((rename(copy_old_pathname, new_pathname)) != 0) {
		printf("on_basename_edited(): : failed to rename file/folder: \"%s\" to \"%s\"\n", copy_old_pathname, new_pathname);
		//return;
		goto wrap_up;
	}

	// set COLUMN_IS_EDITABLE to FALSE?
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
		COLUMN_BASENAME, new_basename,
		COLUMN_FULL_PATH, new_pathname,
		COLUMN_IS_EDITABLE, FALSE,
		-1);

wrap_up:
	free(copy_old_pathname);	
	//free(old_basename); // ?
	//free(new_basename); // ?

	return;
}

/*
	Once user is done editing, "edited"-signal fires for the column edited..
	So we can register a callback for that.
*/
static void set_node_editable(GtkTreeStore* store, GtkTreePath *parent, GtkTreePath *node)
{
	gtk_tree_view_expand_to_path(GTK_TREE_VIEW(filebrowser), parent);

	GtkTreeIter node_iter;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &node_iter, node);
	gtk_tree_store_set(store, &node_iter, COLUMN_IS_EDITABLE, TRUE, -1);

	GtkTreeViewColumn *col = gtk_tree_view_get_column(GTK_TREE_VIEW(filebrowser), 0);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(filebrowser), node, col, TRUE);
}

static void on_dir_menu_newfolder_selected(GtkMenuItem *item, gpointer data)
{
	LOG_MSG("on_dir_menu_newfolder_selected()\n");

	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter parent_node, new_node;
	char *parent_fullpath, new_fullpath[255]; //@ max filepath length?
	gboolean is_dir = FALSE;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(filebrowser));
	path = (GtkTreePath *) data;

	gtk_tree_model_get_iter(model, &parent_node, path);
	gtk_tree_model_get(model, &parent_node,
		COLUMN_FULL_PATH, &parent_fullpath,
		COLUMN_IS_DIR, &is_dir,
		-1);

	assert(is_dir == TRUE);

	snprintf(new_fullpath, 255, "%s/%s", parent_fullpath, "Unnamed Folder");

	if (mkdir(new_fullpath, S_IRUSR | S_IWUSR | S_IXUSR) == -1) {
		printf("on_dir_menu_newfolder_selected(): encountered an error when creating a folder: \"%s\"!\n", new_fullpath);
		gtk_tree_path_free(path);
		free(parent_fullpath);
		return;
	}
	LOG_MSG("on_dir_menu_newfolder_selected(): successfully created a folder: \"%s\"!\n", new_fullpath);

	GdkPixbuf *icon = gdk_pixbuf_new_from_file(folder_icon_path, NULL);

	gtk_tree_store_append(GTK_TREE_STORE(model), &new_node, &parent_node);
	//gtk_tree_store_prepend(GTK_TREE_STORE(model), &new_node, &parent_node);
	gtk_tree_store_set(GTK_TREE_STORE(model), &new_node,
		COLUMN_ICON, icon,
		COLUMN_BASENAME, "Unnamed Folder",
		COLUMN_FULL_PATH, new_fullpath,
		COLUMN_IS_DIR, TRUE,

		//@ that could be tricky, we're fine here with TRUE I quess
		// but it's only because we APPEND this node, so it never appears before other
		// directory-nodes. if we were to PREPEND it, we have a bug..
		//@ also, the newly added node is not sorted along with other nodes in the directory..
		// delete all nodes in directory and call create_nodes_for_dir(depth=2) after creating the
		// folder?
		COLUMN_IS_VISITED, TRUE,

		COLUMN_IS_EDITABLE, FALSE,
		-1);

	GtkTreePath *new_node_path = gtk_tree_model_get_path(model, &new_node);
	set_node_editable(GTK_TREE_STORE(model), path, new_node_path);

	gtk_tree_path_free(path);
	gtk_tree_path_free(new_node_path);
	free(parent_fullpath);
}

static void on_dir_menu_newfile_selected(GtkMenuItem *item, gpointer data)
{
	LOG_MSG("on_dir_menu_newfile_selected()\n");

	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter parent_node, new_node;
	char *parent_fullpath, new_fullpath[256]; //@ max filepath length?
	gboolean is_dir = FALSE;
	int fd;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(filebrowser));
	path = (GtkTreePath *) data;

	gtk_tree_model_get_iter(model, &parent_node, path);
	gtk_tree_model_get(model, &parent_node,
		COLUMN_FULL_PATH, &parent_fullpath,
		COLUMN_IS_DIR, &is_dir,
		-1);

	assert(is_dir == TRUE);

	snprintf(new_fullpath, 256, "%s/%s", parent_fullpath, "Unnamed File");

	fd = open(new_fullpath, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		printf("on_dir_menu_newfile_selected(): encountered an error when creating a file: \"%s\"!\n", new_fullpath);
		gtk_tree_path_free(path);
		free(parent_fullpath);
		return;
	}
	LOG_MSG("on_dir_menu_newfile_selected(): successfully created a file: \"%s\"\n", new_fullpath);

	close(fd);

	GdkPixbuf *icon = gdk_pixbuf_new_from_file(file_icon_path, NULL);

	gtk_tree_store_append(GTK_TREE_STORE(model), &new_node, &parent_node);
	gtk_tree_store_set(GTK_TREE_STORE(model), &new_node,
		COLUMN_ICON, icon,
		COLUMN_BASENAME, "Unnamed File",
		COLUMN_FULL_PATH, new_fullpath,
		COLUMN_IS_DIR, FALSE,
		COLUMN_IS_VISITED, FALSE,
		COLUMN_IS_EDITABLE, FALSE,
		-1);
	
	GtkTreePath *new_node_path = gtk_tree_model_get_path(model, &new_node);
	set_node_editable(GTK_TREE_STORE(model), path, new_node_path);

	gtk_tree_path_free(path);
	gtk_tree_path_free(new_node_path);
	free(parent_fullpath);
}

static void on_menu_rename_selected(GtkMenuItem *item, gpointer data)
{
	LOG_MSG("on_menu_rename_selected()\n");

	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(filebrowser));
	gboolean result = gtk_tree_selection_get_selected(selection, &model, &iter);
	assert(result == TRUE); // there is a selection

	path = (GtkTreePath *) data;
	//path = gtk_tree_model_get_path(model, &iter);

/*
	GValue editability = G_VALUE_INIT;
	g_value_init(&editability, G_TYPE_BOOLEAN);
	g_value_set_boolean(&editability, TRUE);
	gtk_tree_store_set_value(GTK_TREE_STORE(model), &iter, COLUMN_IS_EDITABLE, &editability);
*/

	gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COLUMN_IS_EDITABLE, TRUE, -1);

	GtkTreeViewColumn *col = gtk_tree_view_get_column(GTK_TREE_VIEW(filebrowser), 0);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(filebrowser), path, col, TRUE);
}

static void on_menu_delete_selected(GtkMenuItem *item, gpointer data)
{
	LOG_MSG("on_menu_delete_selected()\n");

	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	char *full_path;
	int ret;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(filebrowser));
	path = (GtkTreePath *) data;

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, COLUMN_FULL_PATH, &full_path, -1);

	/* It works on Ubuntu 16.04 LTS bla-bla, no idea if it works on anything else.. */
	char command[256];
	snprintf(command, 256, "mv \"%s\" /home/eero/.local/share/Trash/files", full_path); //@ hardcoded path
	ret = system(command);
	LOG_MSG("on_menu_delete_selected(): system() returned %d\n", ret);
	if (ret != 0) {
		printf("on_menu_delete_selected(): failed to trash file: \"%s\"!\n", full_path);
	} else {
		gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
		LOG_MSG("on_menu_delete_selected(): successfully trashed file: \"%s\"!\n", full_path);
	}

	/* remove() deletes files and empty directories */
/*
	ret = remove(full_path); // returns 0 on success, -1 on error
	if (ret == -1) {
		printf("on_menu_delete_selected(): failed to delete file: \"%s\"!\n", full_path);
	} else {
		gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
		printf("on_menu_delete_selected(): successfully deleted file: \"%s\"!\n", full_path);
	}
*/

	gtk_tree_path_free(path);
	free(full_path);
}

static gboolean on_filebrowser_button_pressed(
	GtkWidget *filebrowser,
	GdkEvent *event,
	gpointer data)
{
	//printf("on_filebrowser_button_pressed()\n");

	GdkEventType event_type = gdk_event_get_event_type(event);
	//printf("on_filebrowser_button_pressed(): type: %d\n", event_type);

	//Strangely enough, even though we register this callback for "button-press-event", this assertion fails when a user double-clicks on a row:
	//assert(event_type == GDK_BUTTON_PRESS);

	if (event_type != GDK_BUTTON_PRESS) {
		LOG_MSG("on_filebrowser_button_pressed(): unknown event.. exiting..\n");
		//return TRUE; // no other handlers
		return FALSE; // propagate further 
	}

	GdkEventButton *button_event = (GdkEventButton *) event;
	
	/* left: 1, middle: 2, right: 3 */
	LOG_MSG("on_filebrowser_button_pressed(): GDK_BUTTON_PRESS (%d)\n", button_event->button);

	if (button_event->button == 3) {

		// these seem to be relative to the widget the callback was registered for:
		LOG_MSG("on_filebrowser_button_pressed(): x: %f, y: %f\n", button_event->x, button_event->y);

		GtkTreePath *path;
		gboolean path_exists = gtk_tree_view_get_path_at_pos(
			GTK_TREE_VIEW(filebrowser),
			(int) button_event->x, // float to int?
			(int) button_event->y,
			&path,
			NULL, NULL, NULL);

		if (path_exists == FALSE) {
			LOG_MSG("on_filebrowser_button_pressed(): no row at that location..\n");
			return FALSE;
		}

		GtkTreeIter iter;
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(filebrowser));
		gtk_tree_model_get_iter(model, &iter, path);

		gboolean is_dir;
		gtk_tree_model_get(model, &iter, COLUMN_IS_DIR, &is_dir, -1);
	
		GtkWidget *menu = gtk_menu_new();

		if (is_dir == TRUE) {
			add_menu_item(GTK_MENU(menu), "New Folder", G_CALLBACK(on_dir_menu_newfolder_selected), (gpointer) path);
			add_menu_item(GTK_MENU(menu), "New File", G_CALLBACK(on_dir_menu_newfile_selected), (gpointer) path);
		}
		add_menu_item(GTK_MENU(menu), "Rename", G_CALLBACK(on_menu_rename_selected), (gpointer) path);
		add_menu_item(GTK_MENU(menu), "Move To Trash", G_CALLBACK(on_menu_delete_selected), (gpointer) path);

		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button_event->button, button_event->time);
	}

	return FALSE;
}

GtkTreeStore *create_store()
{
	//LOG_MSG("create_tree_store(): root_dir: %s\n", root_dir);
	LOG_MSG("create_tree_store(): root_dir: %s\n", root_dir);
	GtkTreeStore *store = gtk_tree_store_new(
		N_COLUMNS,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_BOOLEAN,
		G_TYPE_BOOLEAN,
		G_TYPE_BOOLEAN
	);
	create_nodes_for_dir(store, NULL, root_dir, 2);
	return store;
}

static GtkWidget *create_filebrowser_view(GtkTreeModel *model)
{
	LOG_MSG("create_filebrowser_view()\n");
	//getcwd(root_dir, sizeof(root_dir));

	//GtkTreeStore *tree_store = create_tree_store();
	GtkWidget *view = gtk_tree_view_new_with_model(model);


	GtkTreeViewColumn *col = gtk_tree_view_column_new();

	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", COLUMN_ICON, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_BASENAME, "editable", COLUMN_IS_EDITABLE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(on_basename_edited), model);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	return view;
}

GtkWidget *create_filebrowser_widget(void)
{
	LOG_MSG("create_file_browser_widget()\n");
	//printf("create_filebrowser_widget()\n");

	GtkTreeStore *store = create_store();
	filebrowser = create_filebrowser_view(GTK_TREE_MODEL(store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(filebrowser), FALSE);
	//gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(filebrowser_widget), 9);
	add_class(filebrowser, "filebrowser");

	g_signal_connect(filebrowser, "row-activated", G_CALLBACK(on_filebrowser_row_doubleclicked), NULL);
	g_signal_connect(filebrowser, "row-expanded", G_CALLBACK(on_filebrowser_row_expanded), NULL);
	g_signal_connect(filebrowser, "button-press-event", G_CALLBACK(on_filebrowser_button_pressed), NULL);

/*
	pthread_t id;
	if ((pthread_create(&id, NULL, monitor_fs_changes, NULL)) != 0) {
		printf("create_filebrowser_widget(): pthread_create() error!\n");
	}
*/

	return filebrowser;
}
