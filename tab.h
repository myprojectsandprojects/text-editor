
/*
Its totally tedious having to update this enum, when "registering" a new *thing*.
Can't we have a function like register_thing(tab, "name-for-the-thing", (void *) the-thing) ?
And then go like get_the_thing(tab, "name-for-the-thing") ..

Really I think its a matter of minimizing the amount of dependencies between modules..
For some reason it seems like a good idea to have abstractions as separated from eachother and independent as possible.. for code reusability purposes I quess, but also to keep the complexity at a minimum. but thats only ofcourse when abstractions are good..

@ So in this case, maybe we could create a hash-table or something? Even if a hash-table in this particular case (where perhaps a simple array of some structs would do) would be an overkill, a hash-table is such a general-purpose data-structure that I think we might want one around anyways.
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
	FILEPATH_LABEL,
	AUTOCOMPLETE_WORDS,
	CURRENT_TEXT_HIGHLIGHTING,
	HIGHLIGHTING_BUTTON_LABEL,
	HIGHLIGHTING_CHANGED_EVENT_HANDLERS,
	HIGHLIGHTING_CHANGED_EVENT_HANDLERS_INDEX,
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


