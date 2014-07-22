#include "desktop.h"


int main(int argc, char **argv) {
	FILE *fp;
	char *buff;
	long sz;
	struct desktop_file_s *df;

	fp = fopen(argv[1], "r");
	fseek(fp, 0, SEEK_END);
	sz = ftell(fp);
	buff = malloc(sz + 1);
	rewind(fp);
	fread(buff, sz, 1, fp);
	buff[sz] = 0;

	df = desktop_parse(buff);
	desktop_dump(df);
	desktop_free(df);

	return 0;
}
