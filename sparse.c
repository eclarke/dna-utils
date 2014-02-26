#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct bin_tree {
	size_t index;
	unsigned long long value;
	struct bin_tree *right;
	struct bin_tree *left;
};

typedef struct bin_tree node;

void insert(node ** tree, size_t index) {
	node *temp = NULL;
	if(!(*tree)) {
		temp = (node *)malloc(sizeof(node));
		temp->left = temp->right = NULL;
		temp->index = index;
		temp->value = 1;
		*tree = temp;
		return;
	}

	if(index < (*tree)->index)
		insert(&(*tree)->left, index);
	else if(index > (*tree)->index)
		insert(&(*tree)->right, index);

}

void deltree(node * tree) {
	if (tree) {
		deltree(tree->left);
		deltree(tree->right);
		free(tree);
	}
}

node* search(node ** tree, size_t index) {
	if(!(*tree))
		return NULL;

	if(index < (*tree)->index)
		search(&((*tree)->left), index);

	else if(index > (*tree)->index)
		search(&((*tree)->right), index);

	else if(index == (*tree)->index)
		return *tree;
}

unsigned long long *lookup(node **tree, size_t index) {
	node *ret = search(tree, index);
	if(ret)
		return ret->value;
	else
		return 0;
}
