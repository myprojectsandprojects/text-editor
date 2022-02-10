#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "declarations.h"


struct Attribute {
	char *name;
	char *value;
};

struct Tag {
	char *name;
	struct List<struct Attribute *> *attributes;
};

struct Language {
	char *name;
	struct List<struct Tag *> *tags;
};


struct List<struct Language *> *languages;
struct Table *func_table; // names -> highlighting functions

extern GtkWidget *notebook;
extern struct Settings settings;

//@ multiple tabs share these global variables... could this be a problem?
int global_location_offset; // location of last insertion or deletion
int global_length; // length of text inserted or 0 if deletion
char *global_text;


gunichar peek_next_character(GtkTextIter *iter)
{
	gunichar c;
	if (gtk_text_iter_is_end(iter) == TRUE) { // ...So that we are not moving the iterator backwards if already at the end.
		c = gtk_text_iter_get_char(iter); // returns 0 if iter is not dereferenceable
	} else {
		gtk_text_iter_forward_char(iter);
		c = gtk_text_iter_get_char(iter); // returns 0 if iter is not dereferenceable
		gtk_text_iter_backward_char(iter);
	}
	return c;
}

gunichar get_next_character(GtkTextIter *iter)
{
	gtk_text_iter_forward_char(iter);
	gunichar c = gtk_text_iter_get_char(iter); // returns 0 if iter is not dereferenceable
	return c;
}

void c_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end)
{
	printf("c_highlight()\n");

	//char *text = gtk_text_buffer_get_text(text_buffer, start, end, FALSE);
	//LOG_MSG("highlighting: \"%s\"\n", text);
	//free(text);

	//printf("adding highlighting to range: %d -> %d\n", gtk_text_iter_get_offset(start), gtk_text_iter_get_offset(end));

	gtk_text_buffer_remove_all_tags(text_buffer, start, end); // @ shouldnt do that here at all?

	char possible_type_identifier[100];
	possible_type_identifier[0] = 0;
	GtkTextIter i1, i2;

	GtkTextIter iter;
	for (iter = *start;
			gtk_text_iter_compare(&iter, end) != 1 && gtk_text_iter_is_end(&iter) != TRUE;
			gtk_text_iter_forward_char(&iter)) {

		gunichar c1 = gtk_text_iter_get_char(&iter);
		gunichar c2 = peek_next_character(&iter);
		//g_print("%c ", c);

		if (c1 == '/' && c2 == '*') {
			GtkTextIter begin = iter;
			c1 = get_next_character(&iter);
			while (c1 != 0) {
				c1 = get_next_character(&iter);
				c2 = peek_next_character(&iter);
				if (c1 == '*' && c2 == '/') break;
			}
			if (c1 != 0) {
				// comment /* -> */
				gtk_text_iter_forward_chars(&iter, 2);
			}
			gtk_text_buffer_remove_all_tags(text_buffer, &begin, &iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "comment", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		if (c1 == '/' && c2 == '/') {
			GtkTextIter begin = iter;
			gtk_text_iter_forward_line(&iter);
			gtk_text_buffer_remove_all_tags(text_buffer, &begin, &iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "comment", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		//@ range is still not correct for strings...
		if (c1 == '\"') { //@ string can be continued on a new line using backslash, but whatever for now. 
			GtkTextIter begin = iter;
			do {
				c1 = get_next_character(&iter);
				if (c1 == '\"') {
					gtk_text_iter_backward_char(&iter);
					gunichar prev = gtk_text_iter_get_char(&iter);
					if (prev != 92) { // backslash
						gtk_text_iter_forward_char(&iter);
						break;
					}
					gtk_text_iter_forward_char(&iter);
				}
			} while (c1 != '\n' && c1 != 0);
			
			gtk_text_iter_forward_char(&iter);
			gtk_text_buffer_remove_all_tags(text_buffer, &begin, &iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "string-literal", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		if (c1 == '\'') {
			GtkTextIter begin = iter;
			do {
				c1 = get_next_character(&iter);
				if (c1 == '\'') {
					gtk_text_iter_backward_char(&iter);
					gunichar prev = gtk_text_iter_get_char(&iter);
					if (prev != 92) { // backslash
						gtk_text_iter_forward_char(&iter);
						break;
					}
					gtk_text_iter_forward_char(&iter);
				}
			} while (c1 != '\n' && c1 != 0);
			
			gtk_text_iter_forward_char(&iter);
			gtk_text_buffer_remove_all_tags(text_buffer, &begin, &iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "string-literal", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		if (c1 == '#') { // @ something wrong here?
			GtkTextIter begin = iter;
			while (gtk_text_iter_forward_line(&iter)) { // Is there a next line?
				gtk_text_iter_backward_chars(&iter, 2);
				if (gtk_text_iter_get_char(&iter) == 92) {
					gtk_text_iter_forward_chars(&iter, 2);
					continue;
					//gtk_text_iter_forward_line(&iter);
				} else {
					gtk_text_iter_forward_chars(&iter, 2);
					break;
				}
			}
			
			gtk_text_buffer_remove_all_tags(text_buffer, &begin, &iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "preprocessor-directive", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		if (c1 == '+' || c1 == '-' || c1 == '*' || c1 == '/' || c1 == '='
				|| c1 == '(' || c1 == ')' || c1 == '[' || c1 == ']' || c1 == '{' || c1 == '}'
				|| c1 == '<' || c1 == '>' || c1 == '!'  || c1 == '|' || c1 == '~' || c1 == '&'
				|| c1 == ';' || c1 == ',' || c1 == '?' || c1 == ':' || c1 == '.') {
			GtkTextIter begin = iter;
			gtk_text_iter_forward_char(&iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "operator", &begin, &iter);
			gtk_text_iter_backward_char(&iter);

			// it could also be an arithmetic operator
			// we could look at the specific way its used:
			// if ident *ident then we could tend to suspect a pointer and highlight that way
			// all other cases like ident * ident or ident*ident or ident* ident etc. 
			// we would highlight as a multiplication operator 
			if (c1 == '*' && (g_unichar_isalpha(c2) || c2 == '_' || c2 == '*'))
				continue; // if immediately followed by an identifier well assume pointer 

			possible_type_identifier[0] = 0;
			continue;
		}

		if (g_unichar_isdigit(c1)) {
			GtkTextIter begin = iter;
			while(gtk_text_iter_forward_char(&iter) == TRUE) {
				c1 = gtk_text_iter_get_char(&iter);
				if(!g_unichar_isalnum(c1) && c1 != '.') break;
			}
			gtk_text_buffer_apply_tag_by_name(text_buffer, "number-literal", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
			continue;
		}

		if(g_unichar_isalpha(c1) || c1 == '_') {
			GtkTextIter begin = iter;
			while(gtk_text_iter_forward_char(&iter)) {
				c1 = gtk_text_iter_get_char(&iter);
				if(!g_unichar_isalnum(c1) && c1 != '_') break;
			}

			// Maybe our identifier is a keyword?
			gboolean is_keyword = FALSE;
			const char *keywords[] = {
				"if", "else", "return", "for", "while", "break", "continue", "struct", "const", "extern", "static", NULL};

			char *identifier = gtk_text_buffer_get_text(text_buffer, &begin, &iter, FALSE);
			int k;
			for(k = 0; keywords[k] != NULL; ++k) {
				if(strcmp(identifier, keywords[k]) == 0) {
					is_keyword = TRUE;
					break;
				}
			}

// identifier could also identify a type... lets see if we can recognize that...
// lets just assume for now, that if we have an identifier followed by an identifier,
// then the first identifier identifies a type.

			if (is_keyword == TRUE) {
				gtk_text_buffer_apply_tag_by_name(text_buffer, "keyword", &begin, &iter);
			} else {
				if (possible_type_identifier[0] != 0) {
					gtk_text_buffer_apply_tag_by_name(text_buffer, "type", &i1, &i2);
					possible_type_identifier[0] = 0;
					gtk_text_buffer_apply_tag_by_name(text_buffer, "identifier", &begin, &iter);
				} else {
					gtk_text_buffer_apply_tag_by_name(text_buffer, "identifier", &begin, &iter);
					strncpy(possible_type_identifier, identifier, 100);
					i1 = begin; i2 = iter;
				}
			}

			gtk_text_iter_backward_char(&iter);
			free(identifier);
			continue;
		}

		if (c1 != ' ' && c1 != '\t' && c1 != '\n') {
			GtkTextIter begin = iter;
			/*do {
				c1 = get_next_character(&iter);
			} while (c1 != ' ' && c1 != '\t' && c1 != '\n' && c1 != 0);*/
			
			gtk_text_iter_forward_char(&iter);
			gtk_text_buffer_apply_tag_by_name(text_buffer, "unknown", &begin, &iter);
			gtk_text_iter_backward_char(&iter);
		}
	}
}

void css_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end)
{
	printf("css_highlight()\n");
}

void init_highlighting()
{
	assert(!func_table);

	func_table = table_create();
	table_store(func_table, "C", (void *) c_highlight);
	table_store(func_table, "Css", (void *) css_highlight);
}

void create_tags(GtkTextBuffer *text_buffer, const char *language)
{
	printf("create_tags()\n");
	printf("create_tags: creating tags for \"%s\"\n", language);

	for (int i = 0; i < languages->index; ++i) {
		//printf("%s\n", languages->data[i]->name);
		//const char *language_name = languages->data[i]->name;
		struct Language *l = languages->data[i];
		if (strcmp(language, l->name) == 0) {
			printf("create_tags: found our language!\n");
			for (int j = 0; j < l->tags->index; ++j) {
				struct Tag *t = l->tags->data[j];
				printf("create_tags: creating tag \"%s\"\n", t->name);
				GtkTextTag *tag = gtk_text_buffer_create_tag(text_buffer, t->name, NULL);
				for (int k = 0; k < t->attributes->index; ++k) {
					/*
					if (strcmp(attributes[i]->property, "style") == 0) {
						PangoStyle translated_value;
						if (strcmp(attributes[i]->value, "normal") == 0) {
							translated_value = PANGO_STYLE_NORMAL;
						} else if (strcmp(attributes[i]->value, "italic") == 0) {
							translated_value = PANGO_STYLE_ITALIC;
						} else if (strcmp(attributes[i]->value, "oblique") == 0) {
							translated_value = PANGO_STYLE_OBLIQUE;
						} else {
							assert(0);
						}
						g_object_set(G_OBJECT(t), attributes[i]->property, translated_value, NULL);
						continue;
					}
					*/
					g_object_set(G_OBJECT(tag), t->attributes->data[k]->name, t->attributes->data[k]->value, NULL);
				}
			}
			return;
		}
	}
	assert(false);
}

void remove_tags(GtkTextBuffer *text_buffer, const char *language)
{
	printf("remove_tags()\n");

	for (int i = 0; i < languages->index; ++i) {
		//printf("%s\n", languages->data[i]->name);
		//const char *language_name = languages->data[i]->name;
		struct Language *l = languages->data[i];
		if (strcmp(language, l->name) == 0) {
			// found the language
			GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);
			for (int j = 0; j < l->tags->index; ++j) {
				const char *tag_name = l->tags->data[j]->name;
				printf("remove_tags: deleting tag \"%s\"\n", tag_name);
				GtkTextTag* t = gtk_text_tag_table_lookup(table, tag_name);
				gtk_text_tag_table_remove(table, t);
			}
			return;
		}
	}
	assert(false);
}

/* 
	Maybe consider highlighting by line
	or maybe highlight 1 statement at a time (;)?
	Block-comments begin and end sequences potentially affect a whole file, but these seem to be an exception.
	Strings can span across multiple lines when backslashed. Preprocessor directives as well.
*/
void on_text_buffer_changed_for_highlighting(GtkTextBuffer *buffer, gpointer data)
{
	LOG_MSG("on_text_buffer_changed_for_highlighting()\n");

	/*GtkTextIter abs_start, abs_end;
	gtk_text_buffer_get_bounds(buffer, &abs_start, &abs_end);
	highlight(buffer, &abs_start, &abs_end);
	return;*/

	GtkTextIter iter, start, end;

	if (strchr(global_text, '\"') || strchr(global_text, '\\')) { // backslash could escape a doublequote, hence
		GtkTextIter text_start, text_end;
		gtk_text_buffer_get_iter_at_offset(buffer, &text_start, global_location_offset);
		gtk_text_buffer_get_iter_at_offset(buffer, &text_end, global_location_offset + global_length);
g_print("before\n");
		if (gtk_text_iter_get_char(&text_start) == '\n') goto TOKEN_RANGE;
		gtk_text_iter_forward_char(&text_start);
		if (gtk_text_iter_get_char(&text_start) == '\n') goto TOKEN_RANGE;
		gtk_text_iter_backward_char(&text_start);
g_print("after\n");
		for (; !gtk_text_iter_is_start(&text_start); gtk_text_iter_backward_char(&text_start)) {
			if (gtk_text_iter_get_char(&text_start) == '\n') break;
		}
		for (; !gtk_text_iter_is_end(&text_end); gtk_text_iter_forward_char(&text_end)) {
			if (gtk_text_iter_get_char(&text_end) == '\n') break;
		}

		void *func = tab_retrieve_widget((GtkWidget *) data, HIGHLIGHTING_FUNC);
		if (func) {
			((void (*) (GtkTextBuffer *, GtkTextIter *, GtkTextIter *)) func)(buffer, &text_start, &text_end);
		}
		
		return;
	}

TOKEN_RANGE:
	gtk_text_buffer_get_iter_at_offset(buffer, &iter, global_location_offset);

	//g_print("offset: %d\n", offset);

	int count = 0;
	start = iter;
	gtk_text_iter_backward_char(&start);
	while (gtk_text_iter_backward_char(&start)) {
		if (gtk_text_iter_begins_tag(&start, NULL)) {
			if (count > 0) break;
			count += 1;
		}
	}

	count = 0;
	end = iter;
	while (gtk_text_iter_forward_char(&end)) {
		if (gtk_text_iter_ends_tag(&end, NULL)) {
			if (count > 1) break; // weve seen tag end more than once!
			count += 1;
		}
	}

	/*GtkTextIter abs_start, abs_end;
	gtk_text_buffer_get_bounds(buffer, &abs_start, &abs_end);
	print_tags(buffer, &abs_start, &abs_end);*/

	void *func = tab_retrieve_widget((GtkWidget *) data, HIGHLIGHTING_FUNC);
	if (func) {
		((void (*) (GtkTextBuffer *, GtkTextIter *, GtkTextIter *)) func)(buffer, &start, &end);
	}

	//print_tags(buffer, &abs_start, &abs_end);

	/*GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	add_highlighting(buffer, &start, &end);*/
}

void on_text_buffer_insert_text_for_highlighting(GtkTextBuffer *buffer, GtkTextIter *location, char *text, int length, gpointer data)
{
	LOG_MSG("on_text_buffer_insert_text_for_highlighting() called!\n");
	/*GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	const char *contents = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);*/
	//g_print("contents: %s\n", contents);

	global_location_offset = gtk_text_iter_get_offset(location);
	global_length = length;
	global_text = text;
}

void on_text_buffer_delete_range_for_highlighting(GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end)
{
	LOG_MSG("on_text_buffer_delete_range_for_highlighting()\n");
	global_location_offset = gtk_text_iter_get_offset(start);
	global_text = gtk_text_buffer_get_text(buffer, start, end, FALSE);
	global_length = 0;
}

void highlighting_text_buffer_cursor_position_changed(GObject *object, GParamSpec *pspec, gpointer user_data)
{
	LOG_MSG("highlighting_text_buffer_cursor_position_changed()\n");

	// Line highlighting -- it messes up code highlighting.
	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(object);
	int position;
	g_object_get(G_OBJECT(text_buffer), "cursor-position", &position, NULL);
	GtkTextIter i;
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(text_buffer), &i, position);
	GtkTextIter start, end, start_buffer, end_buffer;
	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
	gtk_text_buffer_remove_tag_by_name(text_buffer, "line-highlight", &start_buffer, &end_buffer);
	gtk_text_iter_set_line_offset(&i, 0);
	start = i;
	gtk_text_iter_forward_line(&i);
	//gtk_text_iter_forward_char(&i);
	//gtk_text_iter_backward_char(&i);
	end = i;
	//printf("applying the tag: %d, %d\n", gtk_text_iter_get_offset(&start), gtk_text_iter_get_offset(&end));
	gtk_text_buffer_apply_tag_by_name(text_buffer, "line-highlight", &start, &end);
}


void register_handlers(GtkWidget *tab, GtkTextBuffer *text_buffer)
{
	printf("register_handlers()\n");

	unsigned long ids;

	ids = g_signal_connect(text_buffer, "changed", G_CALLBACK(on_text_buffer_changed_for_highlighting), (void *) tab);
	g_object_set_data(G_OBJECT(text_buffer), "changed-handler-4-highlighting", (void *) ids);

	ids = g_signal_connect(text_buffer, "insert-text", G_CALLBACK(on_text_buffer_insert_text_for_highlighting), NULL);
	g_object_set_data(G_OBJECT(text_buffer), "insert-text-handler-4-highlighting", (void *) ids);

	ids = g_signal_connect(text_buffer, "delete-range", G_CALLBACK(on_text_buffer_delete_range_for_highlighting), NULL);
	g_object_set_data(G_OBJECT(text_buffer), "delete-range-handler-4-highlighting", (void *) ids);
}


void unregister_handlers(GtkTextBuffer *text_buffer)
{
	printf("unregister_handlers()\n");

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
	printf("event_register_handler()\n");

	if (strcmp(event_name, "highlighting-changed") == 0) {
		printf("todo: register handler for \"highlighting-changed\"\n");
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
	printf("event_trigger_handlers()\n");

	if (strcmp(event_name, "highlighting-changed") == 0) {
		printf("event_trigger_handlers: todo: trigger \"highlighting-changed\" handlers\n");
		struct List<void *> *handlers =
			(struct List<void *> *) tab_retrieve_widget(tab, HIGHLIGHTING_CHANGED_EVENT_HANDLERS);
		for (int i = 0; i < handlers->index; ++i) {
			((void (*) (GtkWidget *)) handlers->data[i])(tab);
		}
	} else {
		assert(false);
	}
}


// * if the highlighting type is not actually changing,
// we catch that in order to avoid doing useless work, but what if someone changes tags?
// wouldnt we want to make those changes?
void set_text_highlighting(GtkWidget *tab, const char *new_highlighting)
{
	printf("set_text_highlighting()\n");
	printf("set_text_highlighting: new highlighting: \"%s\"\n", new_highlighting);

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));

	char *old_highlighting = (char *) tab_retrieve_widget(tab, CURRENT_TEXT_HIGHLIGHTING);

	if (old_highlighting
	&& strcmp(old_highlighting, "None") != 0
	&& strcmp(new_highlighting, old_highlighting) != 0) {
		remove_tags(text_buffer, old_highlighting);
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
	||
	strcmp(new_highlighting, "None") != 0
	&& strcmp(new_highlighting, old_highlighting) != 0) {
		create_tags(text_buffer, new_highlighting);
		GtkTextIter start, end;
		void *func = table_get(func_table, new_highlighting);
		if (func) {
			gtk_text_buffer_get_bounds(text_buffer, &start, &end);
			((void (*) (GtkTextBuffer *, GtkTextIter *, GtkTextIter *)) func)(text_buffer, &start, &end);
		}
		tab_add_widget_4_retrieval(tab, HIGHLIGHTING_FUNC, func);
	}

	tab_add_widget_4_retrieval(tab, CURRENT_TEXT_HIGHLIGHTING, strdup(new_highlighting));

	if (old_highlighting) {
		free(old_highlighting);
	}

	event_trigger_handlers("highlighting-changed", tab);
	//event_trigger_handlers("doesnotexist", tab);
}


void set_current_line_highlighting(GtkTextBuffer *text_buffer, int to_what)
{
	printf("highlighting_init_current_line()\n");

	if (to_what == ON) {
		// current line highlighting ON for the text buffer

		gtk_text_buffer_create_tag(text_buffer,
			"line-highlight", "paragraph-background", settings.line_highlight_color, NULL);

		//highlighting_text_buffer_cursor_position_changed(G_OBJECT(text_buffer), NULL, NULL);
		unsigned long id = g_signal_connect(G_OBJECT(text_buffer),
			"notify::cursor-position", G_CALLBACK(highlighting_text_buffer_cursor_position_changed), NULL);

		g_object_set_data(G_OBJECT(text_buffer), "highlighting-cursor-position-changed", (void *) id);

	} else if (to_what == OFF) {
		// current line highlighting OFF for the text buffer

		GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);
		GtkTextTag* p = gtk_text_tag_table_lookup(table, "line-highligh");
		if (p) {
			gtk_text_tag_table_remove(table, p);
		}

		unsigned long id = (unsigned long) g_object_get_data(G_OBJECT(text_buffer), "highlighting-cursor-position-changed");
		// g_signal_handler_disconnect: assertion 'handler_id > 0' failed
		if (id > 0) {
			g_signal_handler_disconnect(text_buffer, id);
		}
	}
}


void on_highlighting_selected(GtkMenuItem *item, gpointer data)
{
	printf("on_highlighting_selected()\n");

	GtkWidget *tab = (GtkWidget *) data;
	const char *selected_item_text = gtk_menu_item_get_label(item);
	set_text_highlighting(tab, selected_item_text);
}


void on_highlighting_changed(GtkWidget *tab)
{
	printf("on_highlighting_changed()\n");

	// update the button

	const char *highlighting = (const char *) tab_retrieve_widget(tab, CURRENT_TEXT_HIGHLIGHTING);
	assert(highlighting); // we assume that highlighting is set
	GtkWidget *button_label = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_BUTTON_LABEL);
	gtk_label_set_text(GTK_LABEL(button_label), highlighting);
}


// a widget that changes and reflects current highlighting language
GtkWidget *create_highlighting_selection_button(GtkWidget *tab)
{
	printf("create_highlighting_selection_button()\n");

	GtkWidget *hl_label = gtk_label_new("abc");
	//GtkWidget *hl_label = gtk_label_new(NULL);
	GtkWidget *hl_menu_button = gtk_menu_button_new();
	gtk_container_add(GTK_CONTAINER(hl_menu_button), hl_label);
	add_class(hl_menu_button, "code-highlighting-menu-button");

	tab_add_widget_4_retrieval(tab, HIGHLIGHTING_BUTTON_LABEL, (void *) hl_label);
	
	GtkWidget *menu = gtk_menu_new();

	add_menu_item(GTK_MENU(menu), "None", G_CALLBACK(on_highlighting_selected), (void *) tab);
	for (int i = 0; i < languages->index; ++i) {
		const char *language_name = languages->data[i]->name;
		//printf("todo: create menu item for \"%s\"\n", language_name);
		add_menu_item(GTK_MENU(menu), language_name, G_CALLBACK(on_highlighting_selected), (void *) tab);
	}

	gtk_widget_show_all(menu);

	gtk_menu_button_set_popup(GTK_MENU_BUTTON(hl_menu_button), menu);
	//gtk_menu_button_set_direction(GTK_MENU_BUTTON(hl_menu_button), GTK_ARROW_UP);

	//register_highlighting_changed_event_handler(tab, (void *) on_highlighting_changed);
	event_register_handler("highlighting-changed", (void *) on_highlighting_changed, tab);
	//event_register_handler("doesnotexist", (void *) on_highlighting_changed, tab);

	return hl_menu_button;
}


//@ error handling
// * if a line from the file has excessive stuff after our stuff, then we dont detect that
void parse_text_tags_file(void)
{
	printf("parse_text_tags_file()\n");

	if (languages) {
		/*
		for (int i = 0; i < languages->index; ++i) {
			printf("%d -> \"%s\"\n", i, languages->data[i]);
		}
		*/
		free(languages); //@ this by no means actually frees anything
	}
	languages = list_create<struct Language *>();
	struct Language *current_language = NULL;
	struct Tag *current_tag = NULL;

	char *contents = read_file("/home/eero/all/text-editor/themes/text-highlighting-settings");
	//printf("parse_text_tags_file: contents: %s\n", contents);
	while (char *line = get_slice_by(&contents, '\n')) {
		//printf("parse_text_tags_file: line: %s\n", line);

		// count whitespace
		int whitespace_count;
		{
			int i = 0;
			while (line[i] == ' ' || line[i] == '\t') i += 1;
			line = &(line[i]);
			whitespace_count = i;
		}
		//printf("*** %s (%d)\n", line, whitespace_count);

		// strip comments and empty lines
		{
			int i = 0;
			while (*(line + i) != '#' && *(line + i) != '\0') i += 1;
			if (i == 0) {
				// either an empty line or a comment
				//printf("*** empty line or a comment\n\n");
				continue;
			}
			line[i] = '\0';
		}

		char *language_name, *tag_name/*, *property_name, *value*/;
		char *copy = strdup(line);
		
		if (whitespace_count == 0) {
			// expecting a language name
			char *language_name = get_word_with_allocate(&line);
			//printf("*** language name: %s\n\n", language_name);
			struct Language *l = (struct Language *) malloc(sizeof(struct Language));
			l->name = language_name;
			l->tags = list_create<struct Tag *>();
			list_add(languages, l);
			current_language = l;
		} else if (whitespace_count == 1) {
			// expecting a tag name
			char *tag_name = get_word_with_allocate(&line);
			//printf("*** tag name: %s\n\n", tag_name);
			if (!current_language) {
				printf("%s -> cant specify a tag without language being specified first\n\n", copy);
				continue;
			}
			struct Tag *t = (struct Tag *) malloc(sizeof(struct Tag));
			t->name = tag_name;
			t->attributes = list_create<struct Attribute *>();
			list_add(current_language->tags, t);
			current_tag = t;
		} else if (whitespace_count == 2) {
			// expecting a property-value pair
			char *attribute_name = get_word_with_allocate(&line);
			if(!attribute_name) {
				printf("%s -> expecting attribute name!\n", copy);
				continue;
			}
			//printf("*** property name: %s\n", attribute_name);
			char *attribute_value = get_word_with_allocate(&line);
			if(!attribute_value) {
				printf("%s -> expecting attribute value!\n", copy);
				continue;
			}
			//printf("*** value: %s\n\n", attribute_value);
			if (!current_tag) {
				printf("%s -> cant specify an attribute without tag being specified first\n\n", copy);
				continue;
			}
			struct Attribute *a = (struct Attribute *) malloc(sizeof(struct Attribute));
			a->name = attribute_name;
			a->value = attribute_value;
			list_add(current_tag->attributes, a);
		} else {
			// unrecognized number of whitespace characters
			printf("%s -> unrecognized number of whitespace characters!\n\n", copy);
		}

		free(copy);
	}

	/*
	for (int i = 0; i < languages->index; ++i) {
		printf("%s\n", languages->data[i]->name);
		for (int j = 0; j < languages->data[i]->tags->index; ++j) {
			printf(" %s\n", languages->data[i]->tags->data[j]->name);
			for (int k = 0; k < languages->data[i]->tags->data[j]->attributes->index; ++k) {
				printf("  %s, %s\n",
					languages->data[i]->tags->data[j]->attributes->data[k]->name,
					languages->data[i]->tags->data[j]->attributes->data[k]->value);
			}
		}
	}
	*/
}