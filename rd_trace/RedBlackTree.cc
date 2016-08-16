#include "RedBlackTree.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

// If the symbol CHECK_RB_TREE_ASSUMPTIONS is defined then the
// code does a lot of extra checking to make sure certain assumptions
// are satisfied.  This only needs to be done if you suspect bugs are
// present or if you make significant changes and want to make sure
// your changes didn't mess anything up.
// #define CHECK_RB_TREE_ASSUMPTIONS 1


int SearchStats::searches = 0;
int SearchStats::search_depth_sum = 0;
const int MIN_INT=-MAX_INT;

/*
RedBlackTreeNode * RedBlackTreeNode::GetCousin() const {
  return (self == parent->right)?( &nodes[parent->left]) : (&nodes[parent->right]);
}
*/

void RedBlackTree::reassign_sum(RedBlackTreeNode * x){
  x->sum = nodes[x->left].sum + nodes[x->right].sum + 1;
}

RedBlackTreeNode::RedBlackTreeNode(){
}

/*
RedBlackTreeNode::RedBlackTreeNode(RedBlackEntry * newEntry)
  : storedEntry (newEntry) , key(newEntry->GetKey()) {
}
*/

RedBlackTreeNode::~RedBlackTreeNode(){
	//if(storedEntry!=NULL) delete storedEntry;
}

//RedBlackEntry * RedBlackTreeNode::GetEntry() const {return storedEntry;}

/*
RedBlackEntry::RedBlackEntry(){
}
RedBlackEntry::~RedBlackEntry(){
}


void RedBlackEntry::Print() const {
  std::cout << "key: " << GetKey() << std::endl;
}
*/

RedBlackTree::RedBlackTree()
{
	cap = 32;
	n_nodes = 2;
	nodes = (RedBlackTreeNode *) malloc(cap * sizeof(RedBlackTreeNode));
  nil = &nodes[0];
	nil->self = 0;
  nil->left = nil->right = nil->parent = nil->self;
  nil->red = 0;
  //nil->key = MIN_INT;
  //nil->storedEntry = NULL;
  nil->sum = 0;
  //nil->valid = 0;
  
  root = &nodes[1];
	root->self = 1;
  root->parent = root->left = root->right = nil->self;
  //root->key = MAX_INT;
  root->red=0;
  //root->storedEntry = NULL;  
  root->sum=0;
  //root->valid=0;

	tail = nil;
	taili = 0;
}

/***********************************************************************/
/*  FUNCTION:  LeftRotate */
/**/
/*  INPUTS:  the node to rotate on */
/**/
/*  OUTPUT:  None */
/**/
/*  Modifies Input: this, x */
/**/
/*  EFFECTS:  Rotates as described in _Introduction_To_Algorithms by */
/*            Cormen, Leiserson, Rivest (Chapter 14).  Basically this */
/*            makes the parent of x be to the left of x, x the parent of */
/*            its parent before the rotation and fixes other pointers */
/*            accordingly.  */
/***********************************************************************/

void RedBlackTree::LeftRotate(RedBlackTreeNode * x) {
  /*  I originally wrote this function to use the sentinel for */
  /*  nil to avoid checking for nil.  However this introduces a */
  /*  very subtle bug because sometimes this function modifies */
  /*  the parent pointer of nil.  This can be a problem if a */
  /*  function which calls LeftRotate also uses the nil sentinel */
  /*  and expects the nil sentinel's parent pointer to be unchanged */
  /*  after calling this function.  For example, when DeleteFixUP */
  /*  calls LeftRotate it expects the parent pointer of nil to be */
  /*  unchanged. */

	RedBlackTreeNode * y = &nodes[x->right];
  x->right=y->left;

  if (y->left != nil->self) nodes[y->left].parent=x->self; /* used to use sentinel here */
  /* and do an unconditional assignment instead of testing for nil */
  
  y->parent=x->parent;

  /* instead of checking if x->parent is the root as in the book, we */
  /* count on the root sentinel to implicitly take care of this case */
  if( x->self == nodes[x->parent].left) {
    nodes[x->parent].left=y->self;
  } else {
    nodes[x->parent].right=y->self;
  }
  y->left=x->self;
  x->parent=y->self;

	{ 
		x->sum = nodes[x->left].sum + nodes[x->right].sum + 1;
		y->sum = nodes[y->left].sum + nodes[y->right].sum + 1;
	}

#ifdef CHECK_RB_TREE_ASSUMPTIONS
  CheckAssumptions();
#elif defined(DEBUG_ASSERT)
  Assert(!nil->red,"nil not red in RedBlackTree::LeftRotate");
#endif
}

/***********************************************************************/
/*  FUNCTION:  RighttRotate */
/**/
/*  INPUTS:  node to rotate on */
/**/
/*  OUTPUT:  None */
/**/
/*  Modifies Input?: this, y */
/**/
/*  EFFECTS:  Rotates as described in _Introduction_To_Algorithms by */
/*            Cormen, Leiserson, Rivest (Chapter 14).  Basically this */
/*            makes the parent of x be to the left of x, x the parent of */
/*            its parent before the rotation and fixes other pointers */
/*            accordingly.  */
/***********************************************************************/

void RedBlackTree::RightRotate(RedBlackTreeNode * y) {

  /*  I originally wrote this function to use the sentinel for */
  /*  nil to avoid checking for nil.  However this introduces a */
  /*  very subtle bug because sometimes this function modifies */
  /*  the parent pointer of nil.  This can be a problem if a */
  /*  function which calls LeftRotate also uses the nil sentinel */
  /*  and expects the nil sentinel's parent pointer to be unchanged */
  /*  after calling this function.  For example, when DeleteFixUP */
  /*  calls LeftRotate it expects the parent pointer of nil to be */
  /*  unchanged. */

  RedBlackTreeNode* x = &nodes[y->left];
  y->left=x->right;

  if (nil->self != x->right)  nodes[x->right].parent=y->self; /*used to use sentinel here */
  /* and do an unconditional assignment instead of testing for nil */

  /* instead of checking if x->parent is the root as in the book, we */
  /* count on the root sentinel to implicitly take care of this case */
  x->parent=y->parent;
  if( y->self == nodes[y->parent].left) {
    nodes[y->parent].left=x->self;
  } else {
    nodes[y->parent].right=x->self;
  }
  x->right=y->self;
  y->parent=x->self;

	{
		y->sum = nodes[y->left].sum + nodes[y->right].sum + 1;
		x->sum = nodes[x->left].sum + nodes[x->right].sum + 1;
	}

#ifdef CHECK_RB_TREE_ASSUMPTIONS
  CheckAssumptions();
#elif defined(DEBUG_ASSERT)
  Assert(!nil->red,"nil not red in RedBlackTree::RightRotate");
#endif
}

unsigned RedBlackTree::AddNode(){
	if(!freelist.empty()){
	  unsigned free_ind = freelist.front();
		freelist.pop();
		return free_ind;
	}

  if(cap == n_nodes){
	  cap  = cap << 1;
		nodes = (RedBlackTreeNode *) realloc( nodes, cap * sizeof(RedBlackTreeNode));
		//std::cerr << "cap is now " << cap << "\n";
	}
	RedBlackTreeNode * new_node = &nodes[n_nodes];
	new_node->self = n_nodes;
	n_nodes++;
	nil = &nodes[0];
	root = &nodes[1];
	tail = &nodes[taili];
	//std::cerr << "new node self is " << new_node->self << "\n";
	return new_node->self;
}

RedBlackTreeNode * RedBlackTree::InsertTail(unsigned zi){
	assert(zi!=0 && zi!=1);
	RedBlackTreeNode * z = &nodes[zi];

	z->sum = 1;
	//z->valid = 1;
	z->red=1;
	z->right = nil->self;
	z->left = nil->self;

	if (tail == nil) { 
		root->left = z->self;
		z->parent = root->self;
		z->red = 0;
	} else {
	  RedBlackTreeNode * y = tail;

	  RedBlackTreeNode * x = z;
		y->right=x->self;

		x->parent=y->self;

		while(nodes[x->parent].red) { /* use sentinel instead of checking for root */
			/*cannot happen 
				if (x->parent == x->parent->parent->left) {
				y=x->parent->parent->right;
				if (y->red) {
				x->parent->red=0;
				y->red=0;
				x->parent->parent->red=1;
				x=x->parent->parent;
				} else {
				if (x == x->parent->right) {
				x=x->parent;
				LeftRotate(x);
				}
				x->parent->red=0;
				x->parent->parent->red=1;
				RightRotate(x->parent->parent);
				} 
				} else */
			/* case for x->parent == x->parent->parent->right */
			/* this part is just like the section above with */
			/* left and right interchanged */

			assert(x->parent == nodes[nodes[x->parent].parent].right && "x is not his parent's right child");
			y = &nodes[nodes[nodes[x->parent].parent].left];

			if (y->red) {
			//	std::cerr<< "easy case\n";
				nodes[x->parent].red=0;
				y->red=0;
				nodes[nodes[x->parent].parent].red=1;
				nodes[x->parent].sum++;
				nodes[nodes[x->parent].parent].sum++;
				x = &nodes[nodes[x->parent].parent];
			} else  { 
				//std::cerr<< "left rotation\n";
				/*
					 if (x == x->parent->left) {
					 x=x->parent;
					 RightRotate(x);
					 } */
				assert(x->self == nodes[x->parent].right);
				nodes[x->parent].red=0;
				nodes[nodes[x->parent].parent].red=1;
				LeftRotate(&nodes[nodes[x->parent].parent]);
			}

		}

		nodes[root->left].red=0;

		x = &nodes[x->parent];

		FixupSums(x);

		/*
		while(x!=root && (x->parent!=root)){
			x->parent->sum++;
			x=x->parent;
		}*/
	}

	tail = z;
	taili = z->self;


#if defined(DEBUG_ASSERT)
	Assert(!nil->red,"nil not red in RedBlackTree::TreeInsertHelp");
#endif

	return z;

}

/***********************************************************************/
/*  FUNCTION:  TreeInsertHelp  */
/**/
/*  INPUTS:  z is the node to insert */
/**/
/*  OUTPUT:  none */
/**/
/*  Modifies Input:  this, z */
/**/
/*  EFFECTS:  Inserts z into the tree as if it were a regular binary tree */
/*            using the algorithm described in _Introduction_To_Algorithms_ */
/*            by Cormen et al.  This funciton is only intended to be called */
/*            by the Insert function and not by the user */
/***********************************************************************/

//void RedBlackTree::TreeInsertHelp(RedBlackTreeNode* z) {
//
//  /*  This function should only be called by RedBlackTree::Insert */
//  RedBlackTreeNode* x;
//  RedBlackTreeNode* y;
//    
//  z->left=z->right=nil;
//  y=root;
//  x=root->left;
//  while( x != nil) {
//    y=x;
//    if ( x->key > z->key) { 
//      x=x->left;
//    } else { /* x->key <= z->key */
//      x=x->right;
//    }
//  }
//  z->parent=y;
//  if ( (y == root) ||
//       (y->key > z->key) ) { 
//    y->left=z;
//  } else {
//    y->right=z;
//  }
//
//  FixupSums(y);
//
//#if defined(DEBUG_ASSERT)
//  Assert(!nil->red,"nil not red in RedBlackTree::TreeInsertHelp");
//#endif
//}

/*  Before calling InsertNode  the node x should have its key set */

/***********************************************************************/
/*  FUNCTION:  InsertNode */
/**/
/*  INPUTS:  newEntry is the entry to insert*/
/**/
/*  OUTPUT:  This function returns a pointer to the newly inserted node */
/*           which is guarunteed to be valid until this node is deleted. */
/*           What this means is if another data structure stores this */
/*           pointer then the tree does not need to be searched when this */
/*           is to be deleted. */
/**/
/*  Modifies Input: tree */
/**/
/*  EFFECTS:  Creates a node node which contains the appropriate key and */
/*            info pointers and inserts it into the tree. */
/***********************************************************************/

//RedBlackTreeNode * RedBlackTree::Insert(RedBlackEntry * newEntry)
//{
//  RedBlackTreeNode * y;
//  RedBlackTreeNode * x;
//  RedBlackTreeNode * newNode;
//
//  //x = new RedBlackTreeNode(newEntry);
//	x = new RedBlackTreeNode();
//  x->sum = 1;
//  //x->valid = 1;
//  TreeInsertHelp(x);
//  newNode = x;
//  x->red=1;
//  while(x->parent->red) { /* use sentinel instead of checking for root */
//    if (x->parent == x->parent->parent->left) {
//      y=x->parent->parent->right;
//      if (y->red) {
//	x->parent->red=0;
//	y->red=0;
//	x->parent->parent->red=1;
//	x=x->parent->parent;
//      } else {
//	if (x == x->parent->right) {
//	  x=x->parent;
//	  LeftRotate(x);
//	}
//	x->parent->red=0;
//	x->parent->parent->red=1;
//	RightRotate(x->parent->parent);
//      } 
//    } else { /* case for x->parent == x->parent->parent->right */
//             /* this part is just like the section above with */
//             /* left and right interchanged */
//      y=x->parent->parent->left;
//      if (y->red) {
//	x->parent->red=0;
//	y->red=0;
//	x->parent->parent->red=1;
//	x=x->parent->parent;
//      } else {
//	if (x == x->parent->left) {
//	  x=x->parent;
//	  RightRotate(x);
//	}
//	x->parent->red=0;
//	x->parent->parent->red=1;
//	LeftRotate(x->parent->parent);
//      } 
//    }
//  }
//  root->left->red=0;
//  FixupSums(newNode);
//  return(newNode);
//
//#ifdef CHECK_RB_TREE_ASSUMPTIONS
//  CheckAssumptions();
//#elif defined(DEBUG_ASSERT)
//  Assert(!nil->red,"nil not red in RedBlackTree::Insert");
//  Assert(!root->red,"root not red in RedBlackTree::Insert");
//#endif
//}

/***********************************************************************/
/*  FUNCTION:  GetSuccessorOf  */
/**/
/*    INPUTS:  x is the node we want the succesor of */
/**/
/*    OUTPUT:  This function returns the successor of x or NULL if no */
/*             successor exists. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:  uses the algorithm in _Introduction_To_Algorithms_ */
/***********************************************************************/
  
RedBlackTreeNode * RedBlackTree::GetSuccessorOf(RedBlackTreeNode * x) const
{ 
  RedBlackTreeNode* y;

  if (nil != (y = &nodes[x->right])) { /* assignment to y is intentional */
    while(y->left != nil->self) { /* returns the minium of the right subtree of x */
      y=&nodes[y->left];
    }
    return(y);
  } else {
    y=&nodes[x->parent];
    while(x == &nodes[y->right]) { /* sentinel used instead of checking for nil */
      x=y;
      y=&nodes[y->parent];
    }
    if (y == root) return(nil);
    return(y);
  }
}

/***********************************************************************/
/*  FUNCTION:  GetPredecessorOf  */
/**/
/*    INPUTS:  x is the node to get predecessor of */
/**/
/*    OUTPUT:  This function returns the predecessor of x or NULL if no */
/*             predecessor exists. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:  uses the algorithm in _Introduction_To_Algorithms_ */
/***********************************************************************/

RedBlackTreeNode * RedBlackTree::GetPredecessorOf(RedBlackTreeNode * x) const {
  RedBlackTreeNode* y;

  if (nil != (y = &nodes[x->left])) { /* assignment to y is intentional */
    while(y->right != nil->self) { /* returns the maximum of the left subtree of x */
      y=&nodes[y->right];
    }
    return(y);
  } else {
    y=&nodes[x->parent];
    while(x == &nodes[y->left]) { 
      if (y == root) return(nil); 
      x=y;
      y=&nodes[y->parent];
    }
    return(y);
  }
}

/***********************************************************************/
/*  FUNCTION:  Print */
/**/
/*    INPUTS:  none */
/**/
/*    OUTPUT:  none  */
/**/
/*    EFFECTS:  This function recursively prints the nodes of the tree */
/*              inorder. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:    This function should only be called from ITTreePrint */
/***********************************************************************/

/*
void RedBlackTreeNode::Print(RedBlackTreeNode * nil,
			     RedBlackTreeNode * root) const {
  storedEntry->Print();
  printf(", key=%i, sum=%i ",key, sum);
  printf("  l->key=");
  if( left == nil) printf("NULL"); else printf("%i",left->key);
  printf("  r->key=");
  if( right == nil) printf("NULL"); else printf("%i",right->key);
  printf("  p->key=");
  if( parent == root) printf("NULL"); else printf("%i",parent->key);
  printf("  red=%i\n",red);
}
void RedBlackTree::TreePrintHelper( RedBlackTreeNode* x) const {
  
  if (x != nil) {
    TreePrintHelper(x->left);
    x->Print(nil,root);
    TreePrintHelper(x->right);
  }
}

RedBlackTree::~RedBlackTree() {
  RedBlackTreeNode * x = root->left;
  TemplateStack<RedBlackTreeNode *> stuffToFree;

  if (x != nil) {
    if (x->left != nil) {
      stuffToFree.Push(x->left);
    }
    if (x->right != nil) {
      stuffToFree.Push(x->right);
    }
    // delete x->storedEntry;
    delete x;
		//std::cerr << "deleting x1\n";
    while( stuffToFree.NotEmpty() ) {
      x = stuffToFree.Pop();
      if (x->left != nil) {
	stuffToFree.Push(x->left);
      }
      if (x->right != nil) {
	stuffToFree.Push(x->right);
      }
      // delete x->storedEntry;
      delete x;
		//std::cerr << "deleting x2\n";
    }
  }
  delete nil;
  delete root;
}
*/

RedBlackTree::~RedBlackTree() {
  free(nodes);
}


/***********************************************************************/
/*  FUNCTION:  Print */
/**/
/*    INPUTS:  none */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  This function recursively prints the nodes of the tree */
/*             inorder. */
/**/
/*    Modifies Input: none */
/**/
/***********************************************************************/

/*
void RedBlackTree::Print() const {
  TreePrintHelper(root->left);
}
*/

/***********************************************************************/
/*  FUNCTION:  FixupSums */
/**/
/*    INPUTS:  x is lowest node that needs a fixup */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  Recalculates the sum for all ancestore's and super-cousins of x */
/**/
/*    Modifies Input: this, x */
/**/
/***********************************************************************/
void RedBlackTree::FixupSums(RedBlackTreeNode * x){
  RedBlackTreeNode * w = x;

  while(w!=root){
     reassign_sum(w);
     w = &nodes[w->parent];
  }
}



/***********************************************************************/
/*  FUNCTION:  DeleteFixUp */
/**/
/*    INPUTS:  x is the child of the spliced */
/*             out node in DeleteNode. */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  Performs rotations and changes colors to restore red-black */
/*             properties after a node is deleted */
/**/
/*    Modifies Input: this, x */
/**/
/*    The algorithm from this function is from _Introduction_To_Algorithms_ */
/***********************************************************************/

void RedBlackTree::DeleteFixUp(RedBlackTreeNode* x) {
	RedBlackTreeNode * w;
	RedBlackTreeNode * rootLeft = &nodes[root->left];

	while( (!x->red) && !(x==rootLeft)) {
		if (x->self == nodes[x->parent].left) {
			w= &nodes[nodes[x->parent].right];
			if (w->red) {
				w->red=0;
				nodes[x->parent].red=1;
				LeftRotate(&nodes[x->parent]);
				w= &nodes[nodes[x->parent].right];
			}
			if ( (!nodes[w->right].red) && (!nodes[w->left].red) ) { 
				w->red=1;
				x=&nodes[x->parent];
			} else {
				if (!nodes[w->right].red) {
					nodes[w->left].red=0;
					w->red=1;
					RightRotate(w);
					w=&nodes[nodes[x->parent].right];
				}
				w->red=nodes[x->parent].red;
				nodes[x->parent].red=0;
				nodes[w->right].red=0;
				LeftRotate(&nodes[x->parent]);
				x=rootLeft; /* this is to exit while loop */
			}
		} else { /* the code below is has left and right switched from above */
			w=&nodes[nodes[x->parent].left];
			if (w->red) {
				w->red=0;
				nodes[x->parent].red=1;
				RightRotate(&nodes[x->parent]);
				w=&nodes[nodes[x->parent].left];
			}
			if ( (!nodes[w->right].red) && (!nodes[w->left].red) ) { 
				w->red=1;
				x=&nodes[x->parent];
			} else {
				if (!nodes[w->left].red) {
					nodes[w->right].red=0;
					w->red=1;
					LeftRotate(w);
					w=&nodes[nodes[x->parent].left];
				}
				w->red=nodes[x->parent].red;
				nodes[x->parent].red=0;
				nodes[w->left].red=0;
				RightRotate(&nodes[x->parent]);
				x=rootLeft; /* this is to exit while loop */
			}
		}
	}
	x->red=0;

#ifdef CHECK_RB_TREE_ASSUMPTIONS
	CheckAssumptions();
#elif defined(DEBUG_ASSERT)
	Assert(!nil->red,"nil not black in RedBlackTree::DeleteFixUp");
#endif
}


/***********************************************************************/
/*  FUNCTION:  DeleteNode */
/**/
/*    INPUTS:  tree is the tree to delete node z from */
/**/
/*    OUTPUT:  returns the RedBlackEntry stored at deleted node */
/**/
/*    EFFECT:  Deletes z from tree and but don't call destructor */
/**/
/*    Modifies Input:  z */
/**/
/*    The algorithm from this function is from _Introduction_To_Algorithms_ */
/***********************************************************************/

void RedBlackTree::DeleteNode(unsigned zi){
	RedBlackTreeNode * z = &nodes[zi];
  RedBlackTreeNode* y;
  RedBlackTreeNode* x;
	RedBlackTreeNode* px;
  //RedBlackEntry * returnValue = z->storedEntry;

	if(tail==z){
		tail = GetPredecessorOf(z);
		taili = tail->self;
	}

  y= ((z->left == nil->self) || (z->right == nil->self)) ? z : GetSuccessorOf(z);
  x= (y->left == nil->self) ? &nodes[y->right] : &nodes[y->left];
	px = &nodes[y->parent];
  if (root->self == (x->parent = y->parent)) { /* assignment of y->p to x->p is intentional */
    root->left=x->self;
  } else {
    if (y->self == nodes[y->parent].left) {
      nodes[y->parent].left=x->self;
    } else {
      nodes[y->parent].right=x->self;
    }
  }
  if (y != z) { /* y should not be nil in this case */

#ifdef DEBUG_ASSERT
    Assert( (y!=nil),"y is nil in DeleteNode \n");
#endif
    /* y is the node to splice out and x is its child */
  
    y->left=z->left;
    y->right=z->right;
    y->parent=z->parent;
    nodes[z->left].parent=nodes[z->right].parent=y->self;
    if (z->self == nodes[z->parent].left) {
      nodes[z->parent].left=y->self; 
    } else {
      nodes[z->parent].right=y->self;
    }

		if(px==z)
			FixupSums(y);
		else
			FixupSums(px);

    if (!(y->red)) {
      y->red = z->red;
      DeleteFixUp(x);
    } else
      y->red = z->red; 

		    //delete z;
#ifdef CHECK_RB_TREE_ASSUMPTIONS
    CheckAssumptions();
#elif defined(DEBUG_ASSERT)
    Assert(!nil->red,"nil not black in RedBlackTree::Delete");
#endif
  } else {
		FixupSums(px);
    if (!(y->red)) 
		  DeleteFixUp(x);

    //delete y;
#ifdef CHECK_RB_TREE_ASSUMPTIONS
    CheckAssumptions();
#elif defined(DEBUG_ASSERT)
    Assert(!nil->red,"nil not black in RedBlackTree::Delete");
#endif
  }

	freelist.push(zi);
	

  //return returnValue;
}

/***********************************************************************/
/*  FUNCTION:  Enumerate */
/**/
/*    INPUTS:  tree is the tree to look for keys between [low,high] */
/**/
/*    OUTPUT:  stack containing pointers to the nodes between [low,high] */
/**/
/*    Modifies Input: none */
/**/
/*    EFFECT:  Returns a stack containing pointers to nodes containing */
/*             keys which in [low,high]/ */
/**/
/***********************************************************************/

/*
TemplateStack<RedBlackTreeNode *> * RedBlackTree::Enumerate(int low, 
							    int high)  {
  TemplateStack<RedBlackTreeNode *> * enumResultStack = 
    new TemplateStack<RedBlackTreeNode *>(4);

  RedBlackTreeNode* x=root->left;
  RedBlackTreeNode* lastBest=NULL;

  while(nil != x) {
    if ( x->key > high ) {
      x=x->left;
    } else {
      lastBest=x;
      x=x->right;
    }
  }
  while ( (lastBest) && (low <= lastBest->key) ) {
    enumResultStack->Push(lastBest);
    lastBest=GetPredecessorOf(lastBest);
  }
  return(enumResultStack);
}
*/

void RedBlackTree::CheckAssumptions() const {
 //VERIFY(nil->key == MIN_INT);
 //VERIFY(root->key == MAX_INT);
 //VERIFY(nil->storedEntry == NULL);
 //VERIFY(root->storedEntry == NULL);
 VERIFY(nil->red == 0);
 VERIFY(root->red == 0);
}

/*
std::pair<int,RedBlackTreeNode *> RedBlackTree::SearchTopDown(RedBlackTreeNode * top, int key, int depth=0) const {
  RedBlackTreeNode * w = top;
  //std::cout << "TopDown: ";
  //if(w!=nil)
  //  w->GetEntry()->Print();

  if(w==nil){
		//fprintf(stderr,"hops: %d\n",depth);
		SearchStats::search_depth_sum+=depth;
    return std::pair<int,RedBlackTreeNode*>(0,nil);
	}
  
  if(key == w->key){
		//fprintf(stderr,"hops: %d\n",depth);
		SearchStats::search_depth_sum+=depth;
    return std::pair<int,RedBlackTreeNode*>(w->right->sum+1,w);
	}else if(key < w->key){
    auto res = SearchTopDown(w->left,key,depth+1);
    res.first += w->valid+w->right->sum;
    return res;
  }else
    return SearchTopDown(w->right,key,depth+1);
}
*/

unsigned RedBlackTree::GetDistance(unsigned xi)const{
	RedBlackTreeNode * x = &nodes[xi];
  RedBlackTreeNode * z = x;
	RedBlackTreeNode * rootLeft = &nodes[root->left];

	unsigned rd = 1 + nodes[x->right].sum;

	while(z!=rootLeft){
	  if(nodes[z->parent].left == z->self)
			rd += nodes[nodes[z->parent].right].sum + 1;
		z=&nodes[z->parent];
	}

	return rd;
}
/*
std::pair<int,RedBlackTreeNode*> RedBlackTree::SearchBottomUp(RedBlackTreeNode * maxNode, int key)const {
	SearchStats::searches++;
  RedBlackTreeNode * w = maxNode;
  if(w==nil || key > w->key)
    return std::pair<int,RedBlackTreeNode*>(0,nil);

  RedBlackTreeNode * p;
  RedBlackTreeNode * rootLeft = root->left;

	int height = 0;

  while(true){
		height++;
    p = w->parent;
    //p->GetEntry()->Print();
    if(key == w->key)
      return std::pair<int,RedBlackTreeNode*>(1+w->right->sum,w);
    else if(key > p->key){
      auto res = SearchTopDown(w->left,key,height);
      res.first += w->right->sum + 1;
      return res;
    }
    else if(p==rootLeft)
      return SearchTopDown(p,key,height);
    w = p;
  }
  assert(false && "should not come here!\n");
}
*/
