#include <gtk/gtk.h>
#include <assert.h>

GtkWidget *get_visible_tab(GtkNotebook *notebook)
{
	printf("get_visible_tab() called.\n");

	int page = gtk_notebook_get_current_page(notebook);
	if (page == -1) {
		// no tabs open
		return NULL;
	}

	return gtk_notebook_get_nth_page(notebook, page);
}

GtkTextBuffer *get_visible_text_buffer(GtkNotebook *notebook)
{
	GtkScrolledWindow *scrolled_window = NULL;
	GtkTextView *text_view = NULL;
	GtkTextBuffer *text_buffer = NULL;
	GList *children, *child;

	int page = gtk_notebook_get_current_page(notebook);
	if (page == -1) {
		// no tabs open
		return NULL;
	}
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);

	children = gtk_container_get_children(GTK_CONTAINER(tab)); //@ free?
	for(child = children; child != NULL; child = child->next){
		if(GTK_IS_SCROLLED_WINDOW(child->data) == TRUE) {
			scrolled_window = child->data;
			break;
		}
	}
	assert(scrolled_window != NULL); // We dont want to silently fail here.
	text_view = (GtkTextView *) gtk_bin_get_child(GTK_BIN(scrolled_window));
	assert(text_view != NULL); // We dont want to silently fail here.
	text_buffer = gtk_text_view_get_buffer(text_view);
	assert(text_buffer != NULL);
	return text_buffer;
}

GtkTextView *tab_get_text_view(GtkWidget *tab)
{
	GtkScrolledWindow *scrolled_window = NULL;
	GtkTextView *text_view = NULL;
	GList *children, *child;
	children = gtk_container_get_children(GTK_CONTAINER(tab)); //@ free?
	for(child = children; child != NULL; child = child->next){
		if(GTK_IS_SCROLLED_WINDOW(child->data) == TRUE) {
			scrolled_window = child->data;
			break;
		}
	}
	assert(scrolled_window != NULL); // We dont want to silently fail here.
	text_view = (GtkTextView *) gtk_bin_get_child(GTK_BIN(scrolled_window));
	assert(text_view != NULL); // We dont want to silently fail here.
	return text_view;
}

GtkRevealer *tab_get_command_revealer(GtkWidget *tab)
{
	GtkRevealer *command_revealer = NULL;
	GList *children, *child;
	children = gtk_container_get_children(GTK_CONTAINER(tab)); //@ free?
	for(child = children; child != NULL; child = child->next){
		/*if(GTK_IS_REVEALER(child->data) == TRUE) {
			search_revealer = child->data;
			break;
		}*/
		const char *name = gtk_widget_get_name(child->data);
		if(strcmp(name, "command-revealer") == 0) {
			command_revealer = child->data;
		}
	}
	assert(command_revealer != NULL); // We dont want to silently fail here.
	return command_revealer;
}

GtkRevealer *tab_get_search_revealer(GtkWidget *tab)
{
	GtkRevealer *search_revealer = NULL;
	GList *children, *child;
	children = gtk_container_get_children(GTK_CONTAINER(tab)); //@ free?
	for(child = children; child != NULL; child = child->next){
		/*if(GTK_IS_REVEALER(child->data) == TRUE) {
			search_revealer = child->data;
			break;
		}*/
		const char *name = gtk_widget_get_name(child->data);
		if(strcmp(name, "search-revealer") == 0) {
			search_revealer = child->data;
		}
	}
	assert(search_revealer != NULL); // We dont want to silently fail here.
	return search_revealer;
}