#include <string.h>
#include <libgen.h>
#include <stdio.h>
#include "state.h"
#include "desktop.h"
#include "package.h"
#include "config.h"
#include "loop.h"
#include "dbp.h"

/* When calling any of these funtions, other threads must not exist. Else, things will blow up horribly */


/* time_t isn't 64-bit on all systems, although this will not be a major issue for a while... */
signed long long state_btime() {
	FILE *fp;
	signed long long t;
	char buff[4096];
	
	if (!(fp = fopen("/proc/stat", "r")))
		return -1;
	while (!feof(fp)) {
		fgets(buff, 4096, fp);
		if (sscanf(buff, "btime %lli\n", &t) < 1)
			continue;
		fclose(fp);
		return t;
	}

	fclose(fp);
	return -1;
}


void state_dump(struct package_s *p) {
	struct DBPDesktopFile *df;
	char buff[16], buff2[16], *dirname_s, *dir;
	int i, section;

	fprintf(stderr, "*** Dumping state ***\n");
	
	df = dbp_desktop_parse("Type=DBPStateFile\n");

	section = dbp_desktop_section_new(df, "DBPStateControl");

	snprintf(buff, 16, "%lli", state_btime());
	dbp_desktop_entry_new(df, "SystemBootup", "", buff, section);
	snprintf(buff, 16, "%i", p->run_cnt);
	dbp_desktop_entry_new(df, "RunCnt", "", buff, section);
	section = dbp_desktop_section_new(df, "Instances");
	
	for (i = 0; i < p->instances; i++) {
		snprintf(buff, 16, "%u", p->instance[i].run_id);
		dbp_desktop_entry_new(df, "PkgId", buff, p->instance[i].package_id, section);
		snprintf(buff2, 16, "%i", p->instance[i].loop);
		dbp_desktop_entry_new(df, "Loop", buff, buff2, section);
	}
	
	fprintf(stderr, "Writing state dump...\n");
	dirname_s = strdup(dbp_config_struct.state_file);
	dir = dirname(dirname_s);
	dbp_loop_directory_setup(dir, 0700);
	free(dirname_s);
	dbp_desktop_write(df, dbp_config_struct.state_file);
	dbp_desktop_free(df);

	fprintf(dbp_error_log, "State dumping complete\n");
	return;
}


static int state_add_instance(struct package_s *p) {
	struct package_instance_s *pi;
	int i;

	i = p->instances++;
	if (!(pi = realloc(p->instance, p->instances * sizeof(*pi))))
		return (p->instances--, -1);
	p->instance = pi;
	p->instance[i].package_id = NULL, p->instance[i].run_id = -1, p->instance[i].loop = -1;
	return i;
}


static int state_get_instance(struct package_s *p, const char *run_id) {
	int i;
	unsigned int run, id;

	sscanf(run_id, "%u", &run);
	for (i = 0; i < p->instances; i++)
		if (p->instance[i].run_id == run)
			return i;
	id = state_add_instance(p);
	p->instance[id].run_id = run;
	p->instance[id].loop = -1;
	p->instance[id].package_id = NULL;
	return id;
}


void state_recover(struct package_s *p) {
	int i, id, section, valid;
	struct DBPDesktopFile *df;
	signed long long boot, rboot;
	char *s;

	if (!(df = dbp_desktop_parse_file(dbp_config_struct.state_file)))
		return;
	if (!(s = dbp_desktop_lookup(df, "SystemBootup", "", "DBPStateControl")))
		return;
	
	rboot = atoll(s);
	boot = state_btime();
	
	/* FIXME: This may lead to issues during DST transistion. Hopefully not a big issue */
	/* Apparently, during second transition, this value may wiggle a little */
	if (rboot - boot < -1 || rboot - boot > 1) {
		fprintf(dbp_error_log, "Ignoring stale state file\n");
		return;
	}

	if ((section = dbp_desktop_lookup_section(df, "Instances")) < 0) {
		fprintf(dbp_error_log, "Section Instances is missing, Ignoring state file\n");
		return;
	}

	if (!(s = dbp_desktop_lookup(df, "RunCnt", "", "DBPStateControl"))) {
		fprintf(dbp_error_log, "RunCnt is missing from state file. Ignoring state file\n");
		return;
	}

	p->run_cnt = atoll(s);

	for (i = 0; i < df->section[section].entries; i++) {
		id = state_get_instance(p, df->section[section].entry[i].locale);
		if (!strcmp(df->section[section].entry[i].key, "PkgId"))
			p->instance[id].package_id = (free(p->instance[id].package_id), strdup(df->section[section].entry[i].value));
		else if (!strcmp(df->section[section].entry[i].key, "Loop"))
			p->instance[id].loop = atoi(df->section[section].entry[i].value);
	}

	valid = 1;
	/* Validate the data loaded */
	for (i = 0; i < df->section[section].entries; i++) {
		if (!p->instance[id].package_id || p->instance[id].loop < 0) {
			valid = 0;
			fprintf(dbp_error_log, "Invalid state file, bad entry at id=%i (run_id=%i, loop=%i, pkgid=%s\n", i, p->instance[id].run_id, p->instance[id].loop, p->instance[id].package_id);
		}
	}

	if (!valid) {
		fprintf(dbp_error_log, "Invalid state was detected, nuking state...\n");
		for (i = 0; i < p->instances; i++)
			free(p->instance[i].package_id);
		free(p->instance);
		p->instance = NULL, p->instances = 0;
	}

	dbp_desktop_free(df);

	return;
}
