#include "validate.h"

int validate_pkg(const char *path) {
	struct archive *a;

	if (!(a = archive_read_new()))
		return 0;
	archive_read_support_format_zip(a);
	if (archive_read_open_filename(a, path, 512) != ARCHIVE_OK)
		goto error;
	
	

	error:
	archive_read_free(a);
	return 0;
}

