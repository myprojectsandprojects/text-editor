#include "declarations.h"


extern int currentPageIndex;
extern array<NotebookPage> notebookPages;


void init_bookmarks(NotebookPage *page)
{
	LOG_MSG("%s\n", __FUNCTION__);
	ArrayInit(&page->bookmarks);
	page->activeBookmark = -1;
}

gboolean add_bookmark(GdkEventKey *key)
{
	LOG_MSG("add_bookmark()\n");
	LOG_MSG("currentPageIndex: %d\n", currentPageIndex);

	if(currentPageIndex != -1) {
		NotebookPage *current_page = &notebookPages.Data[currentPageIndex];

		GtkTextIter iter_cursor;
		get_cursor_position(current_page->buffer, NULL, &iter_cursor, NULL);

		GtkTextMark **new_bookmark = ArrayAppend(&current_page->bookmarks);
		*new_bookmark = gtk_text_buffer_create_mark(current_page->buffer, NULL, &iter_cursor, true);

		current_page->activeBookmark = current_page->bookmarks.Count - 1;

		return TRUE;
	}

	return FALSE;
}

gboolean goto_next_bookmark(GdkEventKey *key)
{
	LOG_MSG("goto_next_bookmark()\n");

	if(currentPageIndex != -1) {
		NotebookPage *current_page = &notebookPages.Data[currentPageIndex];
		if(current_page->bookmarks.Count > 0) {
			assert(current_page->activeBookmark != -1);

			if(current_page->bookmarks.Count > 1) {
				current_page->activeBookmark += 1;
				current_page->activeBookmark %= current_page->bookmarks.Count;
			}

			GtkTextIter bookmark_pos;
			gtk_text_buffer_get_iter_at_mark(current_page->buffer, &bookmark_pos, current_page->bookmarks.Data[current_page->activeBookmark]);
			gtk_text_buffer_place_cursor(current_page->buffer, &bookmark_pos);
			//gtk_text_view_scroll_to_mark()?
			gtk_text_view_scroll_to_iter(current_page->view, &bookmark_pos,
				0.0,
				TRUE, // use alignment?
				0.0, // x-alignment
				0.5); // y-alignment (in the middle of the screen)
		} else {
			LOG_MSG("goto_next_bookmark(): no bookmark to go to...\n");
		}
		return TRUE;
	}

	return FALSE;
}

gboolean clear_bookmarks(GdkEventKey *key)
{
	LOG_MSG("clear_bookmarks()\n");

	if(currentPageIndex != -1) {
		NotebookPage *current_page = &notebookPages.Data[currentPageIndex];
		if(current_page->bookmarks.Count > 0) {
			for(int i = 0; i < current_page->bookmarks.Count; ++i) {
				gtk_text_buffer_delete_mark(current_page->buffer, current_page->bookmarks.Data[i]);
			}
			current_page->bookmarks.Count = 0;

			current_page->activeBookmark = -1;
		}
		return TRUE;
	}

	return FALSE;
}