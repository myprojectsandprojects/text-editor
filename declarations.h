
/* declarations.h -- well just pile up these function declarations here for now. so everything that wants to call an undeclared function can include this header.. whether the function be in the same or a different module. we could create a separate header file for each module (file) too but we would end up with thousand header files with almost no content whatsoever so.. */

/* If a declaration for a function which expects 0 arguments has () instead of (void), compiler will happily compile any combination of arguments. (?) */

#include <gtk/gtk.h>

/* hotloader.c */
/* gboolean (*GSourceFunc) (gpointer data); */
void hotloader_register_callback(const char *filepath, GSourceFunc when_changed);

/* file-browser.c: */
GtkWidget *create_filebrowser_widget(void);
GtkTreeStore *create_store(void);

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
gboolean move_cursor_end_line(GdkEventKey *key_event);
gboolean move_lines_up(GdkEventKey *key_event);
gboolean move_lines_down(GdkEventKey *key_event);
gboolean move_token_left(GdkEventKey *key_event);
gboolean move_token_right(GdkEventKey *key_event);
gboolean change_line(GdkEventKey *key_event);
gboolean delete_end_of_line(GdkEventKey *key_event);
void get_cursor_position(GtkTextBuffer *buffer, GtkTextMark **pm, GtkTextIter *pi, gint *po);

/* fileio.c: */
char *read_file(const char *filename);
void write_file(const char *filename, const char *contents);

/* main.c: */
char *get_base_name(const char *file_name);
void refresh_application_title(void);
GtkWidget *create_tab(const char *file_name);
void set_root_dir(const char *path);
void add_class(GtkWidget *widget, const char *class_name);
void remove_class(GtkWidget *widget, const char *class_name);

/* search-replace.c */
/*
gboolean toggle_search_entry(GdkEventKey *key_event);
gboolean toggle_replace_entry(GdkEventKey *key_event);
GtkWidget *create_search_and_replace_widget(GtkWidget *tab);
gboolean on_search_and_replace(void);
gboolean replace_selected_text(GdkEventKey *key_event);
*/

/* search.c */
GtkWidget *create_search_widget(GtkWidget *tab);
gboolean toggle_search_entry(GdkEventKey *key_event);
gboolean do_search(GdkEventKey *key_event);

/* undo.c: */
void init_undo(GtkWidget *tab);
void actually_undo_last_action(GtkWidget *tab);

/* highlighting.c: */
void init_highlighting(GtkTextBuffer *text_buffer);
void remove_highlighting(GtkTextBuffer *text_buffer);

/* strings.c: */
char *get_slice_by(char **p_s, char ch);
char **slice_by(const char *s, char c);
char *get_parent_path(const char *path);
int is_beginning_of(const char *needle, const char *haystack);
char *str_replace(const char *h, const char *n, const char *r);
int copy_string(const char *src, char *dst, int src_i, int dst_i, int n);

void test_get_parent_path(void);
void test_str_replace(void);

/* autocomplete.c */
void autocomplete_init(GtkNotebook *notebook, GtkApplicationWindow* app_window);
//void autocomplete_init_4tab(GtkWidget *tab);
gboolean autocomplete_close_popup(GdkEventKey *key_event);
gboolean do_autocomplete(GdkEventKey *key_event);
gboolean autocomplete_upkey(GdkEventKey *key_event);
gboolean autocomplete_downkey(GdkEventKey *key_event);
//gboolean autocomplete_on_appwindow_button_pressed(GtkWidget *x, GdkEvent *event, gpointer data);
/*
gboolean autocomplete_on_window_key_press(GtkWidget *window,
													GdkEvent *event,
													gpointer user_data);
*/
/* root-navigation.c */
GtkWidget *create_root_nav_widget(void);

/* open.c */
GtkWidget *create_openfile_widget(void);
gboolean display_openfile_dialog(GdkEventKey *key_event);

/* autocomplete-character.c */
void init_autocomplete_character(GtkTextBuffer *text_buffer);


//#define PRINT_LOG_MESSAGES
#ifdef PRINT_LOG_MESSAGES
	//#define LOG_MSG(...) printf(__VA_ARGS__)
	#define LOG_MSG(format, ...) printf("[%s:%d] " format, __FILE__, __LINE__, ##__VA_ARGS__)
#else
	#define LOG_MSG(...) 0
#endif


#define SETTING_VALUE_MAX 100

struct Settings {
	int pixels_above_lines;
	int left_margin;

	char line_highlight_color[SETTING_VALUE_MAX];
	char comment_color[SETTING_VALUE_MAX];
	char string_color[SETTING_VALUE_MAX];
	char identifier_color[SETTING_VALUE_MAX];
	char number_color[SETTING_VALUE_MAX];
	char operator_color[SETTING_VALUE_MAX];
	char keyword_color[SETTING_VALUE_MAX];
	char type_color[SETTING_VALUE_MAX];
	char preproccessor_color[SETTING_VALUE_MAX];
	char unknown_color[SETTING_VALUE_MAX];
};