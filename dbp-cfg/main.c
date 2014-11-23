#include "config.h"
#include "desktop.h"

#include <stdio.h>
#include <libintl.h>
#include <locale.h>

#define _(STRING)       gettext(STRING)

int main(int argc, char **argv) {
	struct desktop_file_s *df;
	const char *value;

	setlocale(LC_ALL, "");
	textdomain("dbp-run");

	if (argc < 2) {
		fprintf(stderr, _("Usage: %s <config key>\n"), argv[0]);
		return -1;
	}

	if (!(df = desktop_parse_file(CONFIG_FILE_PATH))) {
		fprintf(stderr, _("Unable to open config file %s\n"), CONFIG_FILE_PATH);
		return -1;
	}

	if (!(value = desktop_lookup(df, argv[1], "", "Package Daemon Config"))) {
		fprintf(stderr, _("Key '%s' doesn't exist in config\n"), argv[1]);
		return -1;
	}

	fprintf(stdout, "%s", value);
	return 0;
}
