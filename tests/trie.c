#include <stdio.h>

#include "trie.h"

void traverse(const trie_node_t *node) {
	const trie_node_t *cur = node;

	while (cur) {
		printf(" %s", trie_key(cur) ? trie_key(cur) : "ROOT");
		if (trie_child(cur)) {
			printf(" (");
			traverse(trie_child(cur));
			printf(")");
		}
		cur = trie_next(cur);
	}
}

void my_traverse(trie_node_t *node, void *data __attribute__((__unused__))) {
	printf("%s", trie_key(node) ? trie_key(node) : "ROOT");
	printf(" -> ");
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
		trie_free(root, NULL);
		return 1;
	}

	if (!trie_add(root, "", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "EMPTY");
		trie_free(root, NULL);
		return 1;
	}

	if (trie_add(root, "/////", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "/////");
		trie_free(root, NULL);
		return 1;
	}

	if (trie_add(root, "/this/is/very/deep////", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "/this/is/very/deep");
		trie_free(root, NULL);
		return 1;
	}

	if (trie_add(root, "/this/is////very/shallow", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "/this/is/very/shallow");
		trie_free(root, NULL);
		return 1;
	}

	if (trie_add(root, "relative", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "relative");
		trie_free(root, NULL);
		return 1;
	}

	if (trie_add(root, "relative/path", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "relative/path");
		trie_free(root, NULL);
		return 1;
	}

	if (trie_add(root, "relative/last//", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "relative/last//");
		trie_free(root, NULL);
		return 1;
	}

	if (trie_add(root, "relative//name", NULL)) {
		printf("Failed to add \"%s\" to trie.\n", "relative//name");
		trie_free(root, NULL);
		return 1;
	}

	traverse(root);
	printf("\n");
	trie_foreach(root, my_traverse, NULL);
	printf("\n\n");

	/* Deletions */
	if (trie_delete(root, "/this/is/very/shallow", NULL)) {
		printf("Failed to delete \"%s\".\n", "/this/is/very/shallow");
		trie_free(root, NULL);
		return 1;
	}

	if (!trie_delete(root, "/this/is/very/narrow/haha", NULL)) {
		printf("Failed to delete \"%s\".\n", "/this/is/very/narrow/haha");
		trie_free(root, NULL);
		return 1;
	}

	traverse(root);
	printf("\n");
	trie_foreach(root, my_traverse, NULL);
	printf("\n\n");

	if (trie_delete(root, "relative", NULL)) {
		printf("Failed to delete \"%s\".\n", "relative");
		trie_free(root, NULL);
		return 1;
	}

	traverse(root);
	printf("\n");
	trie_foreach(root, my_traverse, NULL);
	printf("\n\n");

	trie_free(root, NULL);

	return 0;
}
