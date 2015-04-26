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
left_rotate(struct btree_s *tree, struct bnode_s *r, int key_pos)
{

	struct bnode_s *right_node = r->child[RIGHT_CHILD_OF_KEY(key_pos)];
	struct bnode_s *left_node = r->child[LEFT_CHILD_OF_KEY(key_pos)];

	/* 修改左节点 */	
	left_node->nkeys++;
	left_node->key[left_node->nkeys-1] = r->key[key_pos];
	left_node->child[left_node->nkeys] = right_node->child[0];

	/* 修改父节点 */
	r->key[key_pos] = right_node->key[0];

	/* 修改右节点 */
	right_node->nkeys--;
	int i;
	for (i = 0; i < right_node->nkeys; i++) {
		right_node->key[i] = right_node->key[i+1];
	}
	for (i = 0; i < right_node->nkeys+1; i++) {
		right_node->child[i] = right_node->child[i+1];
	}

}

void 
right_rotate(struct btree_s *tree, struct bnode_s *r, int key_pos)
{

	struct bnode_s *right_node = r->child[RIGHT_CHILD_OF_KEY(key_pos)];
	struct bnode_s *left_node = r->child[LEFT_CHILD_OF_KEY(key_pos)];

	/* 修改右节点 */	
	int i;
	for (i = right_node->nkeys-1; i >= 0; i--) {
		right_node->key[i+1] = right_node->key[i];
	}
	right_node->key[0] = r->key[key_pos];

	for (i = right_node->nkeys; i >= 0; i--) {
		right_node->child[i+1] = right_node->child[i];
	}
	right_node->child[0] = left_node->child[left_node->nkeys];
	right_node->nkeys++;

	/* 修改父节点 */
	r->key[key_pos] = left_node->key[left_node->nkeys-1];

	/* 修改左节点 */
	left_node->nkeys--;

}

void 
merge_child(struct btree_s *tree, struct bnode_s *r, int key_pos)
{
	struct bnode_s *left_node = r->child[LEFT_CHILD_OF_KEY(key_pos)];
	struct bnode_s *right_node = r->child[RIGHT_CHILD_OF_KEY(key_pos)];


	/* 整理key */
	int key_beg = left_node->nkeys;
	left_node->key[key_beg] = r->key[key_pos];
	key_beg++;


	int i;
	for (i = 0; i < right_node->nkeys; i++) {
		left_node->key[key_beg+i] = right_node->key[i];
	}	

	/* 整理child */
	int child_beg = left_node->nkeys+1;
	for (i = 0; i < right_node->nkeys+1; i++) {
		left_node->child[child_beg+i] = right_node->child[i];
	}

	left_node->nkeys += 1 + right_node->nkeys;
	/* 整理父节点key */
	for (i = key_pos; i < r->nkeys-1; i++) { 
		r->key[i] = r->key[i+1];
	}	
	/* 整理父节点child */
	for (i = RIGHT_CHILD_OF_KEY(key_pos); i < r->nkeys; i++) {
		r->child[i] = r->child[i+1];
	}
	r->nkeys--;
}

static int
btree_delete_max(struct btree_s *tree, struct bnode_s *r)
{
	if (r->leaf) {
		int max = r->key[r->nkeys-1];
		r->nkeys--;
		return max;
	} else {
		struct bnode_s *right_most_child = r->child[r->nkeys];
		if (right_most_child->nkeys >= tree->min_key_nr+1) {
			return btree_delete_max(tree, right_most_child);
		} else {
			struct bnode_s *left = r->child[r->nkeys-1];
			if (left->nkeys >= tree->min_key_nr+1) {
				right_rotate(tree, r, r->nkeys-1);
			} else {
				merge_child(tree, r, r->nkeys-1);
			}
			return btree_delete_max(tree, r->child[r->nkeys]);
		}
	}
}

static int 
btree_delete_min(struct btree_s *tree, struct bnode_s *r)
{
	if (r->leaf) {
		int min = r->key[0], i;
		for (i = 1; i <= r->nkeys-1; i++) {
			r->key[i-1] = r->key[i];
		}
		r->nkeys--;
		return min;
	} else {
		struct bnode_s *left_most_chlid = r->child[0];
		if (left_most_chlid->nkeys >= tree->min_key_nr+1) {
			return btree_delete_min(tree, left_most_chlid);
		} else {
			struct bnode_s *right = r->child[1];
			if (right->nkeys >= tree->min_key_nr+1) {
				left_rotate(tree, r, 0);
			} else {
				merge_child(tree, r, 0);
			}
			return btree_delete_min(tree, r->child[0]);
		}

	}
}

void 
btree_remove_impl(struct btree_s *tree, struct bnode_s *r, int k)
{
	int i = r->nkeys - 1;
	while (i >= 0 && k < r->key[i])
		i--;

	if (i >= 0 && k == r->key[i]) {
		/* 要删除的节点在本节点 */
		if (r->leaf) {
			/* 叶子节点，直接删除 */
			for (; i < r->nkeys; i++) {
				r->key[i] = r->key[i+1];
			}
			r->nkeys--;
		} else {
			/* 非叶子节点 */
			assert(i >= 0 && "error");
			int left = LEFT_CHILD_OF_KEY(i);
			int right = RIGHT_CHILD_OF_KEY(i);
			if (r->child[left]->nkeys >= tree->min_key_nr+1) {
				/* 左边的孩子的key足够t个 */
				r->key[i] = btree_delete_max(tree, r->child[left]);
			} else if (r->child[right]->nkeys >= tree->min_key_nr+1) {
				/* 右边的孩子的key足够t个 */
				r->key[i] = btree_delete_min(tree, r->child[right]);
			} else {
				/* 都不够，需要合并 */
				merge_child(tree, r, i);
				if (r->nkeys == 0) { //?????
					/* FIXME : free node */
					tree->root = r->child[0];
					btree_remove_impl(tree, tree->root, k);
				} else {
					btree_remove_impl(tree, r->child[i], k);
				}
			}
		}
	} else {
		if (i >= 0 && r->leaf) {
			printf("%d, %d, \n", i, k);
			void btree_dump(struct btree_s *);
			btree_dump(tree);
			assert(0 && "not in tree");
		}
		/* 要删除的节点不在本节点 */
		int remove_child_pos = RIGHT_CHILD_OF_KEY(i);
		if (r->child[remove_child_pos]->nkeys >= tree->min_key_nr+1) {
			/* 子节点的key个数足够t */
			btree_remove_impl(tree, r->child[remove_child_pos], k);
		} else {
			/* key个数不够 */
			if (remove_child_pos-1 >=0 && 
					r->child[remove_child_pos-1]->nkeys >= tree->min_child_nr+1) {
				/* 有左兄弟且左兄弟够 */
				right_rotate(tree, r, LEFT_KEY_OF_CHILD(remove_child_pos));
				btree_remove_impl(tree, r->child[remove_child_pos], k);
			} else if (remove_child_pos+1 <= r->nkeys &&  
					r->child[remove_child_pos+1]->nkeys >= tree->min_child_nr+1) {
				/* 有右兄弟且有兄弟够 */
				left_rotate(tree, r, RIGHT_KEY_OF_CHILD(remove_child_pos));
				btree_remove_impl(tree, r->child[remove_child_pos], k);
			} else {
				/* 都不够 */
				int merge_pos = -1; 
				if (remove_child_pos-1 >= 0) {
					merge_pos = LEFT_KEY_OF_CHILD(remove_child_pos);
				} else if (remove_child_pos+1 <= r->nkeys) {
					merge_pos = RIGHT_KEY_OF_CHILD(remove_child_pos);
				} else {
					printf("%d %d %d [%d]\n", remove_child_pos-1, remove_child_pos+1, r->nkeys, k);
					assert(0 && "damn");
				}
				merge_child(tree, r, merge_pos);
				if (r->nkeys == 0) {
					/* FIXME : free node */
					tree->root = r->child[0];
					btree_remove_impl(tree, tree->root, k);
				} else {
					btree_remove_impl(tree, r->child[merge_pos], k);
				}
			}
		}
	}
}

void
btree_remove(struct btree_s *tree, int k)
{
	btree_remove_impl(tree, tree->root, k);
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

static void 
btree_check_key(struct bnode_s *n, int key_pos)
{
	assert(n);

	struct bnode_s *left, *right;	
	left = n->child[LEFT_CHILD_OF_KEY(key_pos)];
	right = n->child[RIGHT_CHILD_OF_KEY(key_pos)];

	assert(left && right);

	int i;
	for (i = 0; i <= left->nkeys-2; i++) {
		assert(left->key[i] < left->key[i+1]);
	}

	assert(left->key[left->nkeys-1] < n->key[key_pos]);

	assert(n->key[key_pos] < right->key[0]);

	for (i = 0; i < right->nkeys-2; i++) {
		assert(right->key[i] < right->key[i+1]);
	}
}

static void 
btree_check(struct bnode_s *r)
{
	if (!r->leaf) {
		int i = 0;
		for (i = 0; i < r->nkeys; i++) {
			btree_check_key(r, i);
		}

		for (i = 0; i < r->nkeys+1; i++) {
			btree_check(r->child[i]);
		}
	}
}

#include <gtest/gtest.h>
TEST(BTree, Insert)
{
	struct btree_s tree;
	btree_init(&tree, 3);
	int i;

	for (i = 0; i < 10; i++) {
		btree_insert(&tree, i);
	}

	btree_dump(&tree);
}

TEST(BTree, Remove)
{
	struct btree_s tree;
	btree_init(&tree, 3);
	int i;

	for (i = 0; i < 10; i++) {
		btree_insert(&tree, i);
	}

	btree_dump(&tree);

	btree_remove(&tree, 5);
	btree_dump(&tree);

	btree_remove(&tree, 9);
	btree_dump(&tree);

	btree_remove(&tree, 8);
	btree_dump(&tree);
}

TEST(BTree, RemoveALot)
{
	struct btree_s tree;
	btree_init(&tree, 3);
	int i;

	for (i = 0; i < 1000000; i++) {
		btree_insert(&tree, i);
	}

	for (i = 10; i < 999992; i++) {
		btree_remove(&tree, i);
	}

	btree_dump(&tree);
}

TEST(BTree, RemoveALot1)
{
	struct btree_s tree;
	btree_init(&tree, 3);
	int i;

	srand(time(0));

	for (i = 0; i < 100000; i++) {
		btree_insert(&tree, i);
	}

	btree_check(tree.root);

	for (i = 0; i < 99992; i += (rand() % 3 + 1) ) {
		btree_remove(&tree, i);
	}

	btree_check(tree.root);
}

#endif




