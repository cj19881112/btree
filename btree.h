#ifndef _BTREE_H_
#define _BTREE_H_

struct bnode_s {
	int 			leaf;
	int				nkeys;
	void 			*val;
	int 			*key;
	struct bnode_s **child;
};

struct btree_s {
	struct bnode_s *root;
	int t;
	int min_key_nr;
	int max_key_nr;
	int min_child_nr;
	int max_child_nr;
};

void btree_init(struct btree_s *tree, int t);
void btree_insert(struct btree_s *tree, int k);
void btree_remove(struct btree_s *tree, int k);
void btree_destroy(struct btree_s *tree);

#endif

