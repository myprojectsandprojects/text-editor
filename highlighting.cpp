#include <string.h>
#include "declarations.h"

//extern guint gtk_version_major;
//extern guint gtk_version_minor;
//extern guint gtk_version_micro;

static void on_insert_text_after(GtkTextBuffer *text_buffer, GtkTextIter *location, gchar *text, gint len, gpointer tab);
static void on_delete_range_after(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end, gpointer tab);
static void update_highlighting_selection_button(GtkWidget *tab, const char *new_highlighting);

//typedef void (*Highlighter) (GtkTextBuffer *, GtkTextIter *, GtkTextIter *);
typedef void (*Highlighter) (GtkTextBuffer *, GtkTextIter *);

struct TableEntry {
	const char *name;			// highlighting name
	Highlighter highlighter;	// function that does the highlighting
};

TableEntry highlighters[] = {
//	{"C", c_highlight},
	{"C++", cpp_highlight},
//	{"Rust", rust_highlight},
};
//const int highlighters_num_entries = sizeof(highlighters) / sizeof(struct TableEntry);

Highlighter get_highlighter(const char *highlighting_type) {
	for (int i = 0; i < COUNT(highlighters); ++i) {
		if (strcmp(highlighting_type, highlighters[i].name) == 0) {
			return highlighters[i].highlighter;
		}
	}
	return NULL;
}

void highlighting_init(GtkWidget *tab, Node *settings){
	LOG_MSG("highlighting_init()\n");

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));

	Node *text_tags = get_node(settings, "text-tags");

	for(int i = 0; i < text_tags->nodes.count; ++i){
		Node *text_tag = (Node *) text_tags->nodes.data[i];
//		printf("text-tag: %s\n", text_tag->name);

		GtkTextTag *gtk_text_tag = gtk_text_buffer_create_tag(text_buffer, text_tag->name, NULL);

		for (int j = 0; j < text_tag->nodes.count; ++j) {
			Node *attribute_name = (Node *) text_tag->nodes.data[j];
			Node *attribute_value = (Node *) attribute_name->nodes.data[0];
			assert(attribute_value); // attribute has to have a value (for example: "foreground {black}")

// 		style-attribute is a special case, cant use string from the file directly:
//			if (strcmp(attribute_name->name, "style") == 0) {
//				PangoStyle translated_value;
//				if (strcmp(value->name, "PANGO_STYLE_NORMAL") == 0) {
//					translated_value = PANGO_STYLE_NORMAL;
//				} else if (strcmp(value->name, "PANGO_STYLE_ITALIC") == 0) {
//					translated_value = PANGO_STYLE_ITALIC;
//				} else if (strcmp(value->name, "PANGO_STYLE_OBLIQUE") == 0) {
//					translated_value = PANGO_STYLE_OBLIQUE;
//				} else {
//					assert(false); //@ more elegant error handling, inform the user
//				}
//				g_object_set(G_OBJECT(gtk_tag), name->name, translated_value, NULL);
//				continue;
//			}

			g_object_set(G_OBJECT(gtk_text_tag), attribute_name->name, attribute_value->name, NULL);
		}
	}

	// register event-handlers necessary to keep the highlighting uptodate
	g_signal_connect_after(text_buffer, "insert-text", G_CALLBACK(on_insert_text_after), tab);
	g_signal_connect_after(text_buffer, "delete-range", G_CALLBACK(on_delete_range_after), tab);
}

void highlighting_set(GtkWidget *tab, const char *highlighting){
	LOG_MSG("highlighting_set()\n");

	Highlighter f;

	if(strcmp(highlighting, "None") == 0){
		f = NULL;
	}else{
		f = get_highlighter(highlighting);
		if(!f){
//			const char *message = "\n\033[1;31mError: Requested highlighting type (\"%s\") doesnt seem to have a highlighting-function associated with it.\033[0m\nPerhaps you specified a language in the settings-file, but didnt either provide a highlighting-function or didnt mention the highlighting-function in the highlighting-functions-table?\n\n";
//			fprintf(stderr, message, highlighting);

//			const int size = 1000;
//			char message[size];
//			snprintf(message, size, "Error: Requested highlighting type (\"%s\") doesnt seem to have a highlighting-function associated with it.", highlighting);
//			display_error(message, "Perhaps you specified a language in the settings-file, but didnt either provide a highlighting-function or didnt mention the highlighting-function in the highlighting-functions-table?");

			ERROR("Error: Requested highlighting type (\"%s\") doesnt seem to have a highlighting-function associated with it. (Perhaps you specified a language in the settings-file, but didnt either provide a highlighting-function or didnt mention the highlighting-function in the highlighting-functions-table?)", highlighting)
	
			return;
		}
	}

	tab_add_widget_4_retrieval(tab, HIGHLIGHTER, (void *) f);

	GtkTextIter a, b;
	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));
	gtk_text_buffer_get_bounds(text_buffer, &a, &b);
	gtk_text_buffer_remove_all_tags(text_buffer, &a, &b);

	if(f){
//		f(text_buffer, &a, &b);
		long before = get_time_us();
		f(text_buffer, NULL);
		long after = get_time_us();
		long elapsed = after - before;
		printf("initial highlighting took %ld\n", elapsed);
//		print_tags(text_buffer);
	}else{
		assert(strcmp(highlighting, "None") == 0);
	}

	update_highlighting_selection_button(tab, highlighting);
}

void print_tags(GtkTextBuffer *text_buffer){
	GtkTextIter i;
	gtk_text_buffer_get_start_iter(text_buffer, &i);
	while(!gtk_text_iter_is_end(&i)){
		gunichar c = gtk_text_iter_get_char(&i);
		printf("%c", c);
		printf("[");
		GSList *l = gtk_text_iter_get_tags(&i);
		GSList *p = l;
		while(p != NULL){
			GtkTextTag *t = (GtkTextTag *) p->data;
			const char *n;
			g_object_get(t, "name", &n, NULL);
			printf("%s,", n);
			p = p->next;
		}
//		free(l);
		printf("]");

		if(gtk_text_iter_begins_tag(&i, NULL)){
			printf(" begins");
		}

		if(gtk_text_iter_ends_tag(&i, NULL)){
			printf(" ends");
		}

		printf("\n");
		gtk_text_iter_forward_char(&i);
	}
}

static void on_insert_text_after(GtkTextBuffer *text_buffer, GtkTextIter *location, gchar *text, gint len, gpointer tab){
	LOG_MSG("on_insert_text_after()\n");

//	printf("location offset: %d, text: %s, len: %d\n", gtk_text_iter_get_offset(location), text, len);
//	printf("character at location: %c[%d]\n", gtk_text_iter_get_char(location), gtk_text_iter_get_char(location));
	// "location" is pointing at the character after the inserted text

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

//	cpp_highlight(text_buffer, location, NULL);
	Highlighter highlight = (Highlighter) tab_retrieve_widget(GTK_WIDGET(tab), HIGHLIGHTER);
	if (highlight) {
		//@hack begin:
		GtkTextIter start_buffer, end_buffer;
		gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
		gtk_text_buffer_remove_tag_by_name(text_buffer, "line-highlighting", &start_buffer, &end_buffer);
		gtk_text_buffer_remove_tag_by_name(text_buffer, "matching-char-highlighting", &start_buffer, &end_buffer);
		gtk_text_buffer_remove_tag_by_name(text_buffer, "scope-highlighting", &start_buffer, &end_buffer);

		GtkTextIter iter = *location; // changing "location" directly also changes cursor position somehow \_(oo)_/
		gtk_text_iter_backward_chars(&iter, len); // "location" points at the end of the inserted text and we want to be at the beginning
		highlight(text_buffer, &iter);
//		f(text_buffer, &start_buffer, &end_buffer); // highlight the whole buffer

		line_highlighting_on_text_buffer_cursor_position_changed(G_OBJECT(text_buffer), NULL, NULL);
		matching_char_highlighting_on_cursor_position_changed(G_OBJECT(text_buffer), NULL, tab);
		scope_highlighting_on_cursor_position_changed(G_OBJECT(text_buffer), NULL, tab);
		//hack end:
	}
}

static void on_delete_range_after(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end, gpointer tab){
	LOG_MSG("on_delete_range_after()\n");

//	printf("start: %c, end: %c\n", gtk_text_iter_get_char(start), gtk_text_iter_get_char(end));
	assert(gtk_text_iter_compare(start, end) == 0); // should be equal after deletion

//	cpp_highlight(text_buffer, start, NULL);
	Highlighter f = (Highlighter) tab_retrieve_widget(GTK_WIDGET(tab), HIGHLIGHTER);
	if(f){
		//@hack begin:
		GtkTextIter start_buffer, end_buffer;
		gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
		gtk_text_buffer_remove_tag_by_name(text_buffer, "line-highlighting", &start_buffer, &end_buffer);
		gtk_text_buffer_remove_tag_by_name(text_buffer, "matching-char-highlighting", &start_buffer, &end_buffer);
		gtk_text_buffer_remove_tag_by_name(text_buffer, "scope-highlighting", &start_buffer, &end_buffer);
		f(text_buffer, start);
//		f(text_buffer, &start_buffer, &end_buffer); // highlight the whole buffer
		line_highlighting_on_text_buffer_cursor_position_changed(G_OBJECT(text_buffer), NULL, NULL);
		matching_char_highlighting_on_cursor_position_changed(G_OBJECT(text_buffer), NULL, tab);
		scope_highlighting_on_cursor_position_changed(G_OBJECT(text_buffer), NULL, tab);
		//hack end:
	}
}

static void on_highlighting_selected(GtkMenuItem *item, gpointer data){
	LOG_MSG("on_highlighting_selected()\n");

	GtkWidget *tab = (GtkWidget *) data;
	const char *text = gtk_menu_item_get_label(item);
	highlighting_set(tab, text);
}

static void update_highlighting_selection_button(GtkWidget *tab, const char *new_highlighting){
	LOG_MSG("update_highlighting_selection_button()\n");

	GtkWidget *button_label = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_MENU_BUTTON_LABEL);
	gtk_label_set_text(GTK_LABEL(button_label), new_highlighting);
}

static GtkWidget *create_highlighting_selection_menu(Node *settings, GtkWidget *tab){
	LOG_MSG("create_highlighting_selection_menu()\n");

	GtkWidget *menu = gtk_menu_new();

	add_menu_item(GTK_MENU(menu), "None", G_CALLBACK(on_highlighting_selected), (void *) tab);

	Node *languages = get_node(settings, "languages");
	if(languages){
		for (int i = 0; i < languages->nodes.count; ++i){
			Node *language = languages->nodes.data[i];
			add_menu_item(GTK_MENU(menu), language->name, G_CALLBACK(on_highlighting_selected), (void *) tab);
		}
	}

	gtk_widget_show_all(menu);

	return menu;
}

GtkWidget *create_highlighting_selection_button(GtkWidget *tab, Node *settings)
{
	LOG_MSG("create_highlighting_selection_button()\n");

	//GtkWidget *label = gtk_label_new("abc");
	GtkWidget *button_label = gtk_label_new("This is the label");
	GtkWidget *button = gtk_menu_button_new();
	gtk_container_add(GTK_CONTAINER(button), button_label);
	add_class(button, "code-highlighting-menu-button");

	GtkWidget *menu = create_highlighting_selection_menu(settings, tab);
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(button), menu);
	//gtk_menu_button_set_direction(GTK_MENU_BUTTON(menu_button), GTK_ARROW_UP);

//	tab_add_widget_4_retrieval(tab, HIGHLIGHTING_MENU_BUTTON, 			(void *) button);
	tab_add_widget_4_retrieval(tab, HIGHLIGHTING_MENU_BUTTON_LABEL, 	(void *) button_label);
//	tab_add_widget_4_retrieval(tab, HIGHLIGHTING_MENU, 					(void *) menu);

	return button;
}


//// if the settings are updated, this brings highlighting-selection-menu up-to-date
//void highlighting_update_menu(GtkWidget *tab, struct Node *settings)
//{
//	LOG_MSG("highlighting_update_selection_menu()\n");
//
//	GtkWidget *old_menu = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_MENU);
//	GtkWidget *menu_button = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_MENU_BUTTON);
//	//GtkWidget *menu_button_label = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_MENU_BUTTON_LABEL);
//	assert(old_menu && menu_button);
//
//	gtk_widget_destroy(old_menu); //@ this?
//	GtkWidget *new_menu = new_highlighting_selection_menu(tab, settings);
//	gtk_menu_button_set_popup(GTK_MENU_BUTTON(menu_button), new_menu); // I have not consulted the docs
//
//	tab_add_widget_4_retrieval(tab, HIGHLIGHTING_MENU, (void *) new_menu);
//}
