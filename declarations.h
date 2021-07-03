
/* declarations.h -- well just pile up these function declarations here for now. so everything that wants to call an undeclared function can include this header.. whether the function be in the same or a different module. we could create a separate header file for each module (file) too but we would end up with thousand header files with almost no content whatsoever so.. */

/* file-browser.c: */
GtkWidget *create_file_browser_widget();
GtkTreeStore *create_tree_store();

/* search-in-files.c: */
GtkWidget *create_search_in_files_widget();

/* editing.c: */
gboolean move_lines_up(GdkEventKey *key_event);
gboolean move_lines_down(GdkEventKey *key_event);
gboolean duplicate_line(GdkEventKey *key_event);
gboolean delete_line(GdkEventKey *key_event);
void actually_open_line_after(GtkTextBuffer *text_buffer);
void actually_open_line_before(GtkTextBuffer *text_buffer);
void actually_autocomplete_character(GtkTextBuffer *text_buffer, char character);

/* fileio.c: */
char *read_file(const char *filename);
void write_file(const char *filename, const char *contents); // @ can fail

/* main.c: */
char *get_base_name(const char *file_name);
void refresh_application_title(GtkWidget *tab);
GtkWidget *create_tab(const char *file_name);

/* search.c: */
void init_search(GtkWidget *tab);

/* undo.c: */
void init_undo(GtkWidget *tab);
void actually_undo_last_action(GtkWidget *tab);

/* highlighting.c: */
void init_highlighting(GtkTextBuffer *text_buffer);
void remove_highlighting(GtkTextBuffer *text_buffer);

/* strings.c: */
char *get_slice_by(char **p_s, char ch);
char **slice_by(const char *s, char c);


