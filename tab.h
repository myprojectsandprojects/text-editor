
/*
Its totally tedious having to update this enum, when "registering" a new *thing*.
Can't we have a function like register_thing(tab, "name-for-the-thing", (void *) the-thing) ?
And then go like get_the_thing(tab, "name-for-the-thing") ..
*/

enum WidgetName{
	TEXT_VIEW,
	TEXT_BUFFER,
	COMMAND_REVEALER,
	COMMAND_ENTRY,
	SEARCH_REVEALER,
	SEARCH_ENTRY,
	REPLACE_REVEALER,
	REPLACE_ENTRY,
	STATUS_MESSAGE_LABEL,
	N_WIDGETS
};

void *tab_widgets[100][N_WIDGETS];

/*
When some1 needs multiple widgets, then they have to make a separate function call 4 every widget, but in that case we are kinda repeating ourselves...
*/

void tab_add_widget_4_retrieval(GtkWidget *tab, enum WidgetName widget_name, void *widget);
void *tab_retrieve_widget(GtkWidget *tab, enum WidgetName widget_name);

/* Maybe these functions should be somewhere else? */
void *visible_tab_retrieve_widget(GtkNotebook *notebook, enum WidgetName widget_name);
GtkWidget *get_visible_tab(GtkNotebook *notebook);


struct TabInfo {
	unsigned id;
	const char *file_name; // NULL if no file associated with the tab.
	const char *title;
	gboolean unsaved_changes;
	//char *base_name; // Maybe well put basename here for convenience.
};


