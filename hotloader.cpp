#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <assert.h>

#include "declarations.h"


struct Args {
	const char *file_path;
	GSourceFunc when_changed;
	void *user_data;
};


static void print_event(struct inotify_event *event)
{
	printf("print_event():");
	printf(" [%d]", event->wd);
	if (event->mask & IN_ISDIR) printf(" IN_ISDIR");
	if (event->mask & IN_ACCESS) printf(" IN_ACCESS");
	if (event->mask & IN_ATTRIB) printf(" IN_ATTRIB");
	if (event->mask & IN_CREATE) printf(" IN_CREATE");
	if (event->mask & IN_DELETE) printf(" IN_DELETE");
	if (event->mask & IN_DELETE_SELF) printf(" IN_DELETE_SELF");
	if (event->mask & IN_CLOSE_NOWRITE) printf(" IN_CLOSE_NOWRITE");
	if (event->mask & IN_CLOSE_WRITE) printf(" IN_CLOSE_WRITE");
	if (event->mask & IN_IGNORED) printf(" IN_IGNORED");
	if (event->mask & IN_MODIFY) printf(" IN_MODIFY");
	if (event->mask & IN_MOVE_SELF) printf(" IN_MOVE_SELF");
	if (event->mask & IN_MOVED_FROM) printf(" IN_MOVED_FROM");
	if (event->mask & IN_MOVED_TO) printf(" IN_MOVED_TO");
	if (event->mask & IN_OPEN) printf(" IN_OPEN");
	if (event->mask & IN_Q_OVERFLOW) printf(" IN_Q_OVERFLOW");
	if (event->mask & IN_UNMOUNT) printf(" IN_UNMOUNT");

	if (event->len > 0) printf(" \"%s\"", event->name);

	printf(" cookie: %d", event->cookie);

	printf("\n");
}

// Here, instead of monitoring the file directly, we monitor the whole directory.
static void *monitor_changes_and_call(void *args) {
	LOG_MSG("monitor_changes_and_call()\n");

	Args *a = (Args *) args;
	const char *file_path     = a->file_path;
	GSourceFunc when_changed  = a->when_changed;
	void *user_data            = a->user_data;

	const char *parent_path   = get_parent_path(file_path);
	const char *base_name     = get_base_name(file_path);

	int in = inotify_init();

	int watch;
	if(watch = inotify_add_watch(in, parent_path, IN_ALL_EVENTS) == -1) {
		ERROR("monitor_changes_and_call(): inotify_add_watch() error!");
		return NULL;
	}

	char *p;
	const int size = (sizeof(inotify_event) + NAME_MAX + 1) * 10; // we can store 10 events
	char event_buffer[size];

	for (;;) {
		ssize_t num_bytes = read(in, event_buffer, size);
		if (num_bytes < 1) {
			ERROR("monitor_changes_and_call(): read() returned less than 1..\n");
			break;
		}
		
		for (p = event_buffer; p < event_buffer + num_bytes;) {
			inotify_event *event = (inotify_event *) p;
//			print_event(event);

			if (event->mask & IN_IGNORED) {
				ERROR("monitor_changes_and_call(): cant continue monitoring file \"%s\" -- we lost our watch! Parent directory (\"%s\") got deleted?", file_path, parent_path);
				return NULL;
			} else if (((event->mask & IN_MODIFY) || (event->mask & IN_MOVED_TO)) && event->name && strcmp(event->name, base_name) == 0) { // just look for IN_MODIFY events associated with our file. Maybe it works. Also: IN_MOVED_TO for Gedit
				INFO("file (\"%s\") changed", file_path);
				g_timeout_add_seconds(1, when_changed, user_data);
			}

			p += sizeof(inotify_event) + event->len;
		}
	}

	return NULL;
}

/*
static void *monitor_changes_and_call(void *args)
{
	LOG_MSG("monitor_changes_and_call()\n");

	const char *filepath = ((struct Args *) args)->filepath;
	GSourceFunc when_changed = ((struct Args *) args)->when_changed;
	void *user_arg = ((struct Args *) args)->user_arg;

	int in_desc = inotify_init();

	int w_desc = inotify_add_watch(in_desc, filepath, IN_ALL_EVENTS);
	//int w_desc2 = inotify_add_watch(in_desc, "/home/eero/all", IN_ALL_EVENTS);
	//int w_desc = inotify_add_watch(in_desc, filepath, IN_MODIFY);
	//int w_desc = inotify_add_watch(in_desc, filepath, IN_CLOSE_WRITE);
	if (w_desc == -1) {
		printf("monitor_changes_and_call(): inotify_add_watch() error!\n");
		return NULL;
	}

	// seems to be important to have space for multiple event instances..
	#define EVENT_BUFFER_SIZE (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
	char *p;
	char event_buffer[EVENT_BUFFER_SIZE];
	for (;;) {
		ssize_t n_bytes = read(in_desc, event_buffer, EVENT_BUFFER_SIZE);
		if (n_bytes < 1) {
			printf("monitor_changes_and_call(): read() returned less than 1..\n");
			break;
		}
		
		for (p = event_buffer; p < event_buffer + n_bytes;) {
			struct inotify_event *event = (struct inotify_event *) p;
			print_event(event);

			if (event->mask & IN_IGNORED) {
				// gedit actually deletes the file in the course of modifying it, so we lose our watch
				// we try to re-add our watch
				//int w_desc = inotify_add_watch(in_desc, filepath, IN_CLOSE_WRITE);
				int w_desc = inotify_add_watch(in_desc, filepath, IN_ALL_EVENTS);
				if (w_desc == -1) {
					// ... file was deleted
					ERROR("monitor_changes_and_call(): inotify_add_watch() error!\n");
					return NULL;
				}
			}
//			else {
//				assert(event->mask & IN_CLOSE_WRITE);
//				WARNING("file changed!");
//				g_timeout_add_seconds(1, when_changed, user_arg);
//			}

			p += sizeof(struct inotify_event) + event->len;
		}
	}

	return (void *) NULL;
}
*/

// gboolean (*GSourceFunc) (gpointer data);
void hotloader_register_callback(const char *file_path, GSourceFunc when_changed, void *user_data)
{
	LOG_MSG("hotloader_register_callback()\n");

	file_path = strdup(file_path);

/*
	{
		struct Args *args = (Args *) malloc(sizeof(struct Args));
		args->filepath = file_path;
		args->when_changed = when_changed;
		args->user_arg = user_arg;

		pthread_t id;
		if (pthread_create(&id, NULL, monitor_changes_and_call, (void *) args) != 0) {
			printf("hotloader_register_callback(): pthread_create() error!\n");
		}
	}
*/

	{
		Args *a = (Args *) malloc(sizeof(Args));
		a->file_path = file_path;
		a->when_changed = when_changed;
		a->user_data = user_data;

		pthread_t id;
		if (pthread_create(&id, NULL, monitor_changes_and_call, (void *) a) != 0) {
			ERROR("hotloader_register_callback(): pthread_create() failed!");
		}
	}
}
