#define	_GNU_SOURCE
#include "desktop.h"
#include "package.h"
#include "config.h"
#include "loop.h"
#include "meta.h"
#include "dbp.h"
#include "comm.h"

#include <archive.h>
#include <archive_entry.h>

#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <limits.h>

/* The line-count in this file is too damn high! */
static void package_meta_exec_export(const char *exec, int env, struct package_s *p, int id);
static int package_purgatory_add(struct package_s *p, const char *pkg_id, int loop, int reusable);
static int package_purgatory_revive(struct package_s *p, const char *pkgid);


struct package_s package_init() {
	struct package_s p;

	pthread_mutex_init(&p.mutex, NULL);
	p.entry = NULL;
	p.entries = 0;
	p.instance = NULL;
	p.instances = 0;
	p.run_cnt = 0;
	p.purgatory = NULL;
	p.purgatory_entries = 0;
	return p;
}


static int package_filename_interesting(const char *path) {
	int i;
	
	for (i = 0; i < dbp_config_struct.file_extensions; i++) {
		if (strlen(path) < strlen(dbp_config_struct.file_extension[i]))
			continue;
		if (!strcmp(&path[strlen(path) - strlen(dbp_config_struct.file_extension[i])],
		    dbp_config_struct.file_extension[i]))
			return 1;
	}
	return 0;
}


static int package_find(struct package_s *p, const char *id) {
	int i;

	for (i = 0; i < p->entries; i++)
		if (!strcmp(id, p->entry[i].id))
			return i;
	return -1;
}


static int package_id_validate(const char *pkg_id) {
	int i;
	if (!strcmp(pkg_id, "..") || !strcmp(pkg_id, "."))
		return 0;

	for (i = 0; pkg_id[i]; i++)
		if (!(pkg_id[i] >= 'a' && pkg_id[i] <= 'z') && !(pkg_id[i] >= 'A' && pkg_id[i] <= 'Z')
		    && !(pkg_id[i] >= '0' && pkg_id[i] <= '9') && pkg_id[i] != '-' && pkg_id[i] != '_' && pkg_id[i] != '.')
			return 0;
	return 1;
}


static int package_add(struct package_s *p, char *path, char *id, char *device, char *mount, char *appdata, char *sys, char *pkg, char *version) {
	int nid, i, errid;

	for (i = 0; i < p->entries; i++) {
		if (!package_id_validate(id))
			goto badid;
		if (!strcmp(p->entry[i].id, id))
			goto noreg;
	}

	/* pga. ev. "//", gör en fuling */
	if (strstr(path, "//") == path)
		memmove(path, path + 1, strlen(path));

	nid = p->entries++;
	p->entry = realloc(p->entry, sizeof(*p->entry) * p->entries);
	p->entry[nid].path = path;
	p->entry[nid].id = id;
	p->entry[nid].device = device;
	p->entry[nid].mount = mount;
	p->entry[nid].appdata = appdata;
	p->entry[nid].sys_dep = sys;
	p->entry[nid].pkg_dep = pkg;
	p->entry[nid].version = version;
	p->entry[nid].desktop = strdup(dbp_loop_desktop_directory(path) ? "desk" : "nodesk");
	p->entry[nid].exec = NULL, p->entry[nid].execs = 0;
	comm_dbus_announce_new_package(id);

	return nid;

	badid:
	fprintf(dbp_error_log, "Package at '%s' has illegal package ID %s\n", path, id);
	errid = DBP_ERROR_BAD_PKG_ID;
	goto fail;

	noreg:
	fprintf(dbp_error_log, "Package %s is already registered at %s\n", id, p->entry[i].path);
	errid = DBP_ERROR_PKG_REG;
	goto fail;

	fail:
	free(path), free(id), free(device), free(mount), free(sys), free(pkg), free(appdata);
	return errid;
}


static int package_id_lookup(struct package_s *p, const char *pkg_id) {
	int i;

	for (i = 0; i < p->entries; i++)
		if (!strcmp(p->entry[i].id, pkg_id))
			return i;
	return -1;
}


/* If basename(3) didn't suck, this function wouldn't be needed */
static const char *find_filename(const char *path) {
	char *tmp;
	return (tmp = strrchr(path, '/')) ? tmp + 1 : path;
}


static void package_desktop_write(struct package_s *p, int id, const char *fname, char *data, int announce) {
	char writename[PATH_MAX], *override_path;
	const char *pkg_id = p->entry[id].id;
	struct DBPDesktopFile *df;
	int sec, ent;
	FILE *fp;

	asprintf(&override_path, "%s/%s/%s/%s_%s_ovr", p->entry[id].mount, dbp_config_struct.data_directory, p->entry[id].appdata, pkg_id, fname);
	df = dbp_desktop_parse_file(override_path);
	free(override_path);
	if (df)
		df = dbp_desktop_parse_append(data, df);
	else
		df = dbp_desktop_parse(data);
	
	if ((sec = dbp_desktop_lookup_section(df, "Desktop Entry")) < 0)
		goto write;
	if ((ent = dbp_desktop_lookup_entry(df, "Icon", "", sec)) < 0)
		goto write;
	
	snprintf(writename, PATH_MAX, "%s/%s%s_%s", dbp_config_struct.icon_directory, DBP_META_PREFIX, pkg_id, df->section[sec].entry[ent].value);
	free(df->section[sec].entry[ent].value);
	df->section[sec].entry[ent].value = strdup(writename);

	write:
	snprintf(writename, PATH_MAX, "%s/%s%s_%s", dbp_config_struct.desktop_directory, DBP_META_PREFIX, pkg_id, fname);
	if (!(fp = fopen(writename, "w")))
		goto desktop_free;
	dbp_desktop_write(df, writename);
	chmod(writename, 0755);
	package_meta_exec_export(dbp_desktop_lookup(df, "Exec", "", "Package Entry"), 1, p, id);
	package_meta_exec_export(dbp_desktop_lookup(df, "NoEnvExec", "", "Package Entry"), 0, p, id);
	if (announce)
		comm_dbus_announce_new_meta(writename);

	desktop_free:
	dbp_desktop_free(df);
	return;
}


static void package_emit_exec(const char *path, const char *bin, int env, const char *id) {
	FILE *fp, *out;
	int sz, tok_var;
	char *script, *saveptr = NULL, *tok;

	if (!(fp = fopen(dbp_config_struct.exec_template, "r"))) {
		fprintf(dbp_error_log, "Unable to open exec template %s\n", dbp_config_struct.exec_template);
		return;
	}

	if (!(out = fopen(path, "w"))) {
		fprintf(dbp_error_log, "Unable to open exec '%s' for writing\n", path);
		fclose(fp);
		return;
	}
	
	fseek(fp, 0, SEEK_END);
	sz = ftell(fp);
	rewind(fp);

	if (!(script = malloc(sz + 1)))
		goto done;

	fread(script, 1, sz, fp);
	script[sz] = 0;

	for (tok_var = 0, tok = strtok_r(script, "!", &saveptr); tok; tok = strtok_r(NULL, "!", &saveptr)) {
		if (!tok_var) {
			fprintf(out, "%s", tok);
		} else {
			if (tok[0] != '%') {
				tok_var = 0, fprintf(out, "!%s", tok);
				continue;
			}

			tok++;
			/* Not the most efficient wat to do it, but I	*
			** dunno yet if speed's going to be an issue 	*/
			if (!strcmp(tok, "package_id"))
				fprintf(out, "%s", id);
			else if (!strcmp(tok, "package_binary"))
				fprintf(out, "%s", bin);
			else if (!strcmp(tok, "package_enviroment"))
				fprintf(out, "%i", env);
			else
				fprintf(dbp_error_log, "Unhandled sequence %s\n", tok);
		}
		
		tok_var = !tok_var;
	}

	chmod(path, 0755);
	
	done:
	fclose(fp);
	fclose(out);
	free(script);
	return;
}


static void package_meta_exec_export(const char *exec, int env, struct package_s *p, int id) {
	char path[PATH_MAX], *exec_tok, *saveptr = NULL, *tok;
	int exec_id;

	if (!exec)
		return;
	exec_tok = strdup(exec);
	for (tok = strtok_r(exec_tok, ";", &saveptr); tok; tok = strtok_r(NULL, ";", &saveptr)) {
		snprintf(path, PATH_MAX, "%s/%s", dbp_config_struct.exec_directory, find_filename(tok));
		if (!access(path, F_OK)) {
			fprintf(dbp_error_log, "Executable collision! %s already exists\n", path);
			continue;
		}

		package_emit_exec(path, tok, env, p->entry[id].id);
		exec_id = p->entry[id].execs++;
		p->entry[id].exec = realloc(p->entry[id].exec, sizeof(*p->entry[id].exec) * p->entry[id].execs);
		p->entry[id].exec[exec_id] = strdup(find_filename(tok));
	}

	free(exec_tok);

	return;
}


static void package_meta_extract(const char *path, struct package_s *p, int id, int announce) {
	struct archive *a;
	struct archive_entry *ae;
	const char *pkg_id = p->entry[id].id;
	char *pathname, writename[PATH_MAX], *data;
	int size;
	FILE *fp;

	if (!(a = archive_read_new()))
		return;
	archive_read_support_format_zip(a);
	if (archive_read_open_filename(a, path, 512) != ARCHIVE_OK)
		return;
	while (archive_read_next_header(a, &ae) == ARCHIVE_OK) {
		pathname = (char *) archive_entry_pathname(ae);
		if (strlen(pathname) > 256) /* Don't be over-doing it.. */
			continue;

		/* ".desktop" = 8 */
		if (strlen(pathname) < 8)	/* Not a .desktop */
			if (strstr(pathname, "icons/") != pathname)	/* Not icon */
				continue;
		if (strstr(pathname, "icons/") == pathname || !strcmp(&pathname[strlen(pathname) - 8], ".desktop")) {
			data = malloc((size = archive_entry_size(ae)) + 1);
			if (!data)
				continue;
			archive_read_data(a, data, size);
			data[size] = 0;
			if (strstr(pathname, "icons/") == pathname) {
				snprintf(writename, PATH_MAX, "%s/%s%s_%s", 
				    dbp_config_struct.icon_directory, DBP_META_PREFIX, pkg_id,
				    find_filename(pathname));
				
				if (!(fp = fopen(writename, "w"))) {
					free(data);
					continue;
				}

				fwrite(data, 1, size, fp);
				fclose(fp);
				chmod(writename, 0755);
			} else {
				package_desktop_write(p, id, find_filename(pathname), data, announce);
				/* TODO: Extract executables */
			} 
			free(data);
		}
	}
	
	archive_read_free(a);
	return;
}


static int package_register(struct package_s *p, const char *path, const char *device, const char *mount, int *coll_id) {
	struct DBPMetaPackage mp;
	char *pkg_id = "none", *appdata, *sys, *pkg, *version = NULL;
	int id, errid;

	*coll_id = -1;
	if ((errid = dbp_meta_package_open(path, &mp)) < 0)
		goto error;
	if (!(pkg_id = dbp_desktop_lookup(mp.df, "Id", "", mp.section))) {
		errid = DBP_ERROR_BAD_PKG_ID;
		goto error;
	}

	if (!(appdata = dbp_desktop_lookup(mp.df, "Appdata", "", mp.section)))
		appdata = pkg_id;
	else if (!package_id_validate(appdata))
		appdata = pkg_id;
	if (!(sys = dbp_desktop_lookup(mp.df, "SysDependency", "", mp.section)))
		sys = "";
	if (!(pkg = dbp_desktop_lookup(mp.df, "PkgDependency", "", mp.section)))
		pkg = "";
	if (!(version = dbp_desktop_lookup(mp.df, "Version", "", mp.section)))
		version = "";
	
	if ((id = package_add(p, strdup(path), strdup(pkg_id), strdup(device), strdup(mount), strdup(appdata), strdup(sys), strdup(pkg), strdup(version))) < 0) {
		*coll_id = package_id_lookup(p, pkg_id);
		errid = id;
		pkg_id = NULL;
		goto error;
	}

	package_meta_extract(path, p, id, !strcmp(p->entry[id].desktop, "desk"));
	fprintf(dbp_error_log, "Registered package %s at '%s'\n", pkg_id, path);
	
	mp.df = dbp_desktop_free(mp.df);

	return id;

	error:
	fprintf(dbp_error_log, "An error occured while registering a package %s at '%s'\n", pkg_id, path);
	if (mp.df)
		mp.df = dbp_desktop_free(mp.df);
	return errid;
}


int package_register_path(struct package_s *p, const char *device, const char *path, const char *mount, char **pkg_id) {
	int i, n;

	if (!package_filename_interesting(path)) {
		*pkg_id = strdup("!");
		return DBP_ERROR_BADEXT;
	}

	pthread_mutex_lock(&p->mutex);
	i = package_register(p, path, device, mount, &n);
	if ((i < 0 || i >= p->entries) && n < 0)
		*pkg_id = strdup("!");
	else
		*pkg_id = strdup(p->entry[n >= 0 ? n : i].id);
	pthread_mutex_unlock(&p->mutex);
	return i;
}
	


static void package_crawl(struct package_s *p, const char *device, const char *path, const char *mount) {
	DIR *d;
	struct dirent dir, *res;
	char *name_buff;
	int n;

	if (!(d = opendir(path))) {
		fprintf(dbp_error_log, "Unable to open %s for directory list\n", path);
		return;
	}

	for (readdir_r(d, &dir, &res); res; readdir_r(d, &dir, &res)) {
		if (!package_filename_interesting(dir.d_name))
			continue;
		name_buff = malloc(strlen(path) + 2 + strlen(dir.d_name));
		snprintf(name_buff, PATH_MAX, "%s/%s", path, dir.d_name);
		package_register(p, name_buff, device, mount, &n);
		free(name_buff);
	}

	closedir(d);

	return;
}


/* crawl mount/release are only called from the main thread, in the mountpoint
** watch handler */

void package_crawl_mount(struct package_s *p, const char *device, const char *path) {
	int i;
	char *new_path = NULL;
	
	pthread_mutex_lock(&p->mutex);

	for (i = 0; i < dbp_config_struct.search_dirs; i++) {
		new_path = realloc(new_path, strlen(path) + 2 + strlen(dbp_config_struct.search_dir[i]));
		snprintf(new_path, PATH_MAX, "%s/%s", path, dbp_config_struct.search_dir[i]);
		package_crawl(p, device, new_path, path);
	}
	
	free(new_path);
	pthread_mutex_unlock(&p->mutex);

	return;
}


static void package_kill_prefix(const char *dir, const char *prefix, int meta) {
	char full[PATH_MAX];
	DIR *d;
	struct dirent de, *result;

	if (!(d = opendir(dir)))
		return;
	for (readdir_r(d, &de, &result); result; readdir_r(d, &de, &result))
		if (strstr(de.d_name, prefix) == de.d_name) {
			snprintf(full, PATH_MAX, "%s/%s", dir, de.d_name), unlink(full);
			if (meta)
				comm_dbus_announce_rem_meta(full);
		}
	closedir(d);
	return;
}


static void package_meta_remove(const char *pkg_id, int announce) {
	char prefix[PATH_MAX];

	snprintf(prefix, PATH_MAX, "%s%s_", DBP_META_PREFIX, pkg_id);
	package_kill_prefix(dbp_config_struct.icon_directory, prefix, 0);
	package_kill_prefix(dbp_config_struct.desktop_directory, prefix, announce);

	return;
}


static void package_kill(struct package_s *p, int entry) {
	int i;
	char ulinkpath[PATH_MAX];

	if (dbp_config_struct.verbose_output)
		fprintf(dbp_error_log, "Package at '%s' released\n", p->entry[entry].path);
	
	package_meta_remove(p->entry[entry].id, !strcmp(p->entry[entry].desktop, "desk"));
	for (i = 0; i < p->entry[entry].execs; i++) {
		snprintf(ulinkpath, PATH_MAX, "%s/%s", dbp_config_struct.exec_directory, p->entry[entry].exec[i]);
		unlink(ulinkpath);
		free(p->entry[entry].exec[i]);
	}

	fprintf(dbp_error_log, "Unregistering package %s\n", p->entry[entry].id);
	comm_dbus_announce_rem_package(p->entry[entry].id);
	free(p->entry[entry].exec);
	free(p->entry[entry].device);
	free(p->entry[entry].id);
	free(p->entry[entry].path);
	free(p->entry[entry].mount);
	free(p->entry[entry].appdata);
	free(p->entry[entry].sys_dep);
	free(p->entry[entry].pkg_dep);
	free(p->entry[entry].desktop);
	free(p->entry[entry].version);
	p->entries--;
	memmove(&p->entry[entry], &p->entry[entry + 1], (p->entries - entry) * sizeof(*p->entry));
	return;
}


int package_release_path(struct package_s *p, const char *path) {
	int i, killed = 0;
	pthread_mutex_lock(&p->mutex);

	if (strstr(path, "//") == path) path++;
	for (i = 0; i < p->entries; i++)
		if (!strcmp(p->entry[i].path, path)) {
			package_kill(p, i), killed = 1;
			break;
		}
	pthread_mutex_unlock(&p->mutex);
	
	return killed?killed:DBP_ERROR_NOTFOUND;
}


void package_release_mount(struct package_s *p, const char *device) {
	int i;
	void *last = NULL;

	pthread_mutex_lock(&p->mutex);

	fprintf(dbp_error_log, "Umount '%s'\n", device);

	for (i = 0; i < ((volatile int) p->entries); i++) {
		// ID won't change if we failed to kill it last time, and we don't alloc any new
		// ID's somewhere else while this loop is running, since the struct is locked
		if (p->entry[i].id == last) {
			fprintf(dbp_error_log, "Package '%s' seems unkillable for now\n", p->entry[i].id);
			continue;	// Unkillable, for some reason
		}

		last = p->entry[i].id;
		if (strcmp(p->entry[i].device, device)) {
			if (dbp_config_struct.verbose_output)
				fprintf(dbp_error_log, "Package '%s' does not reside on '%s', it's on '%s', skipping\n", p->entry[i].path, device, p->entry[i].device);
			continue;
		}

		package_kill(p, i);
		i--;

	}

	pthread_mutex_unlock(&p->mutex);
	return;
}


/* run/stop is called in the dbus thread, nowhere else */
int package_run(struct package_s *p, const char *id, const char *user) {
	int i, loop, pkg_n;
	void *instance;

	pthread_mutex_lock(&p->mutex);
	/* Find out if the package is already in use, in that case we don't
	** need to mount it again */

	for (i = 0; i < p->instances; i++)
		if (!strcmp(p->instance[i].package_id, id)) {
			loop = p->instance[i].loop;
			goto mounted;
		}
	if ((loop = package_purgatory_revive(p, id)) < -1) {
		pthread_mutex_unlock(&p->mutex);
		return DBP_ERROR_MUTILATED;
	} else if (loop > -1)
		goto mounted;

	if ((pkg_n = package_find(p, id)) < 0) {
		pthread_mutex_unlock(&p->mutex);
		return DBP_ERROR_BAD_PKG_ID;
	}

	if ((loop = dbp_loop_mount(p->entry[pkg_n].path, id, user, p->entry[pkg_n].mount, p->entry[pkg_n].appdata)) < 0) {
		pthread_mutex_unlock(&p->mutex);
		return loop;
	}
	
	mounted:
	i = p->instances++;
	if (!(instance = realloc(p->instance, sizeof(*p->instance) * p->instances))) {
		pthread_mutex_unlock(&p->mutex);
		return DBP_ERROR_NO_MEMORY;
	}

	p->instance = instance;
	p->instance[i].package_id = strdup(id);
	p->instance[i].run_id = p->run_cnt++;
	p->instance[i].loop = loop;
	if (p->run_cnt < 0) {
		fprintf(dbp_error_log, "Run count wrapped around. A bumpy road might await\n");
		p->run_cnt = 0;
	}

	pthread_mutex_unlock(&p->mutex);
	return p->instance[i].run_id;
}

static int package_purgatory_add(struct package_s *p, const char *pkg_id, int loop, int reusable) {
	struct package_purgatory_s *new;
	int id;

	id = p->purgatory_entries++;
	if (!(new = realloc(p->purgatory, p->purgatory_entries * sizeof(*p->purgatory)))) {
		fprintf(dbp_error_log, "ERROR: Can't allocate purgatory space, resources lost in the void. Why can't I hold all these memory blocks?\n");
		p->purgatory_entries--;
		return 0;
	}

	p->purgatory = new;
	p->purgatory[id].reusable = reusable;
	p->purgatory[id].loop_number = loop;
	p->purgatory[id].package_id = strdup(pkg_id);
	p->purgatory[id].expiry = time(NULL) + 5; // Somewhere between 4 and 5 seconds is probably good //

	return 1;
}

static void package_purgatory_remove(struct package_s *p, int id) {
	free(p->purgatory[id].package_id);
	p->purgatory_entries--;
	memmove(&p->purgatory[id], &p->purgatory[id + 1], (p->purgatory_entries - id) * sizeof(*p->purgatory));
	return;
}

static int package_purgatory_find(struct package_s *p, const char *pkgid) {
	int i;

	for (i = 0; i < p->purgatory_entries; i++)
		if (!strcmp(pkgid, p->purgatory[i].package_id))
			return i;
	return -1;
}

// Return loop number //
static int package_purgatory_revive(struct package_s *p, const char *pkgid) {
	int i, loop;

	if ((i = package_purgatory_find(p, pkgid)) < 0)
		return -1;
	if (p->purgatory[i].reusable < 1)
		return -2;
	loop = p->purgatory[i].loop_number;
	package_purgatory_remove(p, i);
	return loop;
}

static int package_purgatory_reap(struct package_s *p, int id) {
	p->purgatory[id].reusable = dbp_loop_umount(p->purgatory[id].package_id, p->purgatory[id].loop_number, NULL, p->purgatory[id].reusable);
	if (!p->purgatory[id].reusable) {
		package_purgatory_remove(p, id);
		return 1;
	} else if (dbp_config_struct.verbose_output)
		fprintf(dbp_error_log, "Unable to reap package %s in purgatory, reason %i\n", p->purgatory[id].package_id, p->purgatory[id].reusable);
	return 0;
}

void package_purgatory_check(struct package_s *p) {
	int i;

	pthread_mutex_lock(&p->mutex);
	
	for (i = 0; i < p->purgatory_entries; i++)
		if (p->purgatory[i].reusable < 1 || p->purgatory[i].expiry)
			if (package_purgatory_reap(p, i))
				i--;

	pthread_mutex_unlock(&p->mutex);
	return;
}

int package_stop(struct package_s *p, int run_id) {
	int i, rid;
	const char *id = NULL;
	
	pthread_mutex_lock(&p->mutex);

	/* Find out if this is the last instance in this package */
	for (i = 0; i < p->instances; i++) {
		if ((int) p->instance[i].run_id == run_id) {
			id = p->instance[i].package_id;
			rid = i;
			break;
		}
	}

	if (!id) {
		fprintf(dbp_error_log, "Requested to stop instance with invalid id %i\n", run_id);
		goto done;
	}

	for (i = 0; i < p->instances; i++) {
		if (!strcmp(p->instance[i].package_id, id) && (int) p->instance[i].run_id != run_id)
			/* Other instances are using the package, do not umount */
			goto umount_done;
	}
	
	/* TODO: Send the actual user instead of NULL */
	/* TODO: When mutilated instances are implemented, take that state into account before flagging as reusable */
	package_purgatory_add(p, p->instance[rid].package_id, p->instance[rid].loop, 1);

	umount_done:
	
	free(p->instance[rid].package_id);
	p->instances--;
	memmove(&p->instance[rid], &p->instance[rid + 1], sizeof(*p->instance) * (p->instances - rid));

	done:
	pthread_mutex_unlock(&p->mutex);
	return 1;
}


char *package_mount_get(struct package_s *p, const char *pkg_id) {
	int i;
	char *path;

	pthread_mutex_lock(&p->mutex);

	if ((i = package_find(p, pkg_id)) < 0) {
		pthread_mutex_unlock(&p->mutex);
		return NULL;
	}

	path = strdup(p->entry[i].mount);
	pthread_mutex_unlock(&p->mutex);
	return path;
}


char *package_id_from_path(struct package_s *p, const char *path) {
	int i;
	char *id;

	pthread_mutex_lock(&p->mutex);

	for (i = 0; i < p->entries; i++)
		if (!strcmp(p->entry[i].path, path))
			break;
	id = (i == p->entries) ? NULL : strdup(p->entry[i].id);
	pthread_mutex_unlock(&p->mutex);
	return id;
}


char *package_appdata_from_id(struct package_s *p, const char *id) {
	char *ad;
	int i;

	pthread_mutex_lock(&p->mutex);
	if ((i = package_find(p, id)) < 0)
		ad = strdup("!");
	else
		ad = strdup(p->entry[i].appdata);
	pthread_mutex_unlock(&p->mutex);
	return ad;
}


int package_deps_from_id(struct package_s *p, const char *id, char **sys, char **pkg) {
	int i;
	
	pthread_mutex_lock(&p->mutex);

	if ((i = package_find(p, id)) < 0)
		*sys = strdup(""), *pkg = strdup("");
	else
		*sys = strdup(p->entry[i].sys_dep), *pkg = strdup(p->entry[i].pkg_dep);
	pthread_mutex_unlock(&p->mutex);
	
	return 0;
}


char *package_path_from_id(struct package_s *p, const char *id) {
	int i;
	char *ret;
	
	pthread_mutex_lock(&p->mutex);
	
	if ((i = package_find(p, id)) < 0)
		ret = NULL;
	else
		ret = strdup(p->entry[i].path);
	pthread_mutex_unlock(&p->mutex);
	return ret;
}
