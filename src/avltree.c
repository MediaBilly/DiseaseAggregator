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
#include "../headers/treestats.h"

#define MAX(A,B) (((A) > (B)) ? (A) : (B))
#define ABS(X) (((X) < 0) ? (-(X)) : (X))

char topkCategories[4][6] = {
	"0-20",
	"21-40",
	"41-60",
	"60+"
};

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

unsigned int RangeQuery(TreeNode root,time_t date1,time_t date2,boolean exited) {
	unsigned int total = 0;
	if (root != NULL) {
		if (!exited) {
			if (difftime(date1,PatientRecord_Get_entryDate(root->data)) < 0) {
				total += RangeQuery(root->left,date1,date2,exited);
			}
			if (difftime(PatientRecord_Get_entryDate(root->data),date1) >= 0 && difftime(PatientRecord_Get_entryDate(root->data),date2) <= 0) {
				total += 1;
			}
			if (difftime(date2,PatientRecord_Get_entryDate(root->data)) >= 0) {
				total += RangeQuery(root->right,date1,date2,exited);
			}
		} else {
			if (PatientRecord_Exited(root->data) && difftime(PatientRecord_Get_exitDate(root->data),date1) >= 0 && difftime(PatientRecord_Get_exitDate(root->data),date2) <= 0) {
				total += 1;
			}
			total += RangeQuery(root->left,date1,date2,exited) + RangeQuery(root->right,date1,date2,exited);
		}
	}
	return total;
}

unsigned int AvlTree_NumRecordsInDateRange(AvlTree tree,time_t date1,time_t date2,boolean exited) {
	return RangeQuery(tree->root,date1,date2,exited);
}

filestat addStats(filestat stats1,filestat stats2) {
	filestat ret;
	memset(&ret,0,sizeof(filestat));
	ret.years0to20 = stats1.years0to20 + stats2.years0to20;
	ret.years21to40 = stats1.years21to40 + stats2.years21to40;
	ret.years41to60 = stats1.years41to60 + stats2.years41to60;
	ret.years60plus = stats1.years60plus + stats2.years60plus;
	return ret;
}

unsigned int sumStats(filestat st) {
	return st.years0to20 + st.years21to40 + st.years41to60 + st.years60plus;
}

// Returns amounts of cases in given date range grouped by age category
filestat StatsQuery(TreeNode root,time_t date1,time_t date2) {
	filestat total;
	memset(&total,0,sizeof(filestat));
	if (root != NULL) {
		if (difftime(date1,PatientRecord_Get_entryDate(root->data)) < 0) {
			total = addStats(total,StatsQuery(root->left,date1,date2));
		}
		if (difftime(PatientRecord_Get_entryDate(root->data),date1) >= 0 && difftime(PatientRecord_Get_entryDate(root->data),date2) <= 0) {
			int age = PatientRecord_Get_age(root->data);
			if (0 <= age && age <= 20) {
				total.years0to20++;
			} else if (21 <= age && age <= 40) {
				total.years21to40++;
			} else if (41 <= age && age <= 60) {
				total.years41to60++;
			} else {
				total.years60plus++;
			}
		}
		if (difftime(date2,PatientRecord_Get_entryDate(root->data)) >= 0) {
			total = addStats(total,StatsQuery(root->right,date1,date2));
		}
	}
	return total;
}

void AvlTree_topk_Age_Ranges(AvlTree tree,time_t date1,time_t date2,unsigned int k,int writefd,unsigned int bufferSize) {
	filestat stats = StatsQuery(tree->root,date1,date2);
	unsigned int total = sumStats(stats);
	if (k > 4) {
		k = 4;
	}
	int casesPerAgeCategory[4],category[4];
	casesPerAgeCategory[0] = stats.years0to20;
	category[0] = 0;
	casesPerAgeCategory[1] = stats.years21to40;
	category[1] = 1;
	casesPerAgeCategory[2] = stats.years41to60;
	category[2] = 2;
	casesPerAgeCategory[3] = stats.years60plus;
	category[3] = 3;
	int i,j,tmp;
	for(i = 4; i >= 0; i--) {
		for(j = 1; j <= i; j++) {
			if (casesPerAgeCategory[j] > casesPerAgeCategory[j - 1]) {
				tmp = casesPerAgeCategory[j];
				casesPerAgeCategory[j] = casesPerAgeCategory[j - 1];
				casesPerAgeCategory[j - 1] = tmp;
			}
		}
	}
	for (i = 0;i < k;i++) {
		char toSend[15];
		sprintf(toSend,"%s: %.2f%%\n",topkCategories[category[i]],((((double)casesPerAgeCategory[i])/total)*100));
		send_data(writefd,toSend,strlen(toSend),bufferSize);
	}
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
