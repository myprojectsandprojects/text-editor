#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "declarations.h"


//struct Table *func_table; // maps highlighting languages to highlighting functions

void c_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end);
void css_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end);
typedef void (*Highlighter) (GtkTextBuffer *, GtkTextIter *, GtkTextIter *);
struct TableEntry {
	const char *name;				// highlighting name
	Highlighter highlighter;		// function that does the highlighting
};
struct TableEntry highlighters[] = {
	{"C", c_highlight},
	{"CSS", css_highlight},
};
const int highlighters_num_entries = sizeof(highlighters) / sizeof(struct TableEntry);

Highlighter highlighters_get_func(const char *name)
{
	for (int i = 0; i < highlighters_num_entries; ++i) {
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

/*
// do we really have to do this at runtime
void init_highlighting(void)
{
	assert(!func_table);

	func_table = table_create();
	table_store(func_table, "C", (void *) c_highlight);
	table_store(func_table, "Css", (void *) css_highlight);
}
*/

void select_highlighting_based_on_file_extension
	(GtkWidget *tab, struct Node *settings, const char *file_name)
{
	printf("select_highlighting_based_on_file_extension()\n");
	//printf("file name: %s\n", file_name);

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
		printf("%s\n", name);
		
		struct Node *extensions = get_node(n_language, "file-extensions");
		assert(extensions);
		for (int i = 0; i < extensions->nodes->i_end; ++i) {
			struct Node *n_extension = (struct Node *) extensions->nodes->data[i];
			const char *extension = n_extension->name;
			//printf("\t%s\n", extension);
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

void create_tags(GtkWidget *tab, const char *language)
{
	printf("create_tags()\n");
	printf("create_tags: creating tags for \"%s\"\n", language);

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));

	// we store them so that we know what to remove when highlighting language changes
	// initially, remove_tags() looked up its tags from settings,
	// but now, as we are doing the hotloading business, we dont know if the tags have changed..
	// alternatively, we could perhaps remove the tags before updating the settings
	// or just keep the old settings around until everyrhing is updated, somehow
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
		printf("creating tag: %s\n", tag_name);
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
	printf("remove_tags()\n");

	GtkTextBuffer *text_buffer = (GtkTextBuffer *) tab_retrieve_widget(tab, TEXT_BUFFER);
	GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);

	struct CList *tags = (struct CList *) tab_retrieve_widget(tab, HIGHLIGHTING_TAGS);
	for (int i = 0; i < tags->i_end; ++i) {
		gtk_text_tag_table_remove(table, (GtkTextTag *) tags->data[i]);
	}
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
	if (func) { //@ shouldnt this be an assertion? why are we being called, if there is no highlighting?
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
void set_text_highlighting(GtkWidget *tab, const char *new_highlighting)
{
	printf("set_text_highlighting()\n");
	printf("set_text_highlighting: new highlighting: \"%s\"\n", new_highlighting);

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
	printf("highlighting_current_line_enable()\n");

	// first disable the old thing if it exists
	highlighting_current_line_disable(text_buffer);

	gtk_text_buffer_create_tag(text_buffer, "line-highlight",
		"paragraph-background", color,
		NULL);

	//highlighting_text_buffer_cursor_position_changed(G_OBJECT(text_buffer), NULL, NULL);
	unsigned long id = g_signal_connect(G_OBJECT(text_buffer),
		"notify::cursor-position", G_CALLBACK(highlighting_text_buffer_cursor_position_changed), NULL);
	g_object_set_data(G_OBJECT(text_buffer), "highlighting-cursor-position-changed", (void *) id);
}


void highlighting_current_line_disable(GtkTextBuffer *text_buffer)
{
	printf("highlighting_current_line_disable()\n");

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
	const char *value = settings_get_value(settings, "highlighting/line-highlighting-color");
	if (value) {
		printf("enable line-highlighting\n");
		highlighting_current_line_enable(text_buffer, value);
	} else {
		printf("disable line-highlighting\n");
		highlighting_current_line_disable(text_buffer);
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

	// update the button label
	const char *highlighting = (const char *) tab_retrieve_widget(tab, CURRENT_TEXT_HIGHLIGHTING);
	assert(highlighting); // we assume that highlighting is set
	GtkWidget *button_label = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_MENU_BUTTON_LABEL);
	gtk_label_set_text(GTK_LABEL(button_label), highlighting);
}


GtkWidget *new_highlighting_selection_menu(GtkWidget *tab, struct Node *settings)
{
	printf("new_highlighting_selection_menu()\n");

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
	printf("create_highlighting_selection_button()\n");

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
	printf("highlighting_update_selection_menu()\n");

	GtkWidget *old_menu = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_MENU);
	GtkWidget *menu_button = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_MENU_BUTTON);
	//GtkWidget *menu_button_label = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_MENU_BUTTON_LABEL);
	assert(old_menu && menu_button);

	gtk_widget_destroy(old_menu); //@ this?
	GtkWidget *new_menu = new_highlighting_selection_menu(tab, settings);
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(menu_button), new_menu); // I have not consulted the docs

	tab_add_widget_4_retrieval(tab, HIGHLIGHTING_MENU, (void *) new_menu);
}