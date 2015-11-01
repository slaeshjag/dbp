#include <dbpbase/dbpbase.h>

#include <stdio.h>
#include <libintl.h>
#include <locale.h>

#define _(STRING)       gettext(STRING)


void usage(char *name) {
	fprintf(stderr, _("Extracts configuration keys out of the DBP config\n"));
	fprintf(stderr, _("By Steven Arnow, 2015, version %s\n"), dbp_config_version_get());
	fprintf(stderr, "\n");
	fprintf(stderr, _("Usage: %s <config key>\n"), name);
}


int main(int argc, char **argv) {
	struct DBPDesktopFile *df;
	const char *value;

	setlocale(LC_ALL, "");
	textdomain("dbp-run");

	dbp_init(NULL);
	if (argc < 2)
		return usage(argv[0]), -1;
	if (!strcmp(argv[1], "--help"))
		return usage(argv[0]), 0;

	if (!(df = dbp_desktop_parse_file(DBP_CONFIG_FILE_PATH))) {
		fprintf(stderr, _("Unable to open config file %s\n"), DBP_CONFIG_FILE_PATH);
		return -1;
	}

	if (!(value = dbp_desktop_lookup(df, argv[1], "", "Package Daemon Config"))) {
		fprintf(stderr, _("Key '%s' doesn't exist in config\n"), argv[1]);
		return -1;
	}

	fprintf(stdout, "%s", value);
	return 0;
}
