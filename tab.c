#include <gtk/gtk.h>
#include <assert.h>

#include "declarations.h"


#define MAX_TABS 100
void *tab_widgets[MAX_TABS][N_WIDGETS];

GtkWidget *get_visible_tab(GtkNotebook *notebook)
{
	int page = gtk_notebook_get_current_page(notebook);
	if (page == -1) return NULL;
	return gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);
}

void *visible_tab_retrieve_widget(GtkNotebook *notebook, enum WidgetName widget_name)
{
	int page = gtk_notebook_get_current_page(notebook);
	//assert(page != -1);
	if (page == -1) return NULL;
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);
	return tab_retrieve_widget(tab, widget_name);
	/*
	struct TabInfo *tab_info =
		(struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	return tab_widgets[tab_info->id][widget_name];
	*/
}


/*
There are multiple ways these functions may fail:
	* the caller doesnt give us a valid pointer to a tab
	* the tab doesnt have a tab-info struct associated
	* the tab-info struct doesnt have an id-field
	* the id-value of the tab-info struct is not in the supported range
	etc.
But since this widget-bookkeeping-system should really only be a temporary solution I hesitate in making it very robust..
*/

void tab_add_widget_4_retrieval(GtkWidget *tab, enum WidgetName widget_name, void *widget)
{
	struct TabInfo *tab_info = (struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	tab_widgets[tab_info->id][widget_name] = widget;
}

void *tab_retrieve_widget(GtkWidget *tab, enum WidgetName widget_name)
{
	struct TabInfo *tab_info = (struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	return tab_widgets[tab_info->id][widget_name];
}


