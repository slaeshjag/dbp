#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <pthread.h>

#include <dbpmgr/dbpmgr.h>
#include <dbpbase/dbpbase.h>

#define	HAS_SUFFIX(str, suffix)		(strlen((suffix)) < strlen((str)) && !strcmp((str) + strlen((str)) - strlen((suffix)), suffix))
#define	HAS_PREFIX(str, prefix)		(strstr((str), (prefix)) == (str))

static char *desktop_directory = NULL;

static char *find_desktop() {
	char *desktop_dir, *home_dir, *desktop_path;
	struct stat stat_s;

	if ((desktop_dir = getenv("XDG_DESKTOP_DIR")))
		return strdup(desktop_dir);
	if (!(home_dir = getenv("HOME"))) {
		fprintf(stderr, "Unable to locate home directory\n");
		return NULL;
	}

	asprintf(&desktop_path, "%s/Desktop", home_dir);
	if (stat(desktop_path, &stat_s) < 0 || !S_ISDIR(stat_s.st_mode)) {
		fprintf(stderr, "Guessed $HOME/Desktop, which doesn't exist. Please configure your $XDG_DESKTOP_DIR.\n");
		return free(desktop_dir), NULL;
	}

	return desktop_path;
}

static void nuke_desktop(char *desktop) {
	char *full_path;
	DIR *dir_s;
	struct dirent ent, *result;

	if (!(dir_s = opendir(desktop))) {
		fprintf(stderr, "Unable to open '%s' for directory list\n", desktop);
		return;
	}

	for (readdir_r(dir_s, &ent, &result); result; readdir_r(dir_s, &ent, &result)) {
		if (HAS_PREFIX(ent.d_name, "__dbp__") && HAS_SUFFIX(ent.d_name, ".desktop")) {
			asprintf(&full_path, "%s/%s", desktop, ent.d_name);
			unlink(full_path);
			free(full_path);
		}
	}

	closedir(dir_s);
}

static void copy_file(const char *source, const char *destination, int thread) {
	char buff[8192];
	char *temp, *dest_path, *temp_path;
	FILE *in, *out = NULL;
	
	temp = strdup(destination), temp_path = dirname(temp);
	asprintf(&dest_path, "%s/.dbp-desktopd-temp%i", temp_path, thread);

	if ((in = fopen(source, "r")) && (out = fopen(dest_path, "w")))
		while (fwrite(buff, 1, fread(buff, 1, 8192, in), out) > 0);

	if (in) fclose(in); if (out) fclose(out);
	// apparently desktop files should be +x //
	chmod(dest_path, 0755);
	rename(dest_path, destination);
	free(temp); free(dest_path);
}

static void add_meta(const char *path, int thread) {
	char *desktop_path, *base, *base_comp;

	base = strdup(path), base_comp = basename(base);
	asprintf(&desktop_path, "%s/%s", desktop_directory, base_comp);
	if (HAS_PREFIX(base_comp, "__dbp__") && HAS_SUFFIX(base_comp, ".desktop")) {
		copy_file(path, desktop_path, thread);
	} else
		fprintf(stderr, "WARNING: Requested to copy a file that isn't a DBP .desktop file: %s\n", base_comp);
	free(desktop_path), free(base);
}

static void remove_meta(const char *path) {
	char *desktop_base, *desktop_copy, *target;
	
	desktop_copy = strdup(path); desktop_base = basename(desktop_copy);
	asprintf(&target, "%s/%s", desktop_directory, desktop_base);
	if (HAS_PREFIX(desktop_base, "__dbp__") && HAS_SUFFIX(desktop_base, ".desktop"))
		unlink(target);
	else
		fprintf(stderr, "WARNING: Requested to remove a file that isn't a DBP .desktop file\n");
	free(target);
	free(desktop_copy);
	return;
}

static void add_package_meta(char *pkg_id) {
	char *prefix, *source;
	DIR *dir_s;
	struct dirent ent, *result;

	asprintf(&prefix, "__dbp__%s_", pkg_id);
	
	if (!(dir_s = opendir(dbp_config_struct.desktop_directory))) {
		fprintf(stderr, "Unable to open '%s' for directory list\n", dbp_config_struct.desktop_directory);
		return free(prefix);
	}

	for (readdir_r(dir_s, &ent, &result); result; readdir_r(dir_s, &ent, &result)) {
		if (HAS_PREFIX(ent.d_name, prefix) && HAS_SUFFIX(ent.d_name, ".desktop")) {
			asprintf(&source, "%s/%s", dbp_config_struct.desktop_directory, ent.d_name);
			add_meta(source, 0);
			free(source);
		}
	}
	
	closedir(dir_s);
	free(prefix);
}

static void sighndlr(const char *signal, const char *value, void *data) {
	(void) data;
	if (!strcmp(signal, "NewMeta")) add_meta(value, 1);
	else if (!strcmp(signal, "RemoveMeta")) remove_meta(value);
}

int main(int argc, char **argv) {
	struct DBPList *list, *next;
	(void) argc; (void) argv;
	dbp_init(NULL);
	if (!(desktop_directory = find_desktop()))
		return 1;
	nuke_desktop(desktop_directory);
	dbpmgr_server_signal_listen(sighndlr, NULL);
	if (dbpmgr_server_connect() < 0) {
		fprintf(stderr, "Unable to connect to dbpd\n");
		return 1;
	}
		
	list = dbpmgr_server_package_list();
	for (next = list; next; next = next->next)
		if (next->on_desktop)
			add_package_meta(next->id);
	dbpmgr_server_package_list_free(list);
	pthread_join(server_signal_th, (void **) &next);
	return 0;
}
