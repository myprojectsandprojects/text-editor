#include <gtk/gtk.h>
#include <assert.h>

#include "tab.h"


GtkWidget *get_visible_tab(GtkNotebook *notebook)
{
	int page = gtk_notebook_get_current_page(notebook);
	if (page == -1) return NULL;
	return gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);
}

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

/* Maybe this function should be somewhere else? */
void *visible_tab_retrieve_widget(GtkNotebook *notebook, enum WidgetName widget_name)
{
	int page = gtk_notebook_get_current_page(notebook);
	//assert(page != -1);
	if (page == -1) return NULL;
	GtkWidget *tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page);
	return tab_retrieve_widget(tab, widget_name);
	/*struct TabInfo *tab_info = (struct TabInfo *) g_object_get_data(G_OBJECT(tab), "tab-info");
	return tab_widgets[tab_info->id][widget_name];*/
}


