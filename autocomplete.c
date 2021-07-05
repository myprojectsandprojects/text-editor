/*
autocomplete.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <gtk/gtk.h>
#include "tab.h"

int reading = 0;
int i = 0;
char word[100];
int j = 0;
char *words[100];

void text_buffer_insert_text_4_autocomplete(
	GtkTextBuffer *buffer,
	GtkTextIter *location,
	char *text,
	int len,
	gpointer user_data)
{
	printf("text_buffer_insert_text_4_autocomplete() called\n");
	//printf("text_buffer_insert_text_4_autocomplete(): %s\n", text);

	assert(strlen(text) == 1);

	int c = text[0];

	if (c == ' ' || c == '\t' || c == '\n') {
		if (reading) {
			reading = 0;
			word[i] = 0;
			words[j] = malloc(strlen(word) + 1);
			strcpy(words[j], word);
			++j;
			int k;
			for (k = 0; k < j; ++k) {
				printf("************************************** word: %s\n", words[k]);
			}
			i = 0;
		}
	} else {
		reading = 1;
		word[i] = c;
		++i;
	}
}

void init_autocomplete(GtkWidget *tab)
{
	printf("init_autocomplete() called\n");
	GObject *text_buffer = (GObject *) tab_retrieve_widget(tab, TEXT_BUFFER);
	g_signal_connect(text_buffer, "insert-text", G_CALLBACK(text_buffer_insert_text_4_autocomplete), NULL);
}





