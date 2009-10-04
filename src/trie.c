/*
 * The MIT License
 *
 * Copyright (c) 2009 Ning Shi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>
#include <stdlib.h>

#include "trie.h"

struct trie_node {
	trie_node_t *parent;
	trie_node_t *child;
	trie_node_t *prev;
	trie_node_t *next;
	char *delim;
	size_t len;					/* Length of the delimiter or the key */
	char *key;
	void *data;
};

static trie_node_t *new_node(const char *token, size_t len)
{
	trie_node_t *node = NULL;
	char *key = NULL;

	if (token && len > 0) {
		key = calloc(1, len + 1);
		if (!key)
			return NULL;
		memcpy(key, token, len);
	}

	node = calloc(1, sizeof(trie_node_t));
	if (node) {
		node->key = key;
		node->len = len;
	}

	return node;
}

/*
 * Find the sibling that contains the given key.
 */
static trie_node_t *find_child(trie_node_t *node, const char *key, size_t len)
{
	trie_node_t *cur = NULL;

	if (!node || !key || len <= 0)
		return NULL;

	cur = node;
	while (cur) {
		if (memcmp(cur->key, key, cur->len > len ? cur->len : len) == 0)
			break;
		cur = cur->next;
	}

	return cur;
}

/* If create is 1, create nodes on the way of searching. */
static trie_node_t *find_and_create(trie_node_t *root, const char *key,
                                    int create) {
	trie_node_t *parent = root;
	trie_node_t *cur = NULL;
	const char *start = NULL;
	const char *end = NULL;
	const char *eos = NULL;

	if (!root || !root->delim || !key || *key == '\0')
		return NULL;

	start = key;
	end = strstr(start, root->delim);
	eos = start + strlen(key);
	while (parent && start != eos) {
		if (!end)
			end = eos;

		/* Check if it's the file system root. */
		if (start == end && start == key)
			end += root->len;

		/* Check and create necessary nodes. */
		cur = find_child(parent->child, start, end - start);
		if (!cur && create) {
			cur = new_node(start, end - start);
			if (!cur)
				return NULL;

			cur->next = parent->child;
			cur->parent = parent;
			parent->child = cur;
			if (cur->next)
				cur->next->prev = cur;
		}
		parent = cur;

		if (end == eos)
			break;
		if (start == key && end == (start + root->len))
			end = start;
		do {
			start = end + root->len;
			if (start != eos)
				end = strstr(start, root->delim);
		} while (start == end);
	}

	return cur;
}

static void foreach(trie_node_t *node, trie_func func, void *udata)
{
	for (; node; node = node->next) {
		foreach(node->child, func, udata);
		if (node->data)
			func(node, udata);
	}
}

trie_node_t *trie_new(const char *delim, size_t len)
{
	trie_node_t *root = NULL;

	if (!delim || len <= 0)
		return NULL;

	if (!(root = new_node(NULL, 0)))
		return NULL;

	root->delim = calloc(1, len + 1);
	if (!root->delim) {
		free(root);
		return NULL;
	}
	memcpy(root->delim, delim, len);
	root->len = len;

	return root;
}

void trie_free(trie_node_t *root, trie_free_func func)
{
	trie_node_t *cur = NULL;
	trie_node_t *next = NULL;

	if (!root)
		return;

	cur = root->child;
	while (cur) {
		next = cur->next;
		trie_free(cur, func);
		cur = next;
	}

	if (func && root->data)
		func(root->data);
	free(root->key);
	free(root->delim);
	free(root);
}

int trie_add(trie_node_t *root, const char *key, void *data)
{
	trie_node_t *node = find_and_create(root, key, 1);
	if (!node)
		return -1;

	node->data = data;

	return 0;
}

int trie_delete(trie_node_t *root, const char *key, trie_free_func func)
{
	trie_node_t *node = find_and_create(root, key, 0);
	if (!node)
		return -1;

	if (node->parent && node->parent->child == node)
		node->parent->child = node->next;
	if (node->prev)
		node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;
	trie_free(node, func);

	return 0;
}

trie_node_t *trie_find(trie_node_t *root, const char *key)
{
	return find_and_create(root, key, 0);
}

void trie_foreach(trie_node_t *root, trie_func func, void *udata)
{
	if (!root || !func)
		return;

	foreach(root->child, func, udata);
}

const char *trie_key(const trie_node_t *node)
{
	if (!node)
		return NULL;

	return node->key;
}

void *trie_data(const trie_node_t *node)
{
	if (!node)
		return NULL;

	return node->data;
}

void trie_set_data(trie_node_t *node, void *data)
{
	if (!node)
		return;

	node->data = data;
}

trie_node_t *trie_parent(const trie_node_t *node)
{
	if (!node)
		return NULL;

	return node->parent;
}

trie_node_t *trie_child(const trie_node_t *node)
{
	if (!node)
		return NULL;

	return node->child;
}

trie_node_t *trie_prev(const trie_node_t *node)
{
	if (!node)
		return NULL;

	return node->prev;
}

trie_node_t *trie_next(const trie_node_t *node)
{
	if (!node)
		return NULL;

	return node->next;
}
