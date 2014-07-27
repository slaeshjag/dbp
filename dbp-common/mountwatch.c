#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include <unistd.h>
#include "mountwatch.h"

/* I keep almost typing mouthwash.. <.< */
struct mountwatch_s mountwatch_struct;

void *mountwatch_loop(void *null) {
	int mountfd;
	fd_set watch;

	if ((mountfd = open("/proc/mounts", O_RDONLY, 0)) < 0) {
		fprintf(stderr, "Unable to open /proc/mounts\n");
		pthread_exit(NULL);
	}

	for (;;) {
		FD_ZERO(&watch);
		FD_SET(mountfd, &watch);
		select(mountfd + 1, NULL, NULL, &watch, NULL);
		sem_post(&mountwatch_struct.changed);
	}
}


int mountwatch_init() {
	pthread_t th;

	sem_init(&mountwatch_struct.changed, 0, 1);
	mountwatch_struct.entry = NULL, mountwatch_struct.entries = 0;
	if (pthread_create(&th, NULL, mountwatch_loop, NULL)) {
		fprintf(stderr, "Error: Unable to create mountpoint watch process\n");
		return 0;
	}

	return 1;
}


int mountwatch_change_add(struct mountwatch_change_s *change, const char *mount, const char *device, int tag) {
	int id;

	id = change->entries++;
	change->entry = realloc(change->entry, sizeof(*change->entry) * change->entries);
	change->entry[id].device = strdup(device);
	change->entry[id].mount = strdup(mount);
	change->entry[id].tag = tag;
	return id;
}


struct mountwatch_change_s mountwatch_diff() {
	struct mountwatch_change_s change;
	FILE *fp;
	char mount[256], device[256];
	int i, n;

	wait:
	sem_wait(&mountwatch_struct.changed);

	change.entry = NULL, change.entries = 0;

	if (!(fp = fopen("/proc/mounts", "r"))) {
		fprintf(stderr, "Unable to open /proc/mounts\n");
		return change;
	}


	while (!feof(fp)) {
		*mount = *device = 0;
		fscanf(fp, "%256s %256s \n", device, mount);

		if (*device != '/')
			/* Special filesystem, ignore */
			/* This will break sshfs, samba etc, but that's	*
			** probably a good thing 			*/
			continue;

		if (strstr(device, "/dev/loop") == device)
			/* Loop-back devices should probably not be	*
			** watched..					*/
			continue;

		for (i = 0; i < mountwatch_struct.entries; i++) {
			if (!strcmp(mountwatch_struct.entry[i].mount, mount)) {
				mountwatch_struct.entry[i].tag = 1;
				if (strcmp(mountwatch_struct.entry[i].device, device)) {
					mountwatch_change_add(&change, mount, device, MOUNTWATCH_TAG_CHANGED);
					free(mountwatch_struct.entry[i].mount);
					mountwatch_struct.entry[i].mount = strdup(mount);
					mountwatch_struct.entry[i].tag = 1;
				} else
					break;
			}
		}

		if (i == mountwatch_struct.entries) {
			mountwatch_change_add(&change, mount, device, MOUNTWATCH_TAG_ADDED);
			n = mountwatch_struct.entries++;
			mountwatch_struct.entry = realloc(mountwatch_struct.entry,
			    sizeof(*mountwatch_struct.entry) * mountwatch_struct.entries);
			mountwatch_struct.entry[n].mount = strdup(mount);
			mountwatch_struct.entry[n].device = strdup(device);
			mountwatch_struct.entry[n].tag = 1;
		}
	}

	for (i = 0; i < mountwatch_struct.entries; i++) {
		if (!mountwatch_struct.entry[i].tag) {
			mountwatch_change_add(&change, mountwatch_struct.entry[i].device,
			    mountwatch_struct.entry[i].device, MOUNTWATCH_TAG_REMOVED);

			free(mountwatch_struct.entry[i].mount);
			free(mountwatch_struct.entry[i].device);
			memmove(&mountwatch_struct.entry[i], &mountwatch_struct.entry[i + 1],
			    (mountwatch_struct.entries - 1 - i) * sizeof(*mountwatch_struct.entry));
			mountwatch_struct.entries--;
			i--;
		} else
			mountwatch_struct.entry[i].tag = 0;
	}

	fclose(fp);
	
	if (!change.entries)
		goto wait;

	return change;
}


void mountwatch_change_free(struct mountwatch_change_s change) {
	int i;
	for (i = 0; i < change.entries; i++)
		free(change.entry[i].device), free(change.entry[i].mount);
	free(change.entry);
	return;
}
