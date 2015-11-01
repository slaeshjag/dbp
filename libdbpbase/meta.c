#define	_LIBINTERNAL
#include "dbp.h"
#include "meta.h"
#include <time.h>
#include <archive.h>
#include <archive_entry.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


int dbp_meta_package_open(const char *path, struct DBPMetaPackage *mp) {
	struct archive *a;
	struct archive_entry *ae;
	int errid, found, size;
	char *data;
	struct stat statbuf;

	mp->df = NULL;

	if (!(a = archive_read_new()))
		return DBP_ERROR_NO_MEMORY;
	archive_read_support_format_zip(a);
	if (archive_read_open_filename(a, path, 512) != ARCHIVE_OK) {
		errid = (stat(path, &statbuf) || S_ISREG(statbuf.st_mode)) ? DBP_ERROR_NO_PKG_ACCESS : DBP_ERROR_BAD_META;
		goto error;
	}

	for (found = 0; archive_read_next_header(a, &ae) == ARCHIVE_OK; )
		if (!strcmp("meta/default.desktop", archive_entry_pathname(ae))) {
			found = 1;
			break;
		}
	
	if (!found) {
		errid = DBP_ERROR_NO_DEFAULTD;
		goto error;
	}

	size = archive_entry_size(ae);
	if (!(data = malloc(size + 1))) {
		errid = DBP_ERROR_NO_MEMORY;
		goto error;
	}

	archive_read_data(a, data, size);
	data[size] = 0;

	mp->df = dbp_desktop_parse(data);
	free(data);
	archive_read_free(a);

	mp->section = "Package Entry";
	return 0;

	error:

	archive_read_free(a);
	return errid;
	
}
