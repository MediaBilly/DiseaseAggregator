/* 
	The implementation of the Avl Tree data structure.
  I did not implement the function to delete a specific node because it is not used in this program.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../headers/avltree.h"
#include "../headers/utils.h"
#include "../headers/patientRecord.h"

#define MAX(A,B) (((A) > (B)) ? (A) : (B))
#define ABS(X) (((X) < 0) ? (-(X)) : (X))

typedef struct treenode *TreeNode;

struct treenode {
	TreeNode left,right,parent;
	patientRecord data;
	int lHeight,rHeight,height,diff;
};

struct avltree {
	TreeNode root;
	int count;
};

int AvlTree_Create(AvlTree *tree) {
	if((*tree = (AvlTree)malloc(sizeof(struct avltree))) == NULL) {
    not_enough_memory();
    return FALSE;
  }
	(*tree)->root = NULL;
	(*tree)->count = 0;
	return TRUE;
}

void RotateLeft(TreeNode *root) {
	TreeNode tmp = *root,parent = (*root)->parent;
	*root = (*root)->right;
	tmp->right = (*root)->left;
	(*root)->left = tmp;

	(*root)->left->rHeight = (*root)->left->right == NULL ? 0 : (*root)->left->right->height;
	(*root)->left->height = MAX((*root)->left->lHeight,(*root)->left->rHeight) + 1;
	(*root)->left->diff = (*root)->left->rHeight - (*root)->left->lHeight;
	(*root)->left->parent = *root;

	(*root)->lHeight = (*root)->left == NULL ? 0 : (*root)->left->height;
	(*root)->height = MAX((*root)->lHeight,(*root)->rHeight) + 1;
	(*root)->diff = (*root)->rHeight - (*root)->lHeight;
	(*root)->parent = parent;
}

void RotateRight(TreeNode *root) {
	TreeNode tmp = *root,parent = (*root)->parent;
	*root = (*root)->left;
	tmp->left = (*root)->right;
	(*root)->right = tmp;

	(*root)->right->lHeight = (*root)->right->left == NULL ? 0 : (*root)->right->left->height;
	(*root)->right->height = MAX((*root)->right->lHeight,(*root)->right->rHeight) + 1;
	(*root)->right->diff = (*root)->right->rHeight - (*root)->right->lHeight;
	(*root)->right->parent = *root;

	(*root)->rHeight = (*root)->right == NULL ? 0 : (*root)->right->height;
	(*root)->height = MAX((*root)->lHeight,(*root)->rHeight) + 1;
	(*root)->diff = (*root)->rHeight - (*root)->lHeight;
	(*root)->parent = parent;
}

void Rebalance(TreeNode *dest) {
  //Balance factor violation
	if(ABS((*dest)->diff) > 1) {
    //RR
		if((*dest)->diff > 0 && (*dest)->right->diff > 0) {
			RotateLeft(dest);
		}
    //RL
		else if((*dest)->diff > 0 && (*dest)->right->diff < 0) {
			RotateRight(&((*dest)->right));
			RotateLeft(dest);
		}
    //LL
		else if((*dest)->diff < 0 && (*dest)->left->diff < 0) {
			RotateRight(dest);
		}
    //LR
		else if((*dest)->diff < 0 && (*dest)->left->diff > 0) {
			RotateLeft(&((*dest)->left));
			RotateRight(dest);
		}
	}
}

int Insert(TreeNode *dest,TreeNode node,TreeNode parent) {
	int ret;
	if(*dest != NULL) {
    //Insert at left subtree
		if(difftime(PatientRecord_Get_entryDate(node->data),PatientRecord_Get_entryDate((*dest)->data)) < 0) {
			ret = Insert(&((*dest)->left),node,*dest);
      //Set balance factors only if inserted successfully
			if(ret != 0) {
				(*dest)->lHeight = (*dest)->left == NULL ? 0 : (*dest)->left->height;
				(*dest)->height = MAX((*dest)->lHeight,(*dest)->rHeight) + 1;
				(*dest)->diff = (*dest)->rHeight - (*dest)->lHeight;
				Rebalance(dest);
			}
			return ret;
		}
    //Insert at right subtree
		else {
			ret = Insert(&((*dest)->right),node,*dest);
      //Set balance factors only if inserted successfully
			if(ret != 0) {
				(*dest)->rHeight = (*dest)->right == NULL ? 0 : (*dest)->right->height;
				(*dest)->height = MAX((*dest)->lHeight,(*dest)->rHeight) + 1;
				(*dest)->diff = (*dest)->rHeight - (*dest)->lHeight;
				Rebalance(dest);
			}
			return ret;
		}
	}
	else {
		*dest = node;//Successfull insertion
		(*dest)->parent = parent;
		return TRUE;
	}
}

int AvlTree_Insert(AvlTree tree,patientRecord data) {
	TreeNode tmp;
  if((tmp = (TreeNode)malloc(sizeof(struct treenode))) == NULL) {
    not_enough_memory();
    return FALSE;
  }
	tmp->left = tmp->right = tmp->parent = NULL;
	tmp->lHeight = tmp->rHeight = tmp->diff = 0;
	tmp->height = 1;
  tmp->data = data;
	Insert(&(tree->root),tmp,NULL);
	tree->count++;
	return TRUE;
}

unsigned int AvlTree_NumRecords(AvlTree tree) {
	return tree == NULL ? 0 : tree->count;
}

unsigned int RangeQuery(TreeNode root,time_t date1,time_t date2) {
	unsigned int total = 0;
	if (root != NULL) {
		if (difftime(date1,PatientRecord_Get_entryDate(root->data)) < 0) {
			total += RangeQuery(root->left,date1,date2);
		}
		if (difftime(PatientRecord_Get_entryDate(root->data),date1) >= 0 && difftime(PatientRecord_Get_entryDate(root->data),date2) <= 0) {
			total += 1;
		}
		if (difftime(date2,PatientRecord_Get_entryDate(root->data)) >= 0) {
			total += RangeQuery(root->right,date1,date2);
		}
	}
	return total;
}

unsigned int AvlTree_NumRecordsInDateRange(AvlTree tree,time_t date1,time_t date2) {
	return RangeQuery(tree->root,date1,date2);
}

void Destroy(TreeNode node) {
	if(node != NULL) {
		Destroy(node->left);
		Destroy(node->right);
		free(node);
	}
}

int AvlTree_Destroy(AvlTree *tree) {
	if(*tree == NULL) //Not initialized
		return FALSE;
	Destroy((*tree)->root);
	free(*tree);
	*tree = NULL;
	return TRUE;
}
