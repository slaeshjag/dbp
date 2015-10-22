#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

struct DPackage {
	char			*name;
	char			*version;
	char			*arch;
	struct DPackage		*next;

};

struct DPackageNode {
	struct DPackage		*match;
	struct DPackageNode	**lookup;
	struct DPackage		*list;
	int			list_size;
};

struct DPackageVersion {
	char			*epoch;
	char			*main;
	char			*rev;
};

static char char_to_index[256];
static int lookup_size = 0;

#define	IS_END(c)	(isdigit((c)) || !(c))

/* I have no idea what I'm doing... */
static int compare_substring(char *s1, char *s2) {
	int i;

	for (i = 0; !IS_END(s1[i]) && !IS_END(s2[i]); i++) {
		if (s1[i] == s2[i])
			continue;
		if (s1[i] == '~')
			return -1;
		if (s2[i] == '~')
			return 1;
		/* TODO: Find out if this is supposed to be case insensitive or not... */
		if (!isalpha(s1[i])) {
			if (!isalpha(s2[i]))
				return s1[i] < s2[i] ? -1 : 1;
			return -1;
		}
		
		if (isalpha(s1[i])) {
			if (isalpha(s2[i]))
				return s1[i] < s2[i] ? -1 : 1;
			return 1;
		}

	}

	if (IS_END(s1[i]) && IS_END(s2[i]))
		return 0;
	if (IS_END(s1[i]))
		return s2[i] != '~' ? -1 : 1;
	if (IS_END(s2[i]))
		return s1[i] != '~' ? 1 : -1;
	/* All cases handled */
	return 0;
}


struct DPackageVersion version_decode(char *version) {
	struct DPackageVersion pv = { NULL, NULL, NULL };
	char *t;

	if ((t = strchr(version, ':')))
		pv.epoch = strndup(version, t - version), version = t + 1;
	else
		pv.epoch = strdup("0");
	if ((t = strchr(version, '-'))) {
		for (; strchr(t, '-'); t = strchr(t, '-') + 1);
		pv.rev = strdup(t);
		pv.main = strndup(version, t - version - 1);
	} else {
		if (*version)
			pv.main = strdup(version);
		else
			pv.main = strdup("0");
		pv.rev = strdup("0");
	}
	return pv;
}

void version_free(struct DPackageVersion pv) {
	free(pv.epoch);
	free(pv.main);
	free(pv.rev);
}

static char *find_next_nonumeric(char *s) {
	while (isdigit(*s))
		s++;
	return s;
}

static char *find_next_numeric(char *s) {
	while (!isdigit(*s) && *s)
		s++;
	return s;
}

static int compare_version_string(char *s1, char *s2) {
	int n1, n2;

	while (*s1 && *s2) {
		n1 = n2 = 0;
		sscanf(s1, "%i", &n1);
		sscanf(s2, "%i", &n2);
		if (n1 < n2)
			return -1;
		if (n1 > n2)
			return 1;
		s1 = find_next_nonumeric(s1);
		s2 = find_next_nonumeric(s2);
		if ((n1 = compare_substring(s1, s2)))
			return n1;
		s1 = find_next_numeric(s1);
		s2 = find_next_numeric(s2);
	}

	if (!*s1 && *s2)
		return *s2 == '~' ? 1 : -1;
	if (*s1 && !*s2)
		return *s1 == '~' ? -1 : 1;

	return 0;
}


int compare_versions(char *ver1, char *ver2) {
	struct DPackageVersion v1, v2;
	int i;

	v1 = version_decode(ver1);
	v2 = version_decode(ver2);
	
	if (atoi(v1.epoch) != atoi(v2.epoch))
		return atoi(v1.epoch) < atoi(v2.epoch) ? -1 : 1;
	i = compare_version_string(v1.main, v2.main);
	if (!i)
		i = compare_version_string(v1.rev, v2.rev);
	version_free(v1);
	version_free(v2);
	return i;
}


static void init_conv_table() {
	int i, j;

	memset(char_to_index, 0, 256);
	for (i = 0, j = 1; i <= 'z' - 'a'; i++, j++)
		char_to_index['a' + i] = j;
	for (i = 0; i <= '9' - '0'; i++, j++)
		char_to_index['0' + i] = j;
	char_to_index['+'] = j++;
	char_to_index['-'] = j++;
	char_to_index['.'] = j++;
	lookup_size = j;
}

char *load_database() {
	FILE *fp;
	int length;	/* If the package database is > 2 GB, the user have bigger problems... */
	char *dbstr;

	if (!(fp = fopen("/var/lib/dpkg/status", "r")))
		return NULL;
	fseek(fp, 0, SEEK_END), length = ftell(fp), rewind(fp);
	if ((dbstr = malloc(length + 1)))
		fread(dbstr, length, 1, fp), dbstr[length] = 0;

	fclose(fp);
	return dbstr;
}


struct DPackageNode *package_tree_populate(struct DPackageNode *root, struct DPackage *entry, int name_index) {
	if (!entry)
		return root;
	if (!root)
		root = calloc(sizeof(*root), 1);
	if (!entry->name[name_index])
		return entry->next = root->match, root->match = entry, root;
	if (!root->lookup)
		entry->next = root->list, root->list = entry, root->list_size++;
	else
		return root->lookup[char_to_index[entry->name[name_index]]] = package_tree_populate(root->lookup[char_to_index[entry->name[name_index]]], entry, name_index + 1), root;
	if (root->list_size > 25) {	// Arbitrary value for when a list is "big"
		struct DPackage *next, *this;
		root->lookup = calloc(sizeof(*(root->lookup)), lookup_size);
		for (this = root->list; this; this = next)
			next = this->next, root->lookup[char_to_index[this->name[name_index]]] = package_tree_populate(root->lookup[char_to_index[this->name[name_index]]], this, name_index + 1);
		root->list = NULL, root->list_size = 0;
	}
	return root;
}

#define	IS_KEY(token, key)	(!strncmp(token, key, strlen(key)))
#define	COPY_VALUE(token, key)	(strdup(token + strlen(key)))

struct DPackageNode *build_database() {
	char *db, *tok, *save;
	struct DPackageNode *root = NULL;
	struct DPackage *this = NULL;

	if (!(db = load_database()))
		return NULL;
	for (save = db, tok = strsep(&save, "\n"); tok; tok = strsep(&save, "\n")) {
		if (!*tok) {
			root = package_tree_populate(root, this, 0), this = NULL;
			continue;
		}
			
		if (!this)
			this = calloc(sizeof(*this), 1);
		if (IS_KEY(tok, "Package: "))
			this->name = COPY_VALUE(tok, "Package: ");
		else if (IS_KEY(tok, "Architecture: "))
			this->arch = COPY_VALUE(tok, "Architecture: ");
		else if (IS_KEY(tok, "Version: "))
			this->version = COPY_VALUE(tok, "Version: ");
	}

	if (this)
		root = package_tree_populate(root, this, 0), this = NULL;

	free(db);
	return root;
}

void free_list(struct DPackage *list) {
	struct DPackage *next;
	if (!list)
		return;
	next = list->next;
	free(list->name), free(list->arch), free(list->version), free(list);
	free_list(next);
}

void free_node(struct DPackageNode *node) {
	int i;
	if (!node)
		return;
	free_list(node->match);
	if (node->lookup)
		for (i = 0; i < lookup_size; i++)
			free_node(node->lookup[i]);
	free(node->lookup);
	free_list(node->list);
	free(node);
}

int main(int argc, char **argv) {
	int n;
	struct DPackageNode *root;
	init_conv_table();
	root = build_database();
	free_node(root);

	if (argc==3) {
		n = compare_versions(argv[1], argv[2]);
		fprintf(stderr, "%s %c %s\n", argv[1], n > 0 ? '>' : (n < 0 ? '<' : '='), argv[2]);
	}
		
	return 0;
}
