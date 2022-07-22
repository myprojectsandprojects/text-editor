#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "declarations.h"


//struct Table *func_table; // maps highlighting languages to highlighting functions

//@ declare all functions, defined in this file, here?
// or do it more conventionally in a separate header file ("highlighting.h")?
// and drop the whole "declarations.h" thing?

void c_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end);
void cpp_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end);
void css_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end);
void rust_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end);

typedef void (*Highlighter) (GtkTextBuffer *, GtkTextIter *, GtkTextIter *);

struct TableEntry {
	const char *name;			// highlighting name
	Highlighter highlighter;	// function that does the highlighting
};

struct TableEntry highlighters[] = {
	{"C", cpp_highlight},
	{"C++", cpp_highlight},
	{"CSS", css_highlight},
	{"Rust", rust_highlight},
};
//const int highlighters_num_entries = sizeof(highlighters) / sizeof(struct TableEntry);

Highlighter highlighters_get_func(const char *name) {
	for (int i = 0; i < COUNT(highlighters); ++i) {
		if (strcmp(name, highlighters[i].name) == 0) {
			return highlighters[i].highlighter;
		}
	}
	return NULL;
}

extern struct Node *settings;

//@ multiple tabs share these global variables... could this be a problem?
int global_location_offset; // location of last insertion or deletion
int global_length; // length of text inserted or 0 if deletion
char *global_text;

void c_get_highlighting_range(GtkTextBuffer *text_buffer, GtkTextIter *iter, GtkTextIter *start, GtkTextIter *end){
//	gtk_text_buffer_get_bounds(text_buffer, start, end);
	*start = *end = *iter;
//	while(gtk_text_iter_forward_char(end)){
//		if(gtk_text_iter_get_char(end) == '\n'){
//			gtk_text_iter_forward_char(end);
//			break;
//		}
//	}
//	while(gtk_text_iter_backward_char(start)){
//		if(gtk_text_iter_get_char(start) == '\n'){
//			gtk_text_iter_forward_char(start);
//			break;
//		}
//	}

	GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);
	GtkTextTag *comment_tag = gtk_text_tag_table_lookup(table, "comment");
	GtkTextTag *string_tag 	= gtk_text_tag_table_lookup(table, "string");

	while (gtk_text_iter_backward_char(start)) {
		//@ what if ';' is inside a comment/string?
		if (gtk_text_iter_get_char(start) == ';') {
			if (gtk_text_iter_has_tag(start, comment_tag) || gtk_text_iter_has_tag(start, string_tag)) {
				continue; // ignore if inside a comment/string
			}
			break;
		}
	}

	while (gtk_text_iter_forward_char(end)) {
		//@ what if ';' is inside a comment?
		if (gtk_text_iter_get_char(end) == ';') {
			if (gtk_text_iter_has_tag(end, comment_tag) || gtk_text_iter_has_tag(end, string_tag)) {
				continue; // ignore if inside a comment/string
			}
			break;
		}
	}

//	char *text = gtk_text_buffer_get_text(text_buffer, start, end, FALSE);
//	printf("before: \"%s\"\n", text);
//	free(text);
}

void select_highlighting_based_on_file_extension(GtkWidget *tab, struct Node *settings, const char *file_name){
	LOG_MSG("select_highlighting_based_on_file_extension()\n");

	if (!file_name) {
		set_text_highlighting(tab, "None");
		return;
	}

	struct Node *languages = get_node(settings, "highlighting/languages");
	assert(languages);
	const char *language = NULL;
	for (int i = 0; !language && i < languages->nodes->i_end; ++i) {
		struct Node *n_language = (struct Node *) languages->nodes->data[i];
		const char *name = n_language->name;
		
		struct Node *extensions = get_node(n_language, "file-extensions");
//		assert(extensions); // we might have a highlighting without any extensions associated with it...
		if (!extensions) {
			continue;
		}

		for (int i = 0; i < extensions->nodes->i_end; ++i) {
			struct Node *n_extension = (struct Node *) extensions->nodes->data[i];
			const char *extension = n_extension->name;
			// "css" or ".css"
			if (ends_with(file_name, extension)) {
				language = name;
				break;
			}
		}
	}

	if (language) {
		set_text_highlighting(tab, language);
	} else {
		set_text_highlighting(tab, "None");
	}
}

void create_tags(GtkWidget *tab, const char *language){
	LOG_MSG("create_tags()\n");

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));

	// we store them so that we know what to remove when highlighting language changes
	// initially, remove_tags() looked up its tags from settings,
	// but now, as we are doing the hotloading business, we dont know if the tags have changed..
	// alternatively, we could perhaps remove the tags before updating the settings
	// or just keep the old settings around until everything is updated, somehow
	struct CList *tags_list = new_list();
	//@ free old list
	tab_add_widget_4_retrieval(tab, HIGHLIGHTING_TAGS, (void *) tags_list);

	const int size = 100;
	char path[size];
	snprintf(path, size, "highlighting/languages/%s/tags", language);
	struct Node *tags = get_node(settings, path);

	//assert(tags);
	if (!tags) {
		return;
	}

	for (int i = 0; i < tags->nodes->i_end; ++i) {
		struct Node *tag = (struct Node *) tags->nodes->data[i];

		const char *tag_name = tag->name;
		GtkTextTag *gtk_tag = gtk_text_buffer_create_tag(text_buffer, tag_name, NULL);
		list_append(tags_list, (void *) gtk_tag);

		for (int i = 0; i < tag->nodes->i_end; ++i) {
			struct Node *name = (struct Node *) tag->nodes->data[i];
			struct Node *value = (struct Node *) name->nodes->data[0];
			if (strcmp(name->name, "style") == 0) {
				PangoStyle translated_value;
				if (strcmp(value->name, "PANGO_STYLE_NORMAL") == 0) {
					translated_value = PANGO_STYLE_NORMAL;
				} else if (strcmp(value->name, "PANGO_STYLE_ITALIC") == 0) {
					translated_value = PANGO_STYLE_ITALIC;
				} else if (strcmp(value->name, "PANGO_STYLE_OBLIQUE") == 0) {
					translated_value = PANGO_STYLE_OBLIQUE;
				} else {
					assert(false); //@ more elegant error handling, inform the user
				}
				g_object_set(G_OBJECT(gtk_tag), name->name, translated_value, NULL);
				continue;
			}
			g_object_set(G_OBJECT(gtk_tag), name->name, value->name, NULL);
		}
	}
}

void remove_tags(GtkWidget *tab)
{
	LOG_MSG("remove_tags()\n");

	GtkTextBuffer *text_buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);
	GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);

	struct CList *tags = (struct CList *) tab_retrieve_widget(tab, HIGHLIGHTING_TAGS);
	for (int i = 0; i < tags->i_end; ++i) {
		gtk_text_tag_table_remove(table, (GtkTextTag *) tags->data[i]);
	}
}

//static void on_text_buffer_changed(GtkTextBuffer *buffer, gpointer data)
//{
//	LOG_MSG("on_text_buffer_changed()\n");
//	printf("on_text_buffer_changed()\n");
//
////	// this is just temporary for testing:
////	// begin
////	{
////		void *func = tab_retrieve_widget((GtkWidget *) data, HIGHLIGHTING_FUNC);
////		if (func) { //@ shouldnt this be an assertion? why are we being called, if there is no highlighting?
////			GtkTextIter start_buf, end_buf;
////			gtk_text_buffer_get_bounds(buffer, &start_buf, &end_buf);
////			((Highlighter) func)(buffer, &start_buf, &end_buf);
////		}
////		return;
////	}
////	// end
//
////	// debug stuff:
////	GtkTextIter dbg_iter;
////	gtk_text_buffer_get_iter_at_offset(buffer, &dbg_iter, global_location_offset);
////	gunichar dbg_ch = gtk_text_iter_get_char(&dbg_iter);
////	printf("********* global_location_offset -> %c\n", dbg_ch);
////	printf("********* global_text -> %s\n", global_text);
//
//	GtkTextIter iter, start, end;
//
//// For what is this for?
////
////	if (strchr(global_text, '\"') || strchr(global_text, '\\')) { // backslash could escape a doublequote, hence
////		printf("!!!\n");
////		GtkTextIter text_start, text_end;
////		gtk_text_buffer_get_iter_at_offset(buffer, &text_start, global_location_offset);
////		gtk_text_buffer_get_iter_at_offset(buffer, &text_end, global_location_offset + global_length);
////g_print("before\n");
////		if (gtk_text_iter_get_char(&text_start) == '\n') goto TOKEN_RANGE;
////		gtk_text_iter_forward_char(&text_start);
////		if (gtk_text_iter_get_char(&text_start) == '\n') goto TOKEN_RANGE;
////		gtk_text_iter_backward_char(&text_start);
////g_print("after\n");
////		for (; !gtk_text_iter_is_start(&text_start); gtk_text_iter_backward_char(&text_start)) {
////			if (gtk_text_iter_get_char(&text_start) == '\n') break;
////		}
////		for (; !gtk_text_iter_is_end(&text_end); gtk_text_iter_forward_char(&text_end)) {
////			if (gtk_text_iter_get_char(&text_end) == '\n') break;
////		}
////
////		void *func = tab_retrieve_widget((GtkWidget *) data, HIGHLIGHTING_FUNC);
////		if (func) {
////			((Highlighter) func)(buffer, &text_start, &text_end);
////		}
////		
////		return;
////	}
//
//TOKEN_RANGE:
//	gtk_text_buffer_get_iter_at_offset(buffer, &iter, global_location_offset);
//
//	// we need to update the highlighting when the buffer contents are modified.
//	// because we cant afford to update the whole buffer each time user types/deletes a character
//	// we have to limit the updated region somehow.
//	// currently, all we do in ...text_buffer_changed() signal-handler is that we count an arbitrary # of tokens (for-/back)ward
//	// to set our bounds.
//	// our highlighting code in c_highlight() tries to recognize types and function calls
//	// which means we have to look at multiple tokens to decide on whats what.
//	// this means though that arbitrarily bounding the updated region inevitably produces incorrect highlighting in some cases.
//	// currently to fix this problem we select the bounds based on language syntax/semantics in c_highlight().
//
//	void *func = tab_retrieve_widget((GtkWidget *) data, HIGHLIGHTING_FUNC);
//	assert(func);
//	
//	if (func == c_highlight) {
//		// language is C, so call the highlighter function immediately
////		c_get_highlighting_range(buffer, &iter, &start, &end);
////		((Highlighter) func)(buffer, &start, &end);
//		gtk_text_buffer_get_start_iter(buffer, &start);
//		gtk_text_buffer_get_end_iter(buffer, &end);
//		((Highlighter) func)(buffer, &start, &end);
//		return;
//	}
//	if (func == rust_highlight) {
//		gtk_text_buffer_get_start_iter(buffer, &start);
//		gtk_text_buffer_get_end_iter(buffer, &end);
//		((Highlighter) func)(buffer, &start, &end);
//		return;
//	}
//
//	int count = 0;
//	start = iter;
//	gtk_text_iter_backward_char(&start);
//	while (gtk_text_iter_backward_char(&start)) {
//		if (gtk_text_iter_begins_tag(&start, NULL)) {
//			if (count > 3) break;
//			count += 1;
//		}
//	}
//
//	count = 0;
//	end = iter;
//	while (gtk_text_iter_forward_char(&end)) {
//		if (gtk_text_iter_ends_tag(&end, NULL)) {
//			if (count > 3) break;
//			count += 1;
//		}
//	}
//
////	// debug stuff:
////	char *dbg_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
////	printf("********* highlighting range: \"%s\"\n", dbg_text);
////	free(dbg_text);
//
//	((Highlighter) func)(buffer, &start, &end);
//}

void highlighting_text_buffer_cursor_position_changed(GObject *object, GParamSpec *pspec, gpointer user_data)
{
	LOG_MSG("highlighting_text_buffer_cursor_position_changed()\n");

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(object);
	int position;
	g_object_get(G_OBJECT(text_buffer), "cursor-position", &position, NULL);
	GtkTextIter i;
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(text_buffer), &i, position);

	GtkTextIter start_buffer, end_buffer;
	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);

//	// lets try something
//	GtkTextIter j1, j2;
//	j1 = j2 = i;
//	gtk_text_iter_forward_char(&j2);
//	gtk_text_buffer_remove_tag_by_name(text_buffer, "cursor-highlight", &start_buffer, &end_buffer);
//	gtk_text_buffer_apply_tag_by_name(text_buffer, "cursor-highlight", &j1, &j2);

	GtkTextIter start, end;
	gtk_text_buffer_remove_tag_by_name(text_buffer, "line-highlight", &start_buffer, &end_buffer);
	gtk_text_iter_set_line_offset(&i, 0);
	start = i;
	gtk_text_iter_forward_line(&i);
	//gtk_text_iter_forward_char(&i);
	//gtk_text_iter_backward_char(&i);
	end = i;
	gtk_text_buffer_apply_tag_by_name(text_buffer, "line-highlight", &start, &end);
}

static void on_text_buffer_insert_text(GtkTextBuffer *buffer, GtkTextIter *location, char *text, int length, gpointer data)
{
	LOG_MSG("on_text_buffer_insert_text()\n");
	printf("on_text_buffer_insert_text()\n");

	/*GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	const char *contents = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);*/
	//g_print("contents: %s\n", contents);

	global_location_offset = gtk_text_iter_get_offset(location);
	global_length = length;
	global_text = text;
}

static void on_text_buffer_delete_range(GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end)
{
	LOG_MSG("on_text_buffer_delete_range()\n");
	printf("on_text_buffer_delete_range()\n");

	global_location_offset = gtk_text_iter_get_offset(start);
	global_text = gtk_text_buffer_get_text(buffer, start, end, FALSE);
	global_length = 0;
}


// what about using insert-text-after and delete-range-after signals?
// then we could get rid of our global_... variables.
static void on_insert_text_after(GtkTextBuffer *text_buffer, GtkTextIter *location, gchar *text, gint len, gpointer user_data){
	printf("on_insert_text_after()\n");

//	printf("location offset: %d, text: %s, len: %d\n", gtk_text_iter_get_offset(location), text, len);
//	printf("character at location: %c[%d]\n", gtk_text_iter_get_char(location), gtk_text_iter_get_char(location));

//	{
//		GtkTextIter a, b;
//		gtk_text_buffer_get_bounds(text_buffer, &a, &b);
//		char *contents = gtk_text_buffer_get_text(text_buffer, &a, &b, FALSE);
//		printf("\"%s\"\n", contents);
//	}

//	{
//		GtkTextIter a, b;
//		a = b = *location;
//		gtk_text_iter_backward_chars(&a, len);
//		char *text_inserted = gtk_text_buffer_get_text(text_buffer, &a, &b, FALSE);
//		printf("text inserted: \"%s\"\n", text_inserted);
//	}

	cpp_highlight(text_buffer, location, NULL);
}

static void on_delete_range_after(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end){
	printf("on_delete_range_after()\n");

//	printf("start: %c, end: %c\n", gtk_text_iter_get_char(start), gtk_text_iter_get_char(end));
	assert(gtk_text_iter_compare(start, end) == 0); // should be equal after deletion

	cpp_highlight(text_buffer, start, NULL);
}

void register_handlers(GtkWidget *tab, GtkTextBuffer *text_buffer)
{
	LOG_MSG("register_handlers()\n");

	unsigned long ids;

//	ids = g_signal_connect(text_buffer, "changed", G_CALLBACK(on_text_buffer_changed), (void *) tab);
//	g_object_set_data(G_OBJECT(text_buffer), "changed-handler-4-highlighting", (void *) ids);
//
//	ids = g_signal_connect(text_buffer, "insert-text", G_CALLBACK(on_text_buffer_insert_text), NULL);
//	g_object_set_data(G_OBJECT(text_buffer), "insert-text-handler-4-highlighting", (void *) ids);
//
//	ids = g_signal_connect(text_buffer, "delete-range", G_CALLBACK(on_text_buffer_delete_range), NULL);
//	g_object_set_data(G_OBJECT(text_buffer), "delete-range-handler-4-highlighting", (void *) ids);

	// testing:
	ids = g_signal_connect_after(text_buffer, "insert-text", G_CALLBACK(on_insert_text_after), NULL);
	g_object_set_data(G_OBJECT(text_buffer), "insert-text-after123", (void *) ids);
	ids = g_signal_connect_after(text_buffer, "delete-range", G_CALLBACK(on_delete_range_after), NULL);
	g_object_set_data(G_OBJECT(text_buffer), "delete-range-after123", (void *) ids);
}


void unregister_handlers(GtkTextBuffer *text_buffer)
{
	LOG_MSG("unregister_handlers()\n");

	unsigned long ids;

	ids = (unsigned long) g_object_get_data(G_OBJECT(text_buffer), "changed-handler-4-highlighting");
	g_signal_handler_disconnect(text_buffer, ids);

	ids = (unsigned long) g_object_get_data(G_OBJECT(text_buffer), "insert-text-handler-4-highlighting");
	g_signal_handler_disconnect(text_buffer, ids);

	ids = (unsigned long) g_object_get_data(G_OBJECT(text_buffer), "delete-range-handler-4-highlighting");
	g_signal_handler_disconnect(text_buffer, ids);
}


void event_register_handler(const char *event_name, void *handler, GtkWidget *tab)
{
	LOG_MSG("event_register_handler()\n");

	if (strcmp(event_name, "highlighting-changed") == 0) {
		struct List<void *> *handlers =
			(struct List<void *> *) tab_retrieve_widget(tab, HIGHLIGHTING_CHANGED_EVENT_HANDLERS);
		if (!handlers) {
			handlers = list_create<void *>();
			tab_add_widget_4_retrieval(tab, HIGHLIGHTING_CHANGED_EVENT_HANDLERS, handlers);
		}
		list_add(handlers, handler);
	} else {
		assert(false);
	}
}

void event_trigger_handlers(const char *event_name, GtkWidget *tab)
{
	LOG_MSG("event_trigger_handlers()\n");

	if (strcmp(event_name, "highlighting-changed") == 0) {
		struct List<void *> *handlers =
			(struct List<void *> *) tab_retrieve_widget(tab, HIGHLIGHTING_CHANGED_EVENT_HANDLERS);
		for (int i = 0; handlers && i < handlers->index; ++i) {
			((void (*) (GtkWidget *)) handlers->data[i])(tab);
		}
	} else {
		assert(false);
	}
}


// * if the highlighting type is not actually changing,
// we catch that in order to avoid doing useless work, but what if someone changes tags?
// wouldnt we want to make those changes?
//@ Simplify?
void set_text_highlighting(GtkWidget *tab, const char *new_highlighting)
{
	LOG_MSG("set_text_highlighting()\n");

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));

	char *old_highlighting = (char *) tab_retrieve_widget(tab, CURRENT_TEXT_HIGHLIGHTING);

	if (old_highlighting
	&& strcmp(old_highlighting, "None") != 0
	&& strcmp(new_highlighting, old_highlighting) != 0) {
		remove_tags(tab);
	}

	if (old_highlighting
	&& strcmp(old_highlighting, "None") != 0
	&& strcmp(new_highlighting, "None") == 0) {
		unregister_handlers(text_buffer);
	}

	if ((!old_highlighting 
	&& strcmp(new_highlighting, "None") != 0)
	||
	(old_highlighting 
	&& strcmp(old_highlighting, "None") == 0 
	&& strcmp(new_highlighting, "None") != 0)) {
		register_handlers(tab, text_buffer);
	}

	if (!old_highlighting && strcmp(new_highlighting, "None") != 0
	|| strcmp(new_highlighting, "None") != 0 && strcmp(new_highlighting, old_highlighting) != 0)
	{
		create_tags(tab, new_highlighting);
//		void *func = table_get(func_table, new_highlighting);
//		if (func) { //@ this should be an assertion, eventually
//			GtkTextIter start, end;
//			gtk_text_buffer_get_bounds(text_buffer, &start, &end);
//			((void (*) (GtkTextBuffer *, GtkTextIter *, GtkTextIter *)) func)(text_buffer, &start, &end);
//		}
		Highlighter highlighter = highlighters_get_func(new_highlighting);
		assert(highlighter);
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds(text_buffer, &start, &end);
		highlighter(text_buffer, &start, &end);
		tab_add_widget_4_retrieval(tab, HIGHLIGHTING_FUNC, (void *) highlighter);
	}

	tab_add_widget_4_retrieval(tab, CURRENT_TEXT_HIGHLIGHTING, strdup(new_highlighting));

	if (old_highlighting) {
		free(old_highlighting);
	}

	event_trigger_handlers("highlighting-changed", tab);
}


void highlighting_current_line_enable(GtkTextBuffer *text_buffer, const char *color)
{
	LOG_MSG("highlighting_current_line_enable()\n");

	// first disable the old thing if it exists
	highlighting_current_line_disable(text_buffer);

	gtk_text_buffer_create_tag(text_buffer, "line-highlight",
		"paragraph-background", color, NULL);

//	gtk_text_buffer_create_tag(text_buffer, "cursor-highlight",
//		"paragraph-background", "red", "foreground", "white", NULL);

	highlighting_text_buffer_cursor_position_changed(G_OBJECT(text_buffer), NULL, NULL);
	unsigned long id = g_signal_connect(G_OBJECT(text_buffer),
		"notify::cursor-position", G_CALLBACK(highlighting_text_buffer_cursor_position_changed), NULL);
	g_object_set_data(G_OBJECT(text_buffer), "highlighting-cursor-position-changed", (void *) id);
}


void highlighting_current_line_disable(GtkTextBuffer *text_buffer)
{
	LOG_MSG("highlighting_current_line_disable()\n");

	GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);
	GtkTextTag* p = gtk_text_tag_table_lookup(table, "line-highlight");
	if (p) {
		gtk_text_tag_table_remove(table, p);
	}

	unsigned long id = (unsigned long) g_object_get_data(G_OBJECT(text_buffer), "highlighting-cursor-position-changed");
	// g_signal_handler_disconnect: assertion 'handler_id > 0' failed
	if (id > 0) {
		g_signal_handler_disconnect(text_buffer, id);
	}
}

void highlighting_current_line_enable_or_disable(struct Node *settings, GtkTextBuffer *text_buffer)
{
	LOG_MSG("highlighting_current_line_enable_or_disable()\n");

	const char *value = settings_get_value(settings, "highlighting/line-highlighting-color");
	if (value) {
		highlighting_current_line_enable(text_buffer, value);
	} else {
		highlighting_current_line_disable(text_buffer);
	}
}

void on_highlighting_selected(GtkMenuItem *item, gpointer data)
{
	LOG_MSG("on_highlighting_selected()\n");

	GtkWidget *tab = (GtkWidget *) data;
	const char *selected_item_text = gtk_menu_item_get_label(item);
	set_text_highlighting(tab, selected_item_text);
}


void on_highlighting_changed(GtkWidget *tab){
	LOG_MSG("on_highlighting_changed()\n");

	// update the button label
	const char *highlighting = (const char *) tab_retrieve_widget(tab, CURRENT_TEXT_HIGHLIGHTING);
	assert(highlighting); // we assume that highlighting is set
	GtkWidget *button_label = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_MENU_BUTTON_LABEL);
	gtk_label_set_text(GTK_LABEL(button_label), highlighting);
}


GtkWidget *new_highlighting_selection_menu(GtkWidget *tab, struct Node *settings)
{
	LOG_MSG("new_highlighting_selection_menu()\n");

	GtkWidget *menu = gtk_menu_new();

	add_menu_item(GTK_MENU(menu), "None", G_CALLBACK(on_highlighting_selected), (void *) tab);
	struct Node *languages = get_node(settings, "highlighting/languages");
	for (int i = 0; languages && i < languages->nodes->i_end; ++i) {
		const char *language_name = ((struct Node *)languages->nodes->data[i])->name;
		add_menu_item(GTK_MENU(menu), language_name, G_CALLBACK(on_highlighting_selected), (void *) tab);
	}

	gtk_widget_show_all(menu);

	return menu;
}


// a widget that changes and reflects current highlighting language
GtkWidget *highlighting_new_menu_button(GtkWidget *tab, struct Node *settings)
{
	LOG_MSG("highlighting_new_menu_button()\n");

	//GtkWidget *label = gtk_label_new("abc");
	GtkWidget *label = gtk_label_new(NULL);
	GtkWidget *menu_button = gtk_menu_button_new();
	gtk_container_add(GTK_CONTAINER(menu_button), label);
	add_class(menu_button, "code-highlighting-menu-button");

	GtkWidget *menu = new_highlighting_selection_menu(tab, settings);
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(menu_button), menu);
	//gtk_menu_button_set_direction(GTK_MENU_BUTTON(menu_button), GTK_ARROW_UP);

	//register_highlighting_changed_event_handler(tab, (void *) on_highlighting_changed);
	event_register_handler("highlighting-changed", (void *) on_highlighting_changed, tab);
	//event_register_handler("doesnotexist", (void *) on_highlighting_changed, tab);

	tab_add_widget_4_retrieval(tab, HIGHLIGHTING_MENU_BUTTON, (void *) menu_button);
	tab_add_widget_4_retrieval(tab, HIGHLIGHTING_MENU_BUTTON_LABEL, (void *) label);
	tab_add_widget_4_retrieval(tab, HIGHLIGHTING_MENU, (void *) menu);

	return menu_button;
}


// if the settings are updated, this brings highlighting-selection-menu up-to-date
void highlighting_update_menu(GtkWidget *tab, struct Node *settings)
{
	LOG_MSG("highlighting_update_selection_menu()\n");

	GtkWidget *old_menu = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_MENU);
	GtkWidget *menu_button = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_MENU_BUTTON);
	//GtkWidget *menu_button_label = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_MENU_BUTTON_LABEL);
	assert(old_menu && menu_button);

	gtk_widget_destroy(old_menu); //@ this?
	GtkWidget *new_menu = new_highlighting_selection_menu(tab, settings);
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(menu_button), new_menu); // I have not consulted the docs

	tab_add_widget_4_retrieval(tab, HIGHLIGHTING_MENU, (void *) new_menu);
}