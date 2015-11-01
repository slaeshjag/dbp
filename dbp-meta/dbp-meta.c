#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <dbpbase/dbpbase.h>
#define	_(STRING)	gettext(STRING)

void usage() {
	fprintf(stdout, _("Extracts meta keys from a DBP\n"));
	fprintf(stdout, _("By Steven Arnow, 2015, version %s\n"), dbp_config_version_get());
	fprintf(stdout, "\n");
	fprintf(stdout, _("Usage:\n"));
	fprintf(stdout, _("dbp-meta list <path-to-dbp>			- lists all meta keys for the package\n"));
	fprintf(stdout, _("dbp-meta get <path-to-dbp> <key> [locale]	- Returns the value of meta-key <key>\n"));
}


int main(int argc, char **argv) {
	struct DBPMetaPackage mp;
	const char *str;

	setlocale(LC_ALL, "");
	textdomain("dbp-run");

	if (argc <3) {
		usage();
		return 1;
	}

	dbp_init(NULL);
	if (dbp_meta_package_open(argv[2], &mp)) {
		fprintf(stderr, _("Unable to open package %s\n"), argv[2]);
		return 1;
	}
	
	if (!strcmp(argv[1], "list")) {
		int section, i;

		section = dbp_desktop_lookup_section(mp.df, mp.section);
		for (i = 0; i < mp.df->section[section].entries; i++)
			fprintf(stdout, "%s[%s]=\"%s\"\n", mp.df->section[section].entry[i].key, mp.df->section[section].entry[i].locale, mp.df->section[section].entry[i].value);
		return 0;
	}

	if (!strcmp(argv[1], "get")) {
		if (argc < 4) {
			fprintf(stderr, _("Error: You need to specify the meta key to extract\n"));
			usage();
			return 1;
		}
		
		if (argc < 5)
			str = dbp_desktop_lookup(mp.df, argv[3], "", mp.section);
		else
			str = dbp_desktop_lookup(mp.df, argv[3], argv[4], mp.section);
		if (str)
			fprintf(stdout, "%s\n", str);
		return 0;
	}

	fprintf(stderr, _("Error: Unknown option '%s'\n"), argv[1]);

	return 1;
}
