#ifndef TREE_WALK_H
#define TREE_WALK_H

#include "cache.h"

struct name_entry {
	struct object_id oid;
	const char *path;
	int pathlen;
	unsigned int mode;
};

struct tree_desc {
	const void *buffer;
	struct name_entry entry;
	unsigned int size;
};

static inline const struct object_id *tree_entry_extract(struct tree_desc *desc, const char **pathp, unsigned short *modep)
{
	*pathp = desc->entry.path;
	*modep = desc->entry.mode;
	return &desc->entry.oid;
}

static inline int tree_entry_len(const struct name_entry *ne)
{
	return ne->pathlen;
}

/*
 * The _gently versions of these functions warn and return false on a
 * corrupt tree entry rather than dying,
 */

#ifndef NO_THE_REPOSITORY_COMPATIBILITY_MACROS
#define update_tree_entry(desc) update_tree_entry_algop(desc, the_hash_algo)
#define init_tree_desc(desc, buf, size) \
	init_tree_desc_algop(desc, buf, size, the_hash_algo)
#endif
void update_tree_entry_algop(struct tree_desc *, const struct git_hash_algo *);
int update_tree_entry_gently(struct tree_desc *);
void init_tree_desc_algop(struct tree_desc *desc, const void *buf, unsigned long size,
						  const struct git_hash_algo *algo);
int init_tree_desc_gently(struct tree_desc *desc, const void *buf, unsigned long size);

/*
 * Helper function that does both tree_entry_extract() and update_tree_entry()
 * and returns true for success
 */
#ifndef NO_THE_REPOSITORY_COMPATIBILITY_MACROS
#define tree_entry(desc, entry) tree_entry_algop(desc, entry, the_hash_algo)
#endif
int tree_entry_algop(struct tree_desc *, struct name_entry *, const struct git_hash_algo *);
int tree_entry_gently(struct tree_desc *, struct name_entry *);

void *fill_tree_descriptor(struct repository *r,
			   struct tree_desc *desc,
			   const struct object_id *oid);

struct traverse_info;
typedef int (*traverse_callback_t)(int n, unsigned long mask, unsigned long dirmask, struct name_entry *entry, struct traverse_info *);
int traverse_trees(struct index_state *istate, int n, struct tree_desc *t, struct traverse_info *info);

enum get_oid_result get_tree_entry_follow_symlinks(struct repository *r, struct object_id *tree_oid, const char *name, struct object_id *result, struct strbuf *result_path, unsigned short *mode);

struct traverse_info {
	const char *traverse_path;
	struct traverse_info *prev;
	const char *name;
	size_t namelen;
	unsigned mode;

	size_t pathlen;
	struct pathspec *pathspec;

	unsigned long df_conflicts;
	traverse_callback_t fn;
	void *data;
	int show_all_errors;
};

int get_tree_entry(struct repository *, const struct object_id *, const char *, struct object_id *, unsigned short *);
char *make_traverse_path(char *path, size_t pathlen, const struct traverse_info *info,
			 const char *name, size_t namelen);
void strbuf_make_traverse_path(struct strbuf *out,
			       const struct traverse_info *info,
			       const char *name, size_t namelen);
void setup_traverse_info(struct traverse_info *info, const char *base);

static inline size_t traverse_path_len(const struct traverse_info *info,
				       size_t namelen)
{
	return st_add(info->pathlen, namelen);
}

/* in general, positive means "kind of interesting" */
enum interesting {
	all_entries_not_interesting = -1, /* no, and no subsequent entries will be either */
	entry_not_interesting = 0,
	entry_interesting = 1,
	all_entries_interesting = 2 /* yes, and all subsequent entries will be */
};

enum interesting tree_entry_interesting(struct index_state *istate,
					const struct name_entry *,
					struct strbuf *, int,
					const struct pathspec *ps);

#endif
