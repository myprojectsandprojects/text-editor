struct TabInfo {
	unsigned id;
	const char *file_name; // NULL if no file associated with the tab.
	const char *title;
	gboolean unsaved_changes;
	//char *base_name; // Maybe well put basename here for convenience.
};

GtkWidget *get_visible_tab(GtkNotebook *notebook);
GtkTextBuffer *get_visible_text_buffer(GtkNotebook *notebook);
GtkTextView *tab_get_text_view(GtkWidget *tab);
GtkRevealer *tab_get_command_revealer(GtkWidget *tab);
GtkRevealer *tab_get_search_revealer(GtkWidget *tab);