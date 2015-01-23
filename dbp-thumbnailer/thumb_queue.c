#include "thumb_queue.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <archive.h>
#include <archive_entry.h>
#include <Imlib2.h>
#include <glib.h>
#include <utime.h>
#include "meta.h"
#include "desktop.h"
#include "thumb_queue.h"
#include "comm.h"

struct ThumbQueue {
	struct ThumbQueue	*next;
	char			*uri;
	char			*flavour;
	int			id;
};


struct {
	pthread_mutex_t		mutex;
	struct ThumbQueue	*first;
	struct ThumbQueue	*last;
	sem_t			work;
	pthread_t		thread;
	int			counter;
} thumb_state;


struct ThumbMemBuffer {
	void			*data;
	int			len;
};


static struct ThumbMemBuffer load_icon(char *path) {
	struct archive *a;
	struct archive_entry *ae;
	struct meta_package_s mp;
	struct ThumbMemBuffer tmb;
	char *icon, icon_path[1024];
	
	tmb.data = NULL;
	if (meta_package_open(path, &mp) < 0)
		// Generate a thumb of a broken DBP?
		return tmb;
	if (!(icon = desktop_lookup(mp.df, "Icon", "", "Package Entry")))
		goto error;
	snprintf(icon_path, 1024, "icons/%s", icon);
	if (!(a = archive_read_new()))
		goto error;
	archive_read_support_format_zip(a);
	if (archive_read_open_filename(a, path, 512) != ARCHIVE_OK)
		goto error;
	while (archive_read_next_header(a, &ae) == ARCHIVE_OK)
		if (!strcmp(icon_path, archive_entry_pathname(ae)))
			goto found;
	goto error;

	found:
	tmb.len = archive_entry_size(ae);
	tmb.data = malloc(tmb.len);
	archive_read_data(a, tmb.data, tmb.len);

	error:
	archive_read_free(a);
	desktop_free(mp.df);
	return tmb;
}


static char *locate_thumbnail_file(char *uri) {
	char *md5, *cache_dir, *thumb_dir;

	md5 = g_compute_checksum_for_data(G_CHECKSUM_MD5, (uint8_t *) uri, strlen(uri));
	if (!(cache_dir = getenv("XDG_CACHE_HOME"))) {
		if (!getenv("HOME"))
			return NULL;
		cache_dir = malloc(strlen(getenv("HOME")) + strlen("/.cache") + 1);
		sprintf(cache_dir, "%s/.cache", getenv("HOME"));
	} else
		cache_dir = strdup(cache_dir);
	thumb_dir = malloc(strlen(cache_dir) + strlen("/thumbnails/normal/") + strlen(md5) + strlen(".png") + 1);
	sprintf(thumb_dir, "%s/thumbnails/normal/%s.png", cache_dir, md5);
	free(cache_dir);
	free(md5);
	return thumb_dir;
}


static bool convert_image(struct ThumbMemBuffer buff, struct ThumbQueue *queue, char *url, enum ThumbError *errp, char **errstrp) {
	FILE *fp;
	char *tmpname, atime[63], *thumb_file, *thumb_file_write, *errmsg;
	struct utimbuf newtime;
	struct stat thumb, file;
	enum ThumbError err;
	bool error;
	Imlib_Image *img;

	error = false;

	if (!(thumb_file = locate_thumbnail_file(queue->uri)))
		goto error;
	thumb_file_write = malloc(strlen(thumb_file) + 5);
	sprintf(thumb_file_write, "%s.prt", thumb_file);
	if (stat(thumb_file, &thumb) < 0)
		goto do_thumb;
	
	if (!stat(url, &file))
		if (thumb.st_mtim.tv_sec >= file.st_mtim.tv_sec)
			goto exit;
	
	do_thumb:
	tmpname = tempnam(getenv("XDG_RUNTIME_DIR"), "thumb");
	if (!(fp = fopen(tmpname, "w"))) {
		err = THUMB_ERROR_I_AM_A_TEAPOT;
		errmsg = "Unable to create temporary thumbnail";
		goto error;
	}

	fwrite(buff.data, buff.len, 1, buff.data);
	fclose(fp);
	
	if (!(img = imlib_load_image_immediately(tmpname))) {
		fprintf(stderr, "imlib failed to open\n");
		err = THUMB_ERROR_BAD_DATA;
		errmsg = "File contained an invalid icon";
		goto error;
	}

	imlib_image_attach_data_value("Thumb::URI", queue->uri, strlen(queue->uri), NULL);
	sprintf(atime, "%lli", (long long int) file.st_mtim.tv_sec);
	imlib_image_attach_data_value("Thumb::MTime", atime, strlen(atime), NULL);
	unlink(thumb_file_write);
	imlib_save_image(thumb_file_write);

	newtime.actime = file.st_atim.tv_sec;
	newtime.modtime = file.st_mtim.tv_sec;
	utime(thumb_file_write, &newtime);

	if (rename(thumb_file_write, thumb_file)) {
		err = THUMB_ERROR_SAVE_ERROR;
		errmsg = "Unable to rename thumbnail to final name";
		goto error;
	}
	
	goto exit;

	error:
	unlink(thumb_file_write);
	*errp = err;
	*errstrp = errmsg;
	error = true;

	/* TODO: Signal error */

	exit:
	imlib_free_image();
	unlink(tmpname);
	free(tmpname);
	free(thumb_file);
	free(thumb_file_write);
	return error;
}


static void create_thumbnail(struct ThumbQueue *queue) {
	char *path;
	struct ThumbMemBuffer buff;
	enum ThumbError err;
	char *errstr;

	if (strcmp(queue->flavour, "normal")) {
		err = THUMB_ERROR_BAD_TASTE;
		errstr = "Unsupported flavour";
		goto error;
	}

	buff.data = NULL;
	if (!(path = strstr(queue->uri, "file://")))
		path = queue->uri;
	buff = load_icon(path);
	if (!buff.data) {
		err = THUMB_ERROR_BAD_DATA;
		errstr = "Package is invalid/didn't have an icon";
		goto error;
	}
	if (!convert_image(buff, queue, path, &err, &errstr))
		goto error;
	free(buff.data);
	comm_dbus_announce_ready(queue->id, queue->uri);
	goto exit;

	error:
	comm_dbus_announce_error(queue->id, queue->uri, err, errstr);

	exit:
	comm_dbus_announce_finished(queue->id);
	free(buff.data);
	return;

}


static void clear_entry(struct ThumbQueue *entry) {
	free(entry->uri);
	free(entry->flavour);
	free(entry);
}


void *thumb_worker(void *bah) {
	struct ThumbQueue *this;

	for (;;) {
		sem_wait(&thumb_state.work);
		do {
			pthread_mutex_lock(&thumb_state.mutex);
			this = thumb_state.first;
			if (this)
				thumb_state.first = this->next;
			pthread_mutex_unlock(&thumb_state.mutex);
			if (!this)
				continue;
			
			comm_dbus_announce_started(this->id);
			create_thumbnail(this);
			clear_entry(this);
		} while (this);
	}
}


int thumb_queue(const char *uri, const char *flavour) {
	struct ThumbQueue *new;

	new = malloc(sizeof(*new));
	new->uri = strdup(uri);
	new->flavour = strdup(flavour);
	new->next = NULL;

	pthread_mutex_lock(&thumb_state.mutex);
	thumb_state.last = new;
	new->id = thumb_state.counter++;
	pthread_mutex_unlock(&thumb_state.mutex);
	return new->id;
}


void thumb_queue_init() {
	pthread_mutex_init(&thumb_state.mutex, NULL);
	sem_init(&thumb_state.work, 0, 0);
	thumb_state.first = NULL;
	thumb_state.last = NULL;
	thumb_state.counter = 0;
	pthread_create(&thumb_state.thread, NULL, thumb_worker, NULL);
}
