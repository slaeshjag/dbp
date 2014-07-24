#include "desktop.h"
#include "package.h"
#include "config.h"

#include <archive.h>
#include <archive_entry.h>

#include <dirent.h>

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


int package_register(struct package_s *p, const char *path, const char *device) {
	struct archive *a;
	struct archive_entry *ae;
	struct desktop_file_s *df;
	char *data, *pkg_id = "none";
	int found, size, id;

	df = NULL;
	if (!(a = archive_read_new()))
		return 0;
	archive_read_support_format_zip(a);
	if (archive_read_open_filename(a, path, 512) != ARCHIVE_OK) {
		fprintf(stderr, "Bad archive %s\n", path);
		goto error;
	}
	
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
	pkg_id = strdup(pkg_id);
	if ((id = package_add(p, strdup(path), pkg_id, strdup(device))) < 0) {
		free(pkg_id);
		goto error;
	}

	df = desktop_free(df);
	archive_read_free(a);
	fprintf(stderr, "Registered package %s\n", pkg_id);
	return 1;

	error:
	fprintf(stderr, "An error occured while registering a package %s\n", pkg_id);
	df = desktop_free(df);
	archive_read_free(a);
	return 0;
}


void package_crawl(struct package_s *p, const char *device, const char *path) {
	DIR *d;
	struct dirent dir, *res;
	int i;
	char *name_buff;

	if (!(d = opendir(path))) {
		fprintf(stderr, "Unable to open %s for directory list\n", path);
		return;
	}

	for (readdir_r(d, &dir, &res); res; readdir_r(d, &dir, &res)) {
		for (i = 0; i < config_struct.file_extensions; i++) {
			if (strlen(dir.d_name) < strlen(config_struct.file_extension[i]))
				continue;
			if (!strcmp(&dir.d_name[strlen(dir.d_name) - strlen(config_struct.file_extension[i])],
			    config_struct.file_extension[i])) {
				name_buff = malloc(strlen(path) + 2 + strlen(dir.d_name));
				sprintf(name_buff, "%s/%s", path, dir.d_name);
				package_register(p, name_buff, device);
				free(name_buff);
				break;
			}
		}
	}

	closedir(d);

	return;
}


void package_crawl_mount(struct package_s *p, const char *device, const char *path) {
	int i;
	char *new_path = NULL;

	for (i = 0; i < config_struct.search_dirs; i++) {
		new_path = realloc(new_path, strlen(path) + 2 + strlen(config_struct.search_dir[i]));
		sprintf(new_path, "%s/%s", path, config_struct.search_dir[i]);
		package_crawl(p, device, new_path);
	}
	
	free(new_path);
	return;
}
