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

/*
 * This is a trie data structure implementation with strings as keys. It is not
 * thread-safe!
 *
 * The data of each node is stored in the "data" field, and the key is in the
 * "key" field.
 */

#ifndef _TRIE_H_
#define _TRIE_H_

typedef struct trie_node trie_node_t;
typedef void (*trie_free_func)(void *data);
typedef void (*trie_func)(trie_node_t *node, void *udata);

trie_node_t *trie_new(const char *delim, size_t len);
void trie_free(trie_node_t *root, trie_free_func func);

/*
 * Adds a node with key to the tree. If the node already exists, it updates the
 * data.
 *
 * 0 is returned on success, otherwise -1 is returned.
 */
int trie_add(trie_node_t *root, const char *key, void *data);

/*
 * Deletes a node with key from the tree.
 *
 * 0 is returned on success, otherwise -1 is returned.
 */
int trie_delete(trie_node_t *root, const char *key, trie_free_func func);
trie_node_t *trie_find(trie_node_t *root, const char *key);
/* Applies func to each node. Traverses the tree in depth-first pattern. */
void trie_foreach(trie_node_t *root, trie_func func, void *udata);

inline const char *trie_key(const trie_node_t *node);
inline void *trie_data(const trie_node_t *node);
inline void trie_set_data(trie_node_t *node, void *data);
inline trie_node_t *trie_parent(const trie_node_t *node);
inline trie_node_t *trie_child(const trie_node_t *node);
inline trie_node_t *trie_prev(const trie_node_t *node);
inline trie_node_t *trie_next(const trie_node_t *node);

#endif
