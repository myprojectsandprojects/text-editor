
/* declarations.h -- well just pile up these function declarations here for now. so everything that wants to call an undeclared function can include this header.. whether the function be in the same or a different module. we could create a separate header file for each module (file) too but we would end up with thousand header files with almost no content whatsoever so.. */

/* If a declaration for a function which expects 0 arguments has () instead of (void), compiler will happily compile any combination of arguments. (?) */

#include <gtk/gtk.h>

// struct List stuff needs this, but I dont quite understand that:
#include <stdlib.h>


/* tab.c */
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
	HIGHLIGHTING_MENU_BUTTON,
	HIGHLIGHTING_MENU_BUTTON_LABEL,
	HIGHLIGHTING_MENU,
	HIGHLIGHTING_CHANGED_EVENT_HANDLERS,
	HIGHLIGHTING_CHANGED_EVENT_HANDLERS_INDEX,
	HIGHLIGHTING_FUNC,
	HIGHLIGHTING_TAGS,
	JUMPTO_MARKS,
	N_WIDGETS
};

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

/* hotloader.c */
/* gboolean (*GSourceFunc) (gpointer data); */
void hotloader_register_callback(const char *filepath, GSourceFunc when_changed, void *user_arg);

/* file-browser.c: */
GtkWidget *create_filebrowser_widget(void);
GtkTreeStore *create_store(void);
void create_nodes_for_dir(GtkTreeStore *store, GtkTreeIter *parent, const char *dir_path, int max_depth);

/* search-in-files.c: */
GtkWidget *create_search_in_files_widget(void);

/* editing.c: */
gboolean handle_tab_key(GtkTextBuffer *text_buffer, GdkEventKey *key_event);
gboolean duplicate_line(GdkEventKey *key_event);
gboolean delete_line(GdkEventKey *key_event);
gboolean insert_line_before(GdkEventKey *key_event);
gboolean insert_line_after(GdkEventKey *key_event);
void actually_autocomplete_character(GtkTextBuffer *text_buffer, char character);
gboolean move_cursor_left(GdkEventKey *key_event);
gboolean move_cursor_right(GdkEventKey *key_event);
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
gboolean move_token_left(GdkEventKey *key_event);
gboolean move_token_right(GdkEventKey *key_event);
gboolean change_line(GdkEventKey *key_event);
gboolean delete_end_of_line(GdkEventKey *key_event);
gboolean delete_word(GdkEventKey *key_event);
gboolean delete_inside(GdkEventKey *key_event);
gboolean select_inside(GdkEventKey *key_event);
gboolean comment_block(GdkEventKey *key_event);
gboolean uncomment_block(GdkEventKey *key_event);
void get_cursor_position(GtkTextBuffer *buffer, GtkTextMark **pm, GtkTextIter *pi, gint *po);

/* fileio.c: */
char *read_file(const char *filename);
void write_file(const char *filename, const char *contents);


/* main.c: */

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

#define INITIAL_SIZE 3

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
	struct CList *nodes; // if this is NULL, then node is a leaf-node and name stores the value
};

struct Node *get_node(struct Node *root, const char *apath);
const char *settings_get_value(struct Node *settings, const char *path);

//// we dont check for duplicates
//struct Table {
//	struct List<const char *> *names;
//	struct List<void *> *values;
//};
//struct Table *table_create(void);
//void table_store(struct Table *t, const char *name, void *value);
//void *table_get(struct Table *t, const char *name);

void add_menu_item(GtkMenu *menu, const char *label, GCallback callback, gpointer data);
char *get_base_name(const char *file_name);
void refresh_application_title(void);
GtkWidget *create_tab(const char *file_name);
void set_root_dir(const char *path);
void add_class(GtkWidget *widget, const char *class_name);
void remove_class(GtkWidget *widget, const char *class_name);
void add_keycombination_handler(
	int modifiers, int keycode, gboolean (*handler)(GdkEventKey *key_event));

struct JumpToMarks {
	int current_mark_i;
	GtkTextMark *marks[2];
};

/* search-replace.c */
GtkWidget *create_search_widget(GtkWidget *tab);
gboolean toggle_search_entry(GdkEventKey *key_event);
gboolean do_search(GdkEventKey *key_event);
const char *print_action2take(int action2take);
int parse_str(const char *str2parse,
	int *line_num, char **search_str, char **replace_with_str);

/* possible values for action2take */
#define SEARCH 0
#define REPLACE 1
#define GO_TO_LINE 2
#define DO_NOTHING 3

/* undo.c: */
void init_undo(GtkWidget *tab);
void actually_undo_last_action(GtkWidget *tab);


/* highlighting.c: */

void init_highlighting(void);
void select_highlighting_based_on_file_extension(GtkWidget *tab, struct Node *settings, const char *file_name);
void set_text_highlighting(GtkWidget *tab, const char *new_highlighting);
/*
#define ON 0
#define OFF 1
void set_current_line_highlighting(GtkTextBuffer *text_buffer, int to_what);
*/
void highlighting_current_line_enable_or_disable(struct Node *settings, GtkTextBuffer *text_buffer);
void highlighting_current_line_enable(GtkTextBuffer *text_buffer, struct Node *settings);
void highlighting_current_line_disable(GtkTextBuffer *text_buffer);
GtkWidget *highlighting_new_menu_button(GtkWidget *tab, struct Node *settings);
void highlighting_update_menu(GtkWidget *tab, struct Node *settings);
//void parse_text_tags_file(void);


/* strings.c: */

char *get_slice_by(char **p_s, char ch);
char **slice_by(const char *s, char c);
char *get_parent_path(const char *path);
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

/* autocomplete.c */

#define MAX_STRS 1000
struct SortedStrs {
	const char *strs[MAX_STRS];
	unsigned int counts[MAX_STRS];
	unsigned int i;
};
struct SortedStrs *sorted_strs_new(void);
void sorted_strs_free(struct SortedStrs* strs);
void sorted_strs_print(struct SortedStrs *strs);
bool sorted_strs_add(struct SortedStrs *strs, const char *str);
struct SortedStrs *sorted_strs_get_starts_with(struct SortedStrs *strs, const char *starts_with);

void autocomplete_init(GtkNotebook *notebook, GtkApplicationWindow* app_window);
struct SortedStrs *autocomplete_create_and_store_words(GtkTextBuffer *text_buffer);
gboolean autocomplete_upkey(GdkEventKey *key_event);
gboolean autocomplete_downkey(GdkEventKey *key_event);
gboolean do_autocomplete(GdkEventKey *key_event);
gboolean autocomplete_close_popup(GdkEventKey *key_event);

/* root-navigation.c */
GtkWidget *create_root_nav_widget(void);

/* open.c */
GtkWidget *create_openfile_widget(void);
gboolean display_openfile_dialog(GdkEventKey *key_event);

/* autocomplete-character.c */
void init_autocomplete_character(GtkTextBuffer *text_buffer);

/* tests.c */
void test_table(void);
int test_case_parse_str(const char *str2parse,
	int expected_action,
	int expected_line_num,
	const char *expected_search_str,
	const char *expected_replace_with_str);
void test_parse_str(void);
void test_get_word_with_allocate(void);


//#define PRINT_LOG_MESSAGES
#ifdef PRINT_LOG_MESSAGES
	//#define LOG_MSG(...) printf(__VA_ARGS__)
	#define LOG_MSG(format, ...) printf("[%s:%d] " format, __FILE__, __LINE__, ##__VA_ARGS__)
#else
	#define LOG_MSG(...) 0
#endif