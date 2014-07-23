#include "desktop.h"
#include "package.h"

#include <archive.h>
#include <archive_entry.h>

int package_find(struct package_s *p, const char *id) {
	int i;

	for (i = 0; i < p->entries; i++)
		if (!strcmp(id, p->entry[i].id))
			return 1;
	return 0;
}


int package_add(struct package_s *p, char *path, char *id, char *device) {
	int nid, i;

	for (i = 0; i < p->entries; i++)
		if (!strcmp(p->entry[i].id, id)) {
			fprintf(stderr, "Package %s is already registered at %s\n", id, p->entry[i].path);
			return -1;
		}
	nid = p->entries++;
	p->entry = realloc(p->entry, sizeof(*p->entry) * p->entries);
	p->entry[nid].path = path;
	p->entry[nid].id = id;

	return nid;
}


int package_register(struct package_s *p, const char *path) {
	struct archive *a;
	struct archive_entry *ae;
	struct desktop_file_s *df;
	char *data, *pkg_id;
	int found, size, id;

	df = NULL;
	if (!(a = archive_read_new()))
		return 0;
	archive_read_support_format_zip(a);
	if (archive_read_open_filename(a, path, 512) != ARCHIVE_OK)
		goto error;
	
	found = 0;
	while (archive_read_next_header(a, &ae) == ARCHIVE_OK) {
		if (!strcmp("meta/default.desktop", archive_entry_pathname(ae))) {
			fprintf(stderr, "Found default.desktop\n");
			found = 1;
			break;
		}
	}

	if (!found) {
		fprintf(stderr, "Package has no default.desktop\n");
		goto error;
	}

	size = archive_entry_size(ae);
	if (!(data = malloc(size + 1)))
		goto error;
	archive_read_data(a, data, size);
	data[size] = 0;

	df = desktop_parse(data);
	if (!(pkg_id = desktop_lookup(df, "Id", "", "Package Entry")))
		goto error;
	if ((id = package_add(p, strdup(path), pkg_id, strdup("/dev/dummy"))) < 0) {
		free(pkg_id);
		goto error;
	}

	df = desktop_free(df);
	archive_read_free(a);
	return 1;

	error:
	df = desktop_free(df);
	archive_read_free(a);
	return 0;
}

