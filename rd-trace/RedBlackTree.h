#ifndef E_REDBLACK_TREE
#define E_REDBLACK_TREE


#include"misc.h"
#include"TemplateStack.H"
#include<math.h>
#include<limits.h>
#include<iostream>
#include<queue>

//  CONVENTIONS:  
//                Function names: Each word in a function name begins with 
//                a capital letter.  An example funcntion name is  
//                CreateRedTree(a,b,c). Furthermore, each function name 
//                should begin with a capital letter to easily distinguish 
//                them from variables. 
//                                                                     
//                Variable names: Each word in a variable name begins with 
//                a capital letter EXCEPT the first letter of the variable 
//                name.  For example, int newLongInt.  Global variables have 
//                names beginning with "g".  An example of a global 
//                variable name is gNewtonsConstant. 


#ifndef MAX_INT
#define MAX_INT INT_MAX // some architechturs define INT_MAX not MAX_INT
#endif

// The RedBlackEntry class is an Abstract Base Class.  This means that no
// instance of the RedBlackEntry class can exist.  Only classes which
// inherit from the RedBlackEntry class can exist.  Furthermore any class
// which inherits from the RedBlackEntry class must define the member
// function GetKey().  The Print() member function does not have to
// be defined because a default definition exists.
//
// The GetKey() function should return an integer key for that entry.
// The key for an entry should never change otherwise bad things might occur.

/*
class RedBlackEntry {
  int key;
public:
  RedBlackEntry();
  RedBlackEntry(int k): key(k){}
  ~RedBlackEntry();
  int GetKey() const {return key;}
  void Print() const;
};
*/

class RedBlackTreeNode {
  friend class RedBlackTree;
public:
  //void Print(RedBlackTreeNode*,
	//     RedBlackTreeNode*) const;
  RedBlackTreeNode();
  //RedBlackTreeNode(RedBlackEntry *);
  //RedBlackEntry * GetEntry() const;
  RedBlackTreeNode* GetCousin() const;
  ~RedBlackTreeNode();
protected:
  //RedBlackEntry * storedEntry;
  //int key;
  unsigned sum;
  //int valid;
  uint8_t red; /* if red=0 then the node is black */
  unsigned left;
  unsigned right;
  unsigned parent;
	unsigned self;
};

class SearchStats {
public:
  static int searches;
  static int search_depth_sum;
};



class RedBlackTree {
public:
  /*  A sentinel is used for root and for nil.  These sentinels are */
  /*  created when RedBlackTreeCreate is caled.  root->left should always */
  /*  point to the node which is the root of the tree.  nil points to a */
  /*  node which should always be black but has aribtrary children and */
  /*  parent and no key or info.  The point of using these sentinels is so */
  /*  that the root and nil nodes do not require special cases in the code */
  RedBlackTreeNode * root;
	RedBlackTreeNode * tail;
	unsigned taili;
  RedBlackTreeNode * nil;

private:
	RedBlackTreeNode * nodes;
	unsigned n_nodes;
	unsigned cap;
	std::queue<unsigned> freelist;

public:
  RedBlackTree();
  ~RedBlackTree();
  //void Print() const;
  void DeleteNode(unsigned zi);
  //RedBlackTreeNode * Insert(RedBlackEntry *);
  RedBlackTreeNode * InsertTail(unsigned);
  RedBlackTreeNode * GetPredecessorOf(RedBlackTreeNode *) const;
  RedBlackTreeNode * GetSuccessorOf(RedBlackTreeNode *) const;
  RedBlackTreeNode * Search(int key);
  std::pair<int,RedBlackTreeNode*> SearchBottomUp(RedBlackTreeNode *, int)const;
	unsigned GetDistance(unsigned)const;
  std::pair<int,RedBlackTreeNode*> SearchTopDown(RedBlackTreeNode *, int,int)const;
  TemplateStack<RedBlackTreeNode *> * Enumerate(int low, int high) ;
  void CheckAssumptions() const;
  unsigned AddNode();
  void reassign_sum(RedBlackTreeNode *);
  //void TreePrintHelper(RedBlackTreeNode *) const;
protected:
  void LeftRotate(RedBlackTreeNode *);
  void RightRotate(RedBlackTreeNode *);
  //void TreeInsertHelp(RedBlackTreeNode *);
  void FixUpMaxHigh(RedBlackTreeNode *);
  void DeleteFixUp(RedBlackTreeNode *);
  void FixupSums(RedBlackTreeNode *);
};

#endif
