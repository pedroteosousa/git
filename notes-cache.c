#include "cache.h"
#include "notes-cache.h"
#include "object-store.h"
#include "repository.h"
#include "commit.h"
#include "refs.h"

static int notes_cache_match_validity(struct repository *r,
				      const char *ref,
				      const char *validity)
{
	struct object_id oid;
	struct commit *commit;
	struct pretty_print_context pretty_ctx;
	struct strbuf msg = STRBUF_INIT;
	int ret;

	if (repo_read_ref(r, ref, &oid) < 0)
		return 0;

	commit = lookup_commit_reference_gently(r, &oid, 1);
	if (!commit)
		return 0;

	memset(&pretty_ctx, 0, sizeof(pretty_ctx));
	format_commit_message(commit, "%s", &msg, &pretty_ctx);
	strbuf_trim(&msg);

	ret = !strcmp(msg.buf, validity);
	strbuf_release(&msg);

	return ret;
}

void notes_cache_init(struct repository *r, struct notes_cache *c,
		      const char *name, const char *validity)
{
	struct strbuf ref = STRBUF_INIT;
	int flags = NOTES_INIT_WRITABLE;

	memset(c, 0, sizeof(*c));
	c->validity = xstrdup(validity);

	strbuf_addf(&ref, "refs/notes/%s", name);
	if (!notes_cache_match_validity(r, ref.buf, validity))
		flags |= NOTES_INIT_EMPTY;
	repo_init_notes(r, &c->tree, ref.buf, combine_notes_overwrite, flags);
	strbuf_release(&ref);
}

int notes_cache_write(struct repository *r, struct notes_cache *c)
{
	struct object_id tree_oid, commit_oid;

	if (!c || !c->tree.initialized || !c->tree.update_ref ||
	    !*c->tree.update_ref)
		return -1;
	if (!c->tree.dirty)
		return 0;

	if (write_notes_tree(r, &c->tree, &tree_oid))
		return -1;
	if (repo_commit_tree(r, c->validity, strlen(c->validity), &tree_oid, NULL,
			&commit_oid, NULL, NULL) < 0)
		return -1;
	if (repo_update_ref(r, "update notes cache", c->tree.update_ref, &commit_oid,
			NULL, 0, UPDATE_REFS_QUIET_ON_ERR) < 0)
		return -1;

	return 0;
}

char *notes_cache_get(struct repository *r, struct notes_cache *c,
		struct object_id *key_oid, size_t *outsize)
{
	const struct object_id *value_oid;
	enum object_type type;
	char *value;
	unsigned long size;

	value_oid = repo_get_note(r, &c->tree, key_oid);
	if (!value_oid)
		return NULL;
	value = repo_read_object_file(r, value_oid, &type, &size);

	*outsize = size;
	return value;
}

int notes_cache_put(struct repository *r, struct notes_cache *c,
		struct object_id *key_oid, const char *data, size_t size)
{
	struct object_id value_oid;

	if (repo_write_object_file(r, data, size, "blob", &value_oid) < 0)
		return -1;
	return repo_add_note(r, &c->tree, key_oid, &value_oid, NULL);
}
