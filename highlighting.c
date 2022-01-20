#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "declarations.h"
//#include "tab.h"

extern GtkWidget *notebook;

//@ multiple tabs share these global variables... could this be a problem?
int global_location_offset; // location of last insertion or deletion
int global_length; // length of text inserted or 0 if deletion
char *global_text;


extern struct Settings settings;


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


void CSS_create_tags(GtkTextBuffer *text_buffer)
{
	printf("CSS_create_tags()\n");
	printf("CSS_create_tags: todo\n");
}


void CSS_remove_tags(GtkTextBuffer *text_buffer)
{
	printf("CSS_remove_tags()\n");
	printf("CSS_remove_tags: todo\n");
}


struct TagAttribute {
	const char *property;
	const char *value;
};


void C_create_tags(GtkTextBuffer *text_buffer)
{
	printf("C_create_tags()\n");

	gtk_text_buffer_create_tag(text_buffer, "identifier", "foreground", settings.identifier_color, NULL);
/*
	gtk_text_buffer_create_tag(text_buffer, "keyword", "foreground", settings.keyword_color,
		"weight", "bold", NULL);
*/
	gtk_text_buffer_create_tag(text_buffer, "keyword", "foreground", settings.keyword_color, NULL);
/*
	struct TagAttribute *attributes[100];

	//struct TagAttribute *attribute = (struct TagAttribute *) malloc(sizeof(struct TagAttribute));
	//attributes[0] = attribute;
	attributes[0] = (struct TagAttribute *) malloc(sizeof(struct TagAttribute));
	attributes[1] = (struct TagAttribute *) malloc(sizeof(struct TagAttribute));
	attributes[2] = (struct TagAttribute *) malloc(sizeof(struct TagAttribute));
	attributes[3] = NULL;

	attributes[0]->property = "foreground";
	attributes[0]->value = "white";
	attributes[1]->property = "background";
	attributes[1]->value = "black";
	attributes[2]->property = "style";
	attributes[2]->value = "italic";

	GtkTextTag *t = gtk_text_buffer_create_tag(text_buffer, "keyword", NULL);
	for (int i = 0; attributes[i] != NULL; ++i) {
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
		g_object_set(G_OBJECT(t), attributes[i]->property, attributes[i]->value, NULL);
	}
*/	
	gtk_text_buffer_create_tag(text_buffer, "type", "foreground", settings.type_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "operator", "foreground", settings.operator_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "number", "foreground", settings.number_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "comment", "foreground", settings.comment_color, NULL);
/*
	gtk_text_buffer_create_tag(text_buffer, "preprocessor-directive", "foreground", settings.preproccessor_color,
		"weight", "bold", NULL);
*/
	gtk_text_buffer_create_tag(text_buffer, "preprocessor-directive", "foreground", settings.preproccessor_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "unknown", "foreground", settings.unknown_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "string", "foreground", settings.string_color, NULL);
}


void C_remove_tags(GtkTextBuffer *text_buffer)
{
	printf("C_remove_tags()\n");
	printf("C_remove_tags: todo\n");

	/*
	{
		// testing:
		printf("*** creating the test-tag!\n");
		gtk_text_buffer_create_tag(text_buffer, "test-tag", "foreground", "red", NULL);
		//GtkTextIter start, end;
		//gtk_text_buffer_get_bounds(text_buffer, &start, &end);
		//gtk_text_buffer_remove_all_tags(text_buffer, &start, &end);
		GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);
		GtkTextTag* t = gtk_text_tag_table_lookup(table, "test-tag");
		gtk_text_tag_table_remove(table, t);
		gtk_text_buffer_create_tag(text_buffer, "test-tag", "foreground", "red", NULL);
	}
	*/

	GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);
	/*
	void remove_tag(GtkTextTag *tag, gpointer data) {
		GtkTextTagTable *table = (GtkTextTagTable *) data;
		gtk_text_tag_table_remove(table, tag);
	}
	gtk_text_tag_table_foreach(table, (GtkTextTagTableForeach) remove_tag, (gpointer) table);
	// GLib-CRITICAL **: g_hash_table_foreach: assertion 'version == hash_table->version' failed
	*/

	GtkTextTag* p;
	p = gtk_text_tag_table_lookup(table, "identifier");
	gtk_text_tag_table_remove(table, p);
	p = gtk_text_tag_table_lookup(table, "keyword");
	gtk_text_tag_table_remove(table, p);
	p = gtk_text_tag_table_lookup(table, "type");
	gtk_text_tag_table_remove(table, p);
	p = gtk_text_tag_table_lookup(table, "operator");
	gtk_text_tag_table_remove(table, p);
	p = gtk_text_tag_table_lookup(table, "number");
	gtk_text_tag_table_remove(table, p);
	p = gtk_text_tag_table_lookup(table, "comment");
	gtk_text_tag_table_remove(table, p);
	p = gtk_text_tag_table_lookup(table, "preprocessor-directive");
	gtk_text_tag_table_remove(table, p);
	p = gtk_text_tag_table_lookup(table, "unknown");
	gtk_text_tag_table_remove(table, p);
	p = gtk_text_tag_table_lookup(table, "string");
	gtk_text_tag_table_remove(table, p);
}


void CSS_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end)
{
	printf("CSS_highlight()\n");
}


void C_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end)
{
	printf("C_highlight()\n");

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
			gtk_text_buffer_apply_tag_by_name(text_buffer, "string", &begin, &iter);
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
			gtk_text_buffer_apply_tag_by_name(text_buffer, "string", &begin, &iter);
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
			gtk_text_buffer_apply_tag_by_name(text_buffer, "number", &begin, &iter);
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


/*
void C_create_tags(GtkTextBuffer *text_buffer)
{
	LOG_MSG("create_tags()\n");

	// check if we already have the tags to avoid gtk warnings..
	GtkTextTagTable *table = gtk_text_buffer_get_tag_table(text_buffer);
	int size = gtk_text_tag_table_get_size(table);
	if (size != 0) {
		//LOG_MSG("create_tags(): tags already created. no need to create them.\n");
		LOG_MSG("\ttags already created. no need to create them.\n");
		return;
	}

	LOG_MSG("\tcreating tags.\n");

	gtk_text_buffer_create_tag(text_buffer, "identifier", "foreground", settings.identifier_color, NULL);
	//gtk_text_buffer_create_tag(text_buffer, "keyword", "foreground", settings.keyword_color, "weight", "bold", NULL);
	gtk_text_buffer_create_tag(text_buffer, "keyword", "foreground", settings.keyword_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "type", "foreground", settings.type_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "operator", "foreground", settings.operator_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "number", "foreground", settings.number_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "comment", "foreground", settings.comment_color, NULL);
	//gtk_text_buffer_create_tag(text_buffer, "preprocessor-directive", "foreground", settings.preproccessor_color, "weight", "bold", NULL);
	gtk_text_buffer_create_tag(text_buffer, "preprocessor-directive", "foreground", settings.preproccessor_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "unknown", "foreground", settings.unknown_color, NULL);
	gtk_text_buffer_create_tag(text_buffer, "string", "foreground", settings.string_color, NULL);
}
*/


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

		{
			GtkWidget *tab = (GtkWidget *) data;
			int highlighting = *((int *) tab_retrieve_widget(tab, CURRENT_TEXT_HIGHLIGHTING));
			if (highlighting == C) {
				C_highlight(buffer, &text_start, &text_end);
			} else if (highlighting == CSS) {
				CSS_highlight(buffer, &text_start, &text_end);
			}
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

	{
		GtkWidget *tab = (GtkWidget *) data;
		int highlighting = *((int *) tab_retrieve_widget(tab, CURRENT_TEXT_HIGHLIGHTING));
		if (highlighting == C) {
			C_highlight(buffer, &start, &end);
		} else if (highlighting == CSS) {
			CSS_highlight(buffer, &start, &end);
		}
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

/*
void init_highlighting(GtkTextBuffer *text_buffer)
{
	LOG_MSG("init_highlighting()\n");
	create_tags(text_buffer);
	GtkTextIter start_buffer, end_buffer;
	gtk_text_buffer_get_bounds(text_buffer, &start_buffer, &end_buffer);
	highlight(text_buffer, &start_buffer, &end_buffer);


	unsigned long ids;

	ids = g_signal_connect(text_buffer, "changed", G_CALLBACK(on_text_buffer_changed_for_highlighting), NULL);
	g_object_set_data(G_OBJECT(text_buffer), "changed-handler-4-highlighting", (void *) ids);

	ids = g_signal_connect(text_buffer, "insert-text", G_CALLBACK(on_text_buffer_insert_text_for_highlighting), NULL);
	g_object_set_data(G_OBJECT(text_buffer), "insert-text-handler-4-highlighting", (void *) ids);

	ids = g_signal_connect(text_buffer, "delete-range", G_CALLBACK(on_text_buffer_delete_range_for_highlighting), NULL);
	g_object_set_data(G_OBJECT(text_buffer), "delete-range-handler-4-highlighting", (void *) ids);
}

void remove_highlighting(GtkTextBuffer *text_buffer)
{
	LOG_MSG("remove_highlighting()\n");
	GtkTextIter range_start, range_end;
	gtk_text_buffer_get_bounds(text_buffer, &range_start, &range_end);
	gtk_text_buffer_remove_all_tags(text_buffer, &range_start, &range_end);


	// remove signal handlers
	unsigned long ids;

	ids = (unsigned long) g_object_get_data(G_OBJECT(text_buffer), "changed-handler-4-highlighting");
	g_signal_handler_disconnect(text_buffer, ids);

	ids = (unsigned long) g_object_get_data(G_OBJECT(text_buffer), "insert-text-handler-4-highlighting");
	g_signal_handler_disconnect(text_buffer, ids);

	ids = (unsigned long) g_object_get_data(G_OBJECT(text_buffer), "delete-range-handler-4-highlighting");
	g_signal_handler_disconnect(text_buffer, ids);
}
*/


/*
void on_highlighting_selected(GtkMenuItem *item, gpointer data)
{
	LOG_MSG("on_highlighting_selected()\n");

	const char *selected_item = gtk_menu_item_get_label(item);
	//printf("label: %s\n", selected_item);
	GtkLabel *button_label = GTK_LABEL(data);

	const char *button_label_text = gtk_label_get_text(button_label);
	if (strcmp(button_label_text, selected_item) == 0) {
		LOG_MSG("on_highlighting_selected(): highlighting option already selected. no need to do anything\n");
		return;
	}

	gtk_label_set_text(button_label, selected_item);

	// instead of visible_tab_retrieve_widget() maybe we should have something that gives us a tab
	//to which the widget we already have belongs to.
	// because we have a button label and we want the tab it belongs to. or really we want the buffer
	GtkTextBuffer *text_buffer = (GtkTextBuffer *) visible_tab_retrieve_widget(GTK_NOTEBOOK(notebook), TEXT_BUFFER);
	if (strcmp(selected_item, "None") == 0) {
		LOG_MSG("on_highlighting_selected(): removing highlighting...\n");
		remove_highlighting(text_buffer);
	} else {
		LOG_MSG("on_highlighting_selected(): initializing highlighting...\n");
		init_highlighting(text_buffer);
	}
}
*/


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


#define MAX_HANDLERS 3
/*
void *highlighting_changed_event_handlers[MAX_HANDLERS]; // assume all elements are initialized to NULL
int handlers_index; // assume initialized to 0
*/

void register_highlighting_changed_event_handler(GtkWidget *tab, void *handler)
{
	printf("register_highlighting_changed_event_handler()\n");

	void **handlers = (void **) tab_retrieve_widget(tab, HIGHLIGHTING_CHANGED_EVENT_HANDLERS);
	int *p_index = (int *) tab_retrieve_widget(tab, HIGHLIGHTING_CHANGED_EVENT_HANDLERS_INDEX);
	if (!handlers) {
		assert(p_index == NULL);
		handlers = (void **) malloc(MAX_HANDLERS * sizeof(void *));
		p_index = (int *) malloc(sizeof(int));
		*p_index = 0;
		tab_add_widget_4_retrieval(tab, HIGHLIGHTING_CHANGED_EVENT_HANDLERS, handlers);
		tab_add_widget_4_retrieval(tab, HIGHLIGHTING_CHANGED_EVENT_HANDLERS_INDEX	, p_index);
	}

	assert(*p_index < MAX_HANDLERS);
	handlers[*p_index] = handler;
	*p_index += 1;
}

void trigger_highlighting_changed_event_handlers(GtkWidget *tab)
{
	printf("trigger_highlighting_changed_event_handlers()\n");

	void **handlers = (void **) tab_retrieve_widget(tab, HIGHLIGHTING_CHANGED_EVENT_HANDLERS);
	int *p_index = (int *) tab_retrieve_widget(tab, HIGHLIGHTING_CHANGED_EVENT_HANDLERS_INDEX);

	if (!handlers) {
		printf("trigger_highlighting_changed_event_handlers: no handlers registered\n");
		return;
	}

	for (int i = 0; i < *p_index; ++i) {
		((void (*) (GtkWidget *)) handlers[i])(tab);
	}
}


//@ i wouldnt be suprised if something is off here
void set_text_highlighting(GtkWidget *tab, int new_highlighting)
{
	printf("set_text_highlighting()\n");

	int is_1st; // we rely on GTK's TRUE/FALSE here pff.
	int old_highlighting;
	void *data;

	GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(tab_retrieve_widget(tab, TEXT_BUFFER));

	if ((data = tab_retrieve_widget(tab, CURRENT_TEXT_HIGHLIGHTING)) == NULL) {
		is_1st = TRUE;
		data = malloc(sizeof(int));
		tab_add_widget_4_retrieval(tab, CURRENT_TEXT_HIGHLIGHTING, data);
	} else {
		is_1st = FALSE;
		old_highlighting = *((int *) data);
	}
	*((int *) data) = new_highlighting;

	if (!is_1st) {
		if (old_highlighting == C && new_highlighting != C) {
			C_remove_tags(text_buffer);
		} else if (old_highlighting == CSS && new_highlighting != CSS) {
			CSS_remove_tags(text_buffer);
		}
	}

	if (!is_1st && old_highlighting != NONE && new_highlighting == NONE) {
		unregister_handlers(text_buffer);
	}

	if ((is_1st && new_highlighting != NONE) || (!is_1st && old_highlighting == NONE && new_highlighting != NONE)) {
		register_handlers(tab, text_buffer);
	}

	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(text_buffer, &start, &end);

	if (new_highlighting == C && old_highlighting != C) {
		C_create_tags(text_buffer);
		C_highlight(text_buffer, &start, &end);
	} else if (new_highlighting == CSS && old_highlighting != CSS) {
		CSS_create_tags(text_buffer);
		CSS_highlight(text_buffer, &start, &end);
	}

	// call handlers for the "highlighting-changed" event
	/*
	for (int i = 0; highlighting_changed_event_handlers[i] != NULL; ++i) {
		((void (*) (void)) highlighting_changed_event_handlers[i])();
	}
	*/
	trigger_highlighting_changed_event_handlers(tab);
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
	//struct TabInfo *tab_info = (struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	//printf("on_highlighting_selected: tab-id: %d\n", tab_info->id);

	const char *selected_item = gtk_menu_item_get_label(item);
	//printf("label: %s\n", selected_item);
	//GtkLabel *button_label = (GtkLabel *) tab_retrieve_widget(tab, HIGHLIGHTING_BUTTON_LABEL);
/*
	const char *button_label_text = gtk_label_get_text(button_label);
	if (strcmp(button_label_text, selected_item) == 0) {
		LOG_MSG("on_highlighting_selected(): highlighting option already selected. no need to do anything\n");
		return;
	}
*/
	//gtk_label_set_text(button_label, selected_item);

	if (strcmp(selected_item, "None") == 0) {
		set_text_highlighting(tab, NONE);
	} else if (strcmp(selected_item, "C") == 0) {
		set_text_highlighting(tab, C);
	} else if (strcmp(selected_item, "CSS") == 0) {
		set_text_highlighting(tab, CSS);
	}
}


void on_highlighting_changed(GtkWidget *tab)
{
	printf("on_highlighting_changed()\n");

	// update the button

	int highlighting = *((int *) tab_retrieve_widget(tab, CURRENT_TEXT_HIGHLIGHTING));
	GtkWidget *button_label = (GtkWidget *) tab_retrieve_widget(tab, HIGHLIGHTING_BUTTON_LABEL);

	if (highlighting == NONE) {
		gtk_label_set_text(GTK_LABEL(button_label), "None");
	} else if (highlighting == C) {
		gtk_label_set_text(GTK_LABEL(button_label), "C");
	} else if (highlighting == CSS) {
		gtk_label_set_text(GTK_LABEL(button_label), "CSS");
	} else {
		assert(0);
	}
}


/*
void on_highlighting_changed2(void)
{
	printf("on_highlighting_changed2()\n");
}


void on_highlighting_changed3(void)
{
	printf("on_highlighting_changed3()\n");
}
*/


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

	//const char *default_highlighting = "None";
	const char *default_highlighting = "C";
	const char *highlightings[] = { "None", "C", "CSS", NULL };
	/*
	for (int i = 0; highlightings[i] != NULL; ++i) {
		//printf("%s\n", languages[i]);
		GtkWidget *item = gtk_menu_item_new_with_label(highlightings[i]);
		gtk_menu_attach(GTK_MENU(menu), item, 0, 1, i, i + 1);
		g_signal_connect(item, "activate", G_CALLBACK(on_highlighting_selected), tab);
		if (strcmp(highlightings[i], default_highlighting) == 0) {
			on_highlighting_selected(GTK_MENU_ITEM(item), hl_label); // set the highlighting
		}
	}
	*/

	for (int i = 0; highlightings[i] != NULL; ++i) {
		add_menu_item(GTK_MENU(menu), highlightings[i], G_CALLBACK(on_highlighting_selected), tab);
	}

	gtk_widget_show_all(menu);

	gtk_menu_button_set_popup(GTK_MENU_BUTTON(hl_menu_button), menu);
	//gtk_menu_button_set_direction(GTK_MENU_BUTTON(hl_menu_button), GTK_ARROW_UP);

	register_highlighting_changed_event_handler(tab, (void *) on_highlighting_changed);
	/*
	register_highlighting_changed_event_handler(tab, on_highlighting_changed2);
	register_highlighting_changed_event_handler(tab, on_highlighting_changed3);
	register_highlighting_changed_event_handler(tab, on_highlighting_changed);
	*/

	return hl_menu_button;
}


char *ignore_whitespace(char *str)
{
	int i = 0;
	while (str[i] == ' ' || str[i] == '\t') ++i;
	return str + i;
}


void parse_text_tags_file(void)
{
	printf("parse_text_tags_file()\n");

	char *contents = read_file("/home/eero/all/text-editor/themes/text-highlighting-settings");
	//printf("parse_text_tags_file: contents: %s\n", contents);
	while (char *line = get_slice_by(&contents, '\n')) {
		//printf("parse_text_tags_file: line: %s\n", line);
		line = ignore_whitespace(line);
		printf("*** %s\n", line);
		/*
		if (line[0] == '\0') {
			continue;
		}
		*/
		if (line[0] == '#') {
			printf("*** comment: %s\n", line);
			continue;
		}

		// get_slice_by(...):
		// abc:123 -> abc, 123, (null)
		// abc: -> abc, (null), (null)
		// abc -> abc, (null), (null), remaining: ""

		// get_slice_by_strict(...):
		// abc:123 -> abc, 123, (null)
		// abc: -> abc, (null), (null)
		// abc -> (null), (null), (null), remaining: "abc"
		// we could check if the remaining still has something... if has, we know there was no colon.

		// get_slice_by(...):
		// abc:123 -> abc, 123, (null)
		// abc: -> abc, "", (null)
		// abc -> abc, (null), (null) or: abc -> (null), (null), (null)
		// : -> "", "", (null)
		// :: -> "", "", ""

		char *copy = strdup(line);

		char *slice1, *slice2, *slice3;
		slice1 = get_slice_by(&line, ':');
		slice2 = get_slice_by(&line, ':');
		slice3 = get_slice_by(&line, ':');
		//printf("slice1: %s, slice2: %s, slice3: %s\n", slice1, slice2, slice3);

		// what about whitespace?
		if (slice1 && slice2 && slice3) {
			printf("parse_text_tags_file: unrecognized line format: %s\n", copy);
			continue;
		}
		if (slice1 && slice2) {
			printf("*** -> property: %s, value: %s\n\n", slice1, slice2);
			continue;
		}
		if (slice1) {
			printf("*** -> tag or language name: %s\n\n", slice1);
			continue;
		}
		// empty line?
		//printf( )
	}
}