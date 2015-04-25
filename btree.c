#include "btree.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LEFT_KEY_OF_CHILD(i) 	(i-1)
#define RIGHT_KEY_OF_CHILD(i) 	(i)
#define LEFT_CHILD_OF_KEY(i)	(i)
#define RIGHT_CHILD_OF_KEY(i)	(i+1)

static struct bnode_s *
alloc_node(int max_key, int max_child)
{
	struct bnode_s *r = (struct bnode_s *)malloc(sizeof (*r));
	assert(r && "oom");
	memset(r, 0, sizeof (*r));

	r->leaf = 1;

	r->key = (int*)malloc(sizeof (int) * max_key);
	r->child = (struct bnode_s **)malloc(sizeof (void**) * max_child);
	assert(r->key && r->child && "omm");
	memset((void*)r->key, 0, sizeof (int) * max_key);
	memset((void*)r->child, 0, sizeof (void**) * max_child);

	return r;
}

static void	
key_insert_before(struct bnode_s *node, int pos, int key)
{
	int i;
	for (i = node->nkeys; i > pos; i--) {
		node->key[i] = node->key[i-1];
	}
	node->key[pos] = key;
}

static void 
node_insert_before(struct bnode_s *node, int pos, struct bnode_s *nnode)
{
	int i;
	for (i = node->nkeys+1; i > pos; i--) {
		node->child[i] = node->child[i-1];
	}
	node->child[pos] = nnode;
}

static void	
node_insert_after(struct bnode_s *node, int pos, struct bnode_s *nnode)
{
	node_insert_before(node, pos+1, nnode);
}

static void 
split_node(struct btree_s *tree, struct bnode_s *node, int child_pos)
{
	struct bnode_s *nnode = alloc_node(tree->max_key_nr, tree->max_child_nr);
	struct bnode_s *onode = node->child[child_pos];

#define COPY(dst, src, pos, len) \
	do { \
		int i; \
		for (i = 0; i < len; i++) { \
			dst[i] = src[pos+i]; \
		} \
	} while (0)

	/* 拷贝一半的节点到新的节点, 先算出要拷贝的位置与个数 */
	int key_begin_pos = tree->min_key_nr+1;
	int key_copy_nr = tree->min_key_nr;

	int child_begin_pos = LEFT_CHILD_OF_KEY(key_begin_pos);
	int child_copy_nr = key_copy_nr + 1;

	COPY(nnode->key, onode->key, key_begin_pos, key_copy_nr);
	COPY(nnode->child, onode->child, child_begin_pos, child_copy_nr);

	nnode->nkeys = tree->min_key_nr;
	nnode->leaf = onode->leaf;

	/* 向父节点插入新的节点与孩子 */
	int key = onode->key[tree->min_key_nr];
	key_insert_before(node, RIGHT_KEY_OF_CHILD(child_pos), key);
	node_insert_after(node, child_pos, nnode);
	node->nkeys++;

	/* 调整原来的节点 */
	onode->nkeys = tree->min_key_nr;
}

void 
btree_init(struct btree_s *tree, int t)
{
	tree->t = t;
	tree->min_key_nr = t - 1;
	tree->max_key_nr = 2 * t - 1;
	tree->min_child_nr = t;
	tree->max_child_nr = 2 * t;
	tree->root = alloc_node(tree->max_key_nr, tree->max_child_nr);
}

static void 
btree_insert_nonfull(struct btree_s *tree, struct bnode_s *node, int k)
{
	if (node->leaf) {
		int i = node->nkeys-1;
		while (i >= 0 && k < node->key[i]) {
			node->key[i+1] = node->key[i];
			i--;
		}
		node->key[i+1] = k; 
		node->nkeys++;
	} else {
		int i = node->nkeys-1;
		while (i >= 0 && k < node->key[i]) {
			i--;
		}
		int child_pos = RIGHT_CHILD_OF_KEY(i); /* 插入到这个key有边的child的位置 */
		if (node->child[child_pos]->nkeys == tree->max_key_nr) {
			split_node(tree, node, child_pos);
			if (k > node->key[RIGHT_KEY_OF_CHILD(child_pos)]) { /* 新插入的key在这个child的右边 */
				/* 插入到新的孩子节点 */
				child_pos++;
			}
		}
		btree_insert_nonfull(tree, node->child[child_pos], k);
	}
}

void 
btree_insert(struct btree_s *tree, int k)
{
	struct bnode_s *r = tree->root;

	if (r->nkeys == tree->max_key_nr) {
		struct bnode_s *new_root = alloc_node(tree->max_key_nr, tree->max_child_nr);
		new_root->nkeys = 0;
		new_root->child[0] = r;
		new_root->leaf = 0;
		tree->root = new_root;
		split_node(tree, new_root, 0);
	} 

	btree_insert_nonfull(tree, tree->root, k);
}

void
btree_remove(struct btree_s *tree, int k)
{
	
}

void 
btree_destroy(struct btree_s *tree)
{

}

static void 
btree_dump_rec(struct bnode_s *r)
{
	int i = 0;
	printf("(");
	for (i = 0; i < r->nkeys; i++) {
		if (!r->leaf) {
			btree_dump_rec(r->child[i]);
		}
		printf(" %d ", r->key[i]);
	}
	if (!r->leaf) {
		btree_dump_rec(r->child[i]);
	}
	printf(")");
}

void 
btree_dump(struct btree_s *tree)
{
	btree_dump_rec(tree->root);
	puts("");
}

#ifdef _GTEST_
#include <gtest/gtest.h>
TEST(BTree, Insert)
{
	struct btree_s tree;
	btree_init(&tree, 3);
	int i;

	for (i = 0; i < 200; i++) {

		btree_insert(&tree, i);
	}

	btree_dump(&tree);
}

#endif




