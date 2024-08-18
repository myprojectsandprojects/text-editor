
/*
possible cursor positions:
	- at the beginning of the buffer
	- at the end of the buffer
	- in the middle of the buffer
	ab
	 ^
	delete -- delete a character at the cursor location
	backspace -- delete a character before the cursor location

	todo:
		each tab should have its own extra cursors
		cursor movement
		selection
*/

#include <stdio.h>//@ this kind of stuff is probably slowing down the build
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "declarations.h"

//@ multiple tabs
static const int ExtraCursorsMax = 4;
static GtkTextMark *ExtraCursors[ExtraCursorsMax];
static int ExtraCursorsIndex = 0; // Cursor count

//
void MultiCursor_RemoveTextTags(GtkTextBuffer *TextBuffer)
{
	GtkTextIter StartBuffer, EndBuffer;
	gtk_text_buffer_get_bounds(TextBuffer, &StartBuffer, &EndBuffer);
	gtk_text_buffer_remove_tag_by_name(TextBuffer, "extra-cursor", &StartBuffer, &EndBuffer);
}

void MultiCursor_AddTextTags(GtkTextBuffer *TextBuffer)
{
	for(int i = 0; i < ExtraCursorsIndex; ++i)
	{
		GtkTextIter ExtraLocation, TagStart, TagEnd;
		gtk_text_buffer_get_iter_at_mark(TextBuffer, &ExtraLocation, ExtraCursors[i]);
		TagStart = ExtraLocation;
		gtk_text_iter_forward_char(&ExtraLocation);//@ buffer end
		TagEnd = ExtraLocation;
		gtk_text_buffer_apply_tag_by_name(TextBuffer, "extra-cursor", &TagStart, &TagEnd);
	}
}

gboolean MultiCursor_TextView_ButtonPress(GtkWidget *self, GdkEventButton *event, gpointer text_buffer)
{
//	printf("button (%u) press!\n", event->button);
	bool ControlDown = (event->state & GDK_CONTROL_MASK);
//	printf("%d\n", ControlDown);

	if(event->button == 1)
	{
		if(ControlDown)
		{
			if(ExtraCursorsIndex < ExtraCursorsMax)
			{
				GtkTextIter iter, tag_start, tag_finish;
				get_cursor_position(GTK_TEXT_BUFFER(text_buffer), NULL, &iter, NULL);
				tag_start = iter;
				gtk_text_iter_forward_char(&iter);//@ buffer end
				tag_finish = iter;
				gtk_text_buffer_apply_tag_by_name(GTK_TEXT_BUFFER(text_buffer), "extra-cursor", &tag_start, &tag_finish);

//				ExtraCursors[ExtraCursorsIndex] = tag_start;
				GtkTextMark *extra_cursor = gtk_text_buffer_create_mark(GTK_TEXT_BUFFER(text_buffer), NULL, &tag_start, FALSE);
				ExtraCursors[ExtraCursorsIndex] = extra_cursor;
				ExtraCursorsIndex += 1;
			}
		}
		else
		{
			//@ worth to check if there is actually anything to remove?
			GtkTextIter start_buffer, end_buffer;
			gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(text_buffer), &start_buffer, &end_buffer);
			gtk_text_buffer_remove_tag_by_name(GTK_TEXT_BUFFER(text_buffer), "extra-cursor", &start_buffer, &end_buffer);

			for(int i = 0; i < ExtraCursorsIndex; ++i)
			{
				gtk_text_buffer_delete_mark(GTK_TEXT_BUFFER(text_buffer), ExtraCursors[i]);
			}
			ExtraCursorsIndex = 0;
		}
	}

	return FALSE;
}

static bool IsOurInsert = false;
void MultiCursor_TextBuffer_InsertText(GtkTextBuffer *Self, /*const*/ GtkTextIter *Location, gchar *Text, gint Len, gpointer UserData)
{
	printf("%s()\n", __FUNCTION__);

	if(!IsOurInsert)
	{
		GtkTextMark *LocationMark = gtk_text_buffer_create_mark(Self, NULL, Location, FALSE);

		for(int i = 0; i < ExtraCursorsIndex; ++i)
		{
			GtkTextIter ExtraLocation;
			gtk_text_buffer_get_iter_at_mark(Self, &ExtraLocation, ExtraCursors[i]);
			IsOurInsert = true;
			gtk_text_buffer_insert(Self, &ExtraLocation, Text, -1);
			IsOurInsert = false;
		}

		gtk_text_buffer_get_iter_at_mark(Self, Location, LocationMark);
		gtk_text_buffer_delete_mark(Self, LocationMark);
	}
}

static bool IsOurDelete = false;
void MultiCursor_TextBuffer_DeleteRange(GtkTextBuffer *Self, /*const*/ GtkTextIter *Start, /*const*/ GtkTextIter *End, gpointer UserData)
{
	printf("%s()\n", __FUNCTION__);

	if(!IsOurDelete)
	{
		int CursorOffset;
		get_cursor_position(GTK_TEXT_BUFFER(Self), NULL, NULL, &CursorOffset);
		int StartOffset = gtk_text_iter_get_offset(Start);
		int EndOffset = gtk_text_iter_get_offset(End);
		printf("cursor: %d, start: %d, end: %d\n", CursorOffset, StartOffset, EndOffset);

		/*
		if End == CursorPos then it was backspace-key
		if End == CursorPos + 1 then it was delete-key
		or selection
		*/
		assert(StartOffset < EndOffset);

		GtkTextMark *StartMark = gtk_text_buffer_create_mark(Self, NULL, Start, FALSE);
		GtkTextMark *EndMark = gtk_text_buffer_create_mark(Self, NULL, End, FALSE);

		if((EndOffset - StartOffset) == 1)
		{
			for(int i = 0; i < ExtraCursorsIndex; ++i)
			{
				GtkTextIter Iter, StartRange, EndRange;
				gtk_text_buffer_get_iter_at_mark(Self, &Iter, ExtraCursors[i]);
				
				if(CursorOffset == EndOffset)
				{
					printf("character BEFORE the cursor was deleted\n");

					EndRange = Iter;
					gtk_text_iter_backward_char(&Iter);//@ start buffer
					StartRange = Iter;
				}
				else if(CursorOffset == StartOffset)
				{
					printf("character AT the cursor was deleted\n");

					StartRange = Iter;
					gtk_text_iter_forward_char(&Iter);//@ start buffer
					EndRange = Iter;
				}
				else
				{
					assert(false);
				}
	
				IsOurDelete = true;
				gtk_text_buffer_delete(Self, &StartRange, &EndRange);
				IsOurDelete = false;
			}
		}
		else
		{
			printf("multiple characters were deleted: we dont handle that yet\n");
		}

		//@ what happens to our text tags when we delete tagged text?
		MultiCursor_RemoveTextTags(Self);
		MultiCursor_AddTextTags(Self);
		
		gtk_text_buffer_get_iter_at_mark(Self, Start, StartMark);
		gtk_text_buffer_get_iter_at_mark(Self, End, EndMark);

		gtk_text_buffer_delete_mark(Self, StartMark);
		gtk_text_buffer_delete_mark(Self, EndMark);
	}
}

//static void
//on_cursor_position_changed_event(GObject *text_buffer, GParamSpec *pspec, gpointer user_data)
//{
////	fprintf(stderr, "%s\n", __FUNCTION__);
//
//	if(IsPreviousCursorPos)
//	{
//		if(LeftControlDown)
//		{
//			GtkTextIter iter, tag_start, tag_finish;
//			iter = PreviousCursorPos;
//			tag_start = iter;
//			gtk_text_iter_forward_char(&iter);//@
//			tag_finish = iter;
//			gtk_text_buffer_apply_tag_by_name(GTK_TEXT_BUFFER(text_buffer), "extra-cursor", &tag_start, &tag_finish);
//		}
//	}
//	else
//	{
//		// first time
//		IsPreviousCursorPos = true;
//	}
//
//	get_cursor_position(GTK_TEXT_BUFFER(text_buffer), NULL, &PreviousCursorPos, NULL);
//}

//// this doesnt really need to be called separately
//void init_multi_cursor_ability(GtkApplicationWindow *app_window)
//{
//	fprintf(stderr, "%s\n", __FUNCTION__);
//
//	g_signal_connect(app_window, "key-press-event",
//		G_CALLBACK(on_key_press_event), NULL);
//	g_signal_connect(app_window, "key-release-event",
//		G_CALLBACK(on_key_release_event), NULL);
//}

void MultiCursor_Init(GtkTextBuffer *text_buffer, Node *settings)
{
//	fprintf(stderr, "%s\n", __FUNCTION__);
	LOG_MSG("NEW: %s\n", __FUNCTION__);

	const char *color_str = settings_get_value(settings, "extra-cursor/color");
	assert(color_str);
	GdkRGBA color;
	if(gdk_rgba_parse(&color, color_str) == TRUE)
	{
		gtk_text_buffer_create_tag(text_buffer, "extra-cursor",
			"underline", PANGO_UNDERLINE_SINGLE,
			"underline-rgba", &color,
			NULL);
	}
	else
	{
		assert(false);
	}
}


