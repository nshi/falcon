#include <stdio.h>

#include "trie.h"

void traverse(const trie_node_t *root) {
	const trie_node_t *cur = root;

	while (cur) {
		printf(" %s", trie_key(cur) ? trie_key(cur) : "ROOT");
		if (trie_child(cur)) {
			printf(" has child: (");
			traverse(trie_child(cur));
			printf(")");
		}
		cur = trie_next(cur);
	}
}

int main(int argc __attribute__((__unused__)),
         char **argv __attribute__((__unused__))) {
	trie_node_t *root = trie_new("/", 1);

	if (!root) {
		printf("Failed to create trie.\n");
		return 1;
	}

	if (trie_add(root, "/", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "/");
		trie_free(root);
		return 1;
	}

	if (!trie_add(root, "", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "EMPTY");
		trie_free(root);
		return 1;
	}

	if (trie_add(root, "/////", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "/////");
		trie_free(root);
		return 1;
	}

	if (trie_add(root, "/this/is/very/deep////", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "/this/is/very/deep");
		trie_free(root);
		return 1;
	}

	if (trie_add(root, "/this/is////very/shallow", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "/this/is/very/shallow");
		trie_free(root);
		return 1;
	}

	if (trie_add(root, "relative", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "relative");
		trie_free(root);
		return 1;
	}

	if (trie_add(root, "relative/path", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "relative/path");
		trie_free(root);
		return 1;
	}

	if (trie_add(root, "relative/last//", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "relative/last//");
		trie_free(root);
		return 1;
	}

	if (trie_add(root, "relative//name", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "relative//name");
		trie_free(root);
		return 1;
	}

	traverse(root);
	printf("\n");

	/* Deletions */
	if (trie_delete(root, "/this/is/very/shallow")) {
		printf("Failed to delete \"%s\".\n", "/this/is/very/shallow");
		trie_free(root);
		return 1;
	}

	if (!trie_delete(root, "/this/is/very/narrow/haha")) {
		printf("Failed to delete \"%s\".\n", "/this/is/very/narrow/haha");
		trie_free(root);
		return 1;
	}

	traverse(root);
	printf("\n");

	if (trie_delete(root, "relative")) {
		printf("Failed to delete \"%s\".\n", "relative");
		trie_free(root);
		return 1;
	}

	traverse(root);
	printf("\n");

	trie_free(root);

	return 0;
}
