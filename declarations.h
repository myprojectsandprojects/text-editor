
/* declarations.h -- well just pile up these function declarations here for now. so everything that wants to call an undeclared function can include this header.. whether the function be in the same or a different module. we could create a separate header file for each module (file) too but we would end up with thousand header files with almost no content whatsoever so.. */

#include <gtk/gtk.h>

#include <stdlib.h>

#include "lib.h" // declares functions defined in lib.c and provides some useful macros


/* tab.cpp */
enum WidgetName{
	TEXT_VIEW,
	TEXT_BUFFER,
	COMMAND_REVEALER,
	COMMAND_ENTRY,
	SEARCH_REVEALER,
	SEARCH_ENTRY,
//	REPLACE_REVEALER,
//	REPLACE_ENTRY,
	SEARCH_STATE,
	STATUS_MESSAGE_LABEL,
	FILEPATH_LABEL,
	AUTOCOMPLETE_WORDS,
	AUTOCOMPLETE_STATE,
//	CURRENT_TEXT_HIGHLIGHTING,
//	HIGHLIGHTING_MENU_BUTTON,
	HIGHLIGHTING_MENU_BUTTON_LABEL,
//	HIGHLIGHTING_MENU,
//	HIGHLIGHTING_CHANGED_EVENT_HANDLERS,
//	HIGHLIGHTING_CHANGED_EVENT_HANDLERS_INDEX,
//	HIGHLIGHTING_FUNC,
//	HIGHLIGHTING_TAGS,
	HIGHLIGHTER,
	JUMPTO_MARKS,
	AUTOCOMPLETE_CHARACTER_HANDLER_ID,
	N_WIDGETS
};

void tab_add_widget_4_retrieval(GtkWidget *tab, enum WidgetName widget_name, void *widget);
void *tab_retrieve_widget(GtkWidget *tab, enum WidgetName widget_name);

/* Maybe these functions should be somewhere else? */
void *visible_tab_retrieve_widget(GtkNotebook *notebook, enum WidgetName widget_name);
GtkWidget *get_visible_tab(GtkNotebook *notebook);

struct TabInfo {
	unsigned int id;
	const char *file_name; // NULL if no file associated with the tab.
	const char *title;
	gboolean unsaved_changes;
	//char *base_name; // Maybe well put basename here for convenience.
};

/* hotloader.cpp */
/* gboolean (*GSourceFunc) (gpointer data); */
void hotloader_register_callback(const char *filepath, GSourceFunc when_changed, void *user_arg);

/* file-browser.cpp: */
GtkWidget *create_filebrowser_widget(void);
GtkTreeStore *create_store(void);
void create_nodes_for_dir(GtkTreeStore *store, GtkTreeIter *parent, const char *dir_path, int max_depth);

/* search-in-files.cpp: */
GtkWidget *create_search_in_files_widget(void);

/* editing.cpp: */
gboolean handle_tab_key(GtkTextBuffer *text_buffer, GdkEventKey *key_event);
gboolean handle_enter(GdkEventKey *key_event);
gboolean duplicate_line(GdkEventKey *key_event);
gboolean delete_line(GdkEventKey *key_event);
gboolean insert_line_before(GdkEventKey *key_event);
gboolean insert_line_after(GdkEventKey *key_event);
void actually_autocomplete_character(GtkTextBuffer *text_buffer, char character);
//gboolean move_cursor_left(GdkEventKey *key_event);
//gboolean move_cursor_right(GdkEventKey *key_event);
//gboolean cursor_short_jump_left(GdkEventKey *key_event);
//gboolean cursor_short_jump_right(GdkEventKey *key_event);
//gboolean cursor_long_jump_left(GdkEventKey *key_event);
//gboolean cursor_long_jump_right(GdkEventKey *key_event);
gboolean cursor_jump_forward(GdkEventKey *key_event);
gboolean cursor_jump_backward(GdkEventKey *key_event);
gboolean move_cursor_up(GdkEventKey *key_event);
gboolean move_cursor_down(GdkEventKey *key_event);
gboolean move_cursor_start_line(GdkEventKey *key_event);
gboolean move_cursor_start_line_shift(GdkEventKey *key_event);
gboolean move_cursor_end_line(GdkEventKey *key_event);
gboolean move_cursor_end_line_shift(GdkEventKey *key_event);
gboolean move_cursor_start_word(GdkEventKey *key_event);
gboolean move_cursor_start_word_shift(GdkEventKey *key_event);
gboolean move_cursor_end_word(GdkEventKey *key_event);
gboolean move_cursor_end_word_shift(GdkEventKey *key_event);
gboolean move_cursor_opening(GdkEventKey *key_event);
gboolean move_cursor_closing(GdkEventKey *key_event);
gboolean move_lines_up(GdkEventKey *key_event);
gboolean move_lines_down(GdkEventKey *key_event);
//gboolean move_token_left(GdkEventKey *key_event);
//gboolean move_token_right(GdkEventKey *key_event);
gboolean change_line(GdkEventKey *key_event);
gboolean delete_end_of_line(GdkEventKey *key_event);
gboolean delete_word(GdkEventKey *key_event);
gboolean delete_inside(GdkEventKey *key_event);
gboolean select_inside(GdkEventKey *key_event);
gboolean comment_block(GdkEventKey *key_event);
gboolean uncomment_block(GdkEventKey *key_event);
void get_cursor_position(GtkTextBuffer *buffer, GtkTextMark **pm, GtkTextIter *pi, gint *po);

/* main.cpp: */

#define CLIST_INITIAL_SIZE 3

struct CList {
	void **data;
	int i_end; // this should be initialized to 0
	int size; // the current size of the elements array
};
struct CList *new_list(void);
void list_append(struct CList *l, void *item);
void *list_pop_last(struct CList *l);
bool list_delete_item(struct CList *l, void *item);

#define INITIAL_SIZE 1000000

template <typename T> struct List {
	T *data; // pointer to dynamically allocated array of any type
	int index; // this should be initialized to 0
	int size; // the current size of the elements array
};

template <typename T>
struct List<T> *list_create(void) {
	struct List<T> *l = (struct List<T> *) malloc(sizeof(struct List<T>));
	l->index = 0;
	l->size = INITIAL_SIZE;
	l->data = (T *) malloc(l->size * sizeof(T));
	return l;
}

template <typename T>
void list_add(struct List<T> *l, T item) {
	//printf("adding an item to the list\n");
	//assert(l->index < SIZE);
	if (!(l->index < l->size)) {
		// realloc()?
		//printf("allocating more memory\n");
		l->size *= 2;
		T *new_memory = (T *) malloc(l->size * sizeof(T));
		for (int i = 0; i < l->index; ++i) {
			new_memory[i] = l->data[i];
		}
		free(l->data); //@ this can be a problem? because we wanna store all kinds of stuff here
		l->data = new_memory;
	}
	l->data[l->index] = item;
	l->index += 1;
}

struct Node {
	const char *name;
//	struct CList *nodes; // if this is NULL, then node is a leaf-node and name stores the value
	Array<Node *> nodes;
};

Node *get_node(Node *root, const char *apath);
const char *settings_get_value(Node *settings, const char *path);

//// we dont check for duplicates
//struct Table {
//	struct List<const char *> *names;
//	struct List<void *> *values;
//};
//struct Table *table_create(void);
//void table_store(struct Table *t, const char *name, void *value);
//void *table_get(struct Table *t, const char *name);

// 1 -- bold, 31 -- red, 33 -- yellow
#define ERROR(message, ...)\
	fprintf(stderr, "\033[1;31m" message "\033[0m\n", ##__VA_ARGS__);
// have warnings too?
#define WARNING(message, ...)\
	fprintf(stderr, "\033[1;33m" message "\033[0m\n", ##__VA_ARGS__);
#define INFO(message, ...)\
	fprintf(stderr, "\033[1;32m" message "\033[0m\n", ##__VA_ARGS__);

//void display_error(const char *primary_message, const char *secondary_message);
void add_menu_item(GtkMenu *menu, const char *label, GCallback callback, gpointer data);
void refresh_application_title(void);
GtkWidget *create_tab(const char *file_name = NULL);
void set_root_dir(const char *path);
void add_class(GtkWidget *widget, const char *class_name);
void remove_class(GtkWidget *widget, const char *class_name);
void add_keycombination_handler(
	int modifiers, int keycode, gboolean (*handler)(GdkEventKey *key_event));

struct JumpToMarks {
	int current_mark_i;
	GtkTextMark *marks[2];
};

void line_highlighting_on_text_buffer_cursor_position_changed(GObject *object, GParamSpec *pspec, gpointer user_data); //@ hack: this is called by highlighting to keep the line-highlighting uptodate.
void scope_highlighting_on_cursor_position_changed(GObject *object, GParamSpec *pspec, gpointer user_data);
void matching_char_highlighting_on_cursor_position_changed(GObject *object, GParamSpec *pspec, gpointer user_data);

enum SelectionGranularity {
	SELECTION_GRANULARITY_NONE = 0, // we are not updating a selection (left mouse button is not down)
	SELECTION_GRANULARITY_CHARACTER,
	SELECTION_GRANULARITY_WORD,
	SELECTION_GRANULARITY_LINE,
};

// MouseState / MouseData / MouseSelectionState ?
struct TextViewMouseSelectionState {
	GtkTextMark *press_position; // single press
	GtkTextMark *original_selection_start, *original_selection_end; // double- and triple-press
	SelectionGranularity selection_granularity;
	int mark_count; // 0
};

// 'Tab' could be confused with tab-key
//struct SidebarNotebookPage
struct NotebookPage {
/*
We store each NotebookPage in an array at an index which is also it's id.
*/
	int id; //-1 -- invalid id

	bool in_use; // When a tab is closed, this is set to false

	/*
	This state is initialized at (left) mouse press and cleaned up at (left) mouse release.
	So if we disallow user to switch between pages and close pages (everything that changes the visible page) while (left) mouse button is down,
	then why not just have one instance of this as opposed to one for each page?
	@@Right now we dont expect the visible page to change underneath us in between button press and button release.
	One solution would be to not allow that to happen, ever. Block all keybindings while left mouse button is down, for example.
	Another would be to somehow gracefully handle these situations.
	*/
	TextViewMouseSelectionState mouse_selection_state;
//	struct {
//		GtkTextMark *position;
//		GtkTextMark *original_selection_start, *original_selection_end;
//		SelectionGranularity selection_granularity;
//	} textview_button_press;

	GtkTextTag *texttags[16];
	int texttags_count; // 0

	//...
	GtkTextView *view;
	GtkTextBuffer *buffer;

	GtkWidget *tab; // temporary
};

bool is_word(unsigned int ch);

/* search-replace.cpp */
GtkWidget *create_search_widget(GtkWidget *tab);
gboolean toggle_search_entry(GdkEventKey *key_event);
gboolean do_search(GdkEventKey *key_event);
gboolean jump_to_next_occurrence(GdkEventKey *key_event);
gboolean jump_to_previous_occurrence(GdkEventKey *key_event);
const char *print_action2take(int action2take);
int parse_str(const char *str2parse,
	int *line_num, char **search_str, char **replace_with_str);

/* possible values for action2take */
#define SEARCH 0
#define REPLACE 1
#define GO_TO_LINE 2
#define DO_NOTHING 3

/* undo.cpp: */
void undo_init(gulong insert_handlers[], int insert_handlers_count, gulong *delete_handlers, int delete_handlers_count, unsigned int tab_id);
void undo_text_buffer_insert_text(GtkTextBuffer *text_buffer, GtkTextIter *location, char *inserted_text, int length, gpointer data);
void undo_text_buffer_delete_range(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end, gpointer data);
void actually_undo_last_action(GtkWidget *tab);

/* highlighting.cpp: */
//typedef void (*Highlighter) (GtkTextBuffer *, GtkTextIter *, GtkTextIter *);
typedef void (*Highlighter) (GtkTextBuffer *, GtkTextIter *);

void highlighting_init(GtkWidget *tab, NotebookPage *page, Node *settings);
void highlighting_set(GtkWidget *tab, const char *highlighting_type);
void print_tags(GtkTextBuffer *text_buffer);
GtkWidget *create_highlighting_selection_button(GtkWidget *tab, Node *settings);
void highlighting_apply_settings(Node *settings, NotebookPage *page);

/* highlighting.c: */
//void select_highlighting_based_on_file_extension(GtkWidget *tab, struct Node *settings, const char *file_name);
//void set_text_highlighting(GtkWidget *tab, const char *new_highlighting);
//void highlighting_current_line_enable_or_disable(struct Node *settings, GtkTextBuffer *text_buffer);
//void highlighting_current_line_enable(GtkTextBuffer *text_buffer, struct Node *settings);
//void highlighting_current_line_disable(GtkTextBuffer *text_buffer);
//GtkWidget *highlighting_new_menu_button(GtkWidget *tab, struct Node *settings);
//void highlighting_update_menu(GtkWidget *tab, struct Node *settings);
//void print_tags(GtkTextBuffer *text_buffer);

/* highlighting-c.cpp*/
void c_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end);

/* highlighting-cpp.cpp: */
//void cpp_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end);
void cpp_highlight(GtkTextBuffer *text_buffer, GtkTextIter *location);

/* highlighting-rust.cpp: */
void rust_highlight(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end);

/* strings.cpp: */
char *get_base_name(const char *file_name);
char *get_slice_by(char **p_s, char ch);
char **slice_by(const char *s, char c);
char *get_parent_path(const char *path);
char *get_parent_path_noalloc(char *path);
int is_beginning_of(const char *needle, const char *haystack); //@ fix inconsistent style
bool ends_with(const char *h, const char *n);
char *str_replace(const char *h, const char *n, const char *r);
int copy_string(const char *src, char *dst, int src_i, int dst_i, int n);
char *get_word_with_allocate(char **pstr);
char *ignore_whitespace(char *str);
char *trim_whitespace(char *str);

void test_get_parent_path(void);
void test_str_replace(void);
void test_get_slice_by(void);
void test_trim_whitespace(void);

/* autocomplete.cpp */

//#define MAX_STRS 1000
//struct SortedStrs {
//	const char *strs[MAX_STRS];
//	unsigned int counts[MAX_STRS];
//	unsigned int i;
//};
//struct SortedStrs *sorted_strs_new(void);
//void sorted_strs_free(struct SortedStrs* strs);
//void sorted_strs_print(struct SortedStrs *strs);
//bool sorted_strs_add(struct SortedStrs *strs, const char *str);
//struct SortedStrs *sorted_strs_get_starts_with(struct SortedStrs *strs, const char *starts_with);
//
//void autocomplete_init(GtkNotebook *notebook, GtkApplicationWindow* app_window);
//struct SortedStrs *autocomplete_create_and_store_words(GtkTextBuffer *text_buffer);
//gboolean autocomplete_upkey(GdkEventKey *key_event);
//gboolean autocomplete_downkey(GdkEventKey *key_event);
//gboolean do_autocomplete(GdkEventKey *key_event);
//gboolean autocomplete_close_popup(GdkEventKey *key_event);

/*
autocomplete-identifier.cpp
*/
#include "autocomplete-identifier.h"
void autocomplete_identifier_init(GtkWidget *tab, GtkTextBuffer *text_buffer);
AutocompleteState *autocomplete_state_new(Array<Identifier *> *identifiers);
void autocomplete_state_free(AutocompleteState *state, GtkTextBuffer *text_buffer);
Array<Identifier *> *autocomplete_get_identifiers(const char *text);
void autocomplete_print_identifiers(Array<Identifier *> *identifiers);
gboolean autocomplete_emacs_style(GdkEventKey *key_event);
gboolean autocomplete_clear(GdkEventKey *key_event);

/* root-navigation.cpp */
GtkWidget *create_root_nav_widget(void);

/* open.cpp */
GtkWidget *create_openfile_widget(void);
gboolean display_openfile_dialog(GdkEventKey *key_event);

/* autocomplete-character.cpp */
//void init_autocomplete_character(GtkTextBuffer *text_buffer, Node *settings, GtkWidget *tab);
void text_expansion_init(GtkTextBuffer *text_buffer, Node *settings, GtkWidget *tab);
void text_expansion_text_buffer_begin_user_action(GtkTextBuffer *text_buffer, gpointer data);
void text_expansion_text_buffer_delete_range(GtkTextBuffer *text_buffer, GtkTextIter *start, GtkTextIter *end, gpointer data);
void text_expansion_text_buffer_insert_text(GtkTextBuffer *text_buffer, GtkTextIter *location, char *inserted_text, int length, gpointer data);

/* tests.cpp */
void test_table(void);
int test_case_parse_str(const char *str2parse,
	int expected_action,
	int expected_line_num,
	const char *expected_search_str,
	const char *expected_replace_with_str);
void test_parse_str(void);
void test_get_word_with_allocate(void);


/* multi-cursor-ability.cpp*/
gboolean MultiCursor_TextView_ButtonPress(GtkWidget *self, GdkEventButton *event, gpointer text_buffer);
void MultiCursor_TextBuffer_InsertText(GtkTextBuffer *Self, /*const*/ GtkTextIter *Location, gchar *Text, gint Len, gpointer UserData);
void MultiCursor_TextBuffer_DeleteRange(GtkTextBuffer *Self, /*const*/ GtkTextIter *Start, /*const*/ GtkTextIter *End, gpointer UserData);
void MultiCursor_Init(GtkTextBuffer *text_buffer, Node *settings);
void MultiCursor_AddTextTags(GtkTextBuffer *TextBuffer);
void MultiCursor_RemoveTextTags(GtkTextBuffer *TextBuffer);


//#define PRINT_LOG_MESSAGES
#ifdef PRINT_LOG_MESSAGES
	//#define LOG_MSG(...) printf(__VA_ARGS__)
//	#define LOG_MSG(format, ...) printf("[%s:%d] " format, __FILE__, __LINE__, ##__VA_ARGS__)
	#define LOG_MSG(format, ...) printf("\x1b[38;5;242m[%s:%d]\x1b[0m " format, __FILE__, __LINE__, ##__VA_ARGS__) // Hardcoded terminal colors might not be compatible with a different color theme.
#else
	#define LOG_MSG(...) 0
#endif
