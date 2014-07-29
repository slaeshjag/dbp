#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>

void util_lookup_mount(const char *path, char **mount, char **dev) {
	char buff[PATH_MAX], buff2[PATH_MAX], buff3[PATH_MAX], *best = NULL, *device = NULL;
	FILE *fp;

	if (!(fp = fopen("/proc/mounts", "r"))) {
		*mount = strdup(path), *dev = strdup("/");
		return;
	}
		
	
	while (!feof(fp)) {
		fgets(buff, PATH_MAX, fp);
		sscanf(buff, "%s %s", buff2, buff3);
		if (strstr(path, buff3) == path) {
			if (!best)
				best = strdup(buff3), device = strdup(buff2);
			else if (strlen(buff3) > strlen(best))
				free(best), free(device), best = strdup(buff3), device = strdup(buff2);
		}
	}

	fclose(fp);
	*mount = best;
	*dev = device;
	return;
}
