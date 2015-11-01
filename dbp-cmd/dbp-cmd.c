#include <dbpbase/dbpbase.h>
#include <dbpmgr/dbpmgr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <locale.h>
#define	_(STRING)	gettext(STRING)

struct FunctionLookup {
	char *name;
	void (*func)(int argc, char **argv);
};

int usage() {
	printf(_("Usage: dbp-cmd <command> [arg]\n"));
	printf(_("Executes a dbpd system call, version %s\n"), VERSION);
	printf(_("List of valid commands:\n"));
	printf(_("\tmount	- Mounts a package so that it can be executed. Takes pkgid.\n"));
	printf(_("\t		  Returns a token that you use to unmount the package\n"));
	printf(_("\tumount	- Unmounts the package if no other instance is using it. Takes mount token\n"));
	printf(_("\tgetmount	- Returns the mountpoint for the media the package is on. Takes pkgid\n"));
	printf(_("\tregister	- Register a package in the database. Takes path to package file\n"));
	printf(_("\tunregister	- Unregisters a package in the database. Takes path to package file\n"));
	printf(_("\tid		- Returns the pkgid for the package at <path>\n"));
	printf(_("\tpath		- Returns the path for the package with id <pkgid>\n"));
	printf(_("\tlist		- Lists all registered packages\n"));
	printf(_("\tping		- Pings dbpd, exits with a failure if dbpd isn't running\n"));
	printf("\n");
	printf(_("pkgid is a unique ID that every valid package has.\n"));
	printf(_("A registered package have its executables and .desktop files exported\n"));
	// TODO: Add note about manpage when a manpage exists //

	return 1;
}

static void cmd_mount(int argc, char **argv) {
	int ret;
	char *user;

	if (argc <3)
		exit(usage());
	if (!(user = getenv("USER")))
		fprintf(stderr, _("Error: Could not determine current user\n")), exit(1);
	if ((ret = dbpmgr_server_mount(argv[2], user)) < 0) {
		fprintf(stderr, _("Error: %s\n"), dbpmgr_error_lookup(ret));
		exit(1);
	}

	printf("%i\n", ret);
	exit(0);

}

static void cmd_umount(int argc, char **argv) {
	int ret;
	if (argc <3)
		exit(usage());
	if ((ret = dbpmgr_server_umount(atoi(argv[2]))) < 0) {
		fprintf(stderr, _("Error: %s\n"), dbpmgr_error_lookup(ret));
		exit(1);
	}

	exit(0);
}

static void cmd_mountpoint_get(int argc, char **argv) {
	int ret;
	char *mpoint;

	if (argc <3)
		exit(usage());
	if ((ret = dbpmgr_server_mountpoint_get(argv[2], &mpoint)) < 0) {
		fprintf(stderr, _("Error: %s\n"), dbpmgr_error_lookup(ret));
		exit(1);
	}

	printf("%s\n", mpoint);
	exit(0);
}

static void cmd_register_path(int argc, char **argv) {
	int ret;
	char *pkg_id;

	if (argc <3)
		exit(usage());
	if ((ret = dbpmgr_server_register_path(argv[2], &pkg_id)) < 0) {
		fprintf(stderr, _("Error: %s\n"), dbpmgr_error_lookup(ret));
		exit(1);
	}

	printf("%s\n", pkg_id);
	exit(0);
}

static void cmd_unregister_path(int argc, char **argv) {
	int ret;
	
	if (argc <3)
		exit(usage());
	if ((ret = dbpmgr_server_unregister_path(argv[2])) < 0) {
		fprintf(stderr, _("Error: %s\n"), dbpmgr_error_lookup(ret));
		exit(1);
	}

	exit(0);
}

static void cmd_id_from_path(int argc, char **argv) {
	int ret;
	char *pkg_id;

	if (argc <3)
		exit(usage());
	if ((ret = dbpmgr_server_id_from_path(argv[2], &pkg_id)) < 0) {
		fprintf(stderr, _("Error: %s\n"), dbpmgr_error_lookup(ret));
		exit(1);
	}

	printf("%s\n", pkg_id);
	exit(0);
}

static void cmd_path_from_id(int argc, char **argv) {
	int ret;
	char *path;

	if (argc <3)
		exit(usage());
	if ((ret = dbpmgr_server_path_from_id(argv[2], &path)) < 0) {
		fprintf(stderr, _("Error: %s\n"), dbpmgr_error_lookup(ret));
		exit(1);
	}

	printf("%s\n", path);
	exit(0);
}

static void cmd_package_list(int argc, char **argv) {
	struct DBPList *list, *next;

	(void) argc; (void) argv;
	list = dbpmgr_server_package_list();
	for (next = list; next; next = next->next)
		printf("'%s' '%s' %s\n", next->path, next->id, next->on_desktop?"desktop":"nodesktop");
	dbpmgr_server_package_list_free(list);
	exit(0);
}

static void cmd_ping(int argc, char **argv) {
	(void) argc; (void) argv;
	exit(dbpmgr_server_ping() < 0);
}


static struct FunctionLookup lookup[] = {
	{ "ping", cmd_ping },
	{ "mount", cmd_mount },
	{ "umount", cmd_umount },
	{ "getmount", cmd_mountpoint_get },
	{ "register", cmd_register_path },
	{ "unregister", cmd_unregister_path },
	{ "id", cmd_id_from_path },
	{ "path", cmd_path_from_id },
	{ "list", cmd_package_list },
	{ NULL, NULL },
};


int main(int argc, char **argv) {
	int i;

	setlocale(LC_ALL, "");
	textdomain("dbp-run");

	if (argc < 2)
		exit(usage());

	dbp_init(NULL);
	if (dbpmgr_server_connect() < 0)
		return 1;
	for (i = 0; lookup[i].name; i++)
		if (!strcmp(lookup[i].name, argv[1])) {
			lookup[i].func(argc, argv);
			break;
		}
	exit(usage());

	return 0;
}
