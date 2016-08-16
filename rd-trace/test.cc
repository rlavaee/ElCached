#include "RedBlackTree.h"

int main(){
  RedBlackTree rb_tree;
  rb_tree.InsertTail(new RedBlackTreeNode(new RedBlackEntry(1)));
	std::cerr<<"*****************\n";
   rb_tree.Print();
	std::cerr<<"*****************\n";
  rb_tree.InsertTail(new RedBlackTreeNode(new RedBlackEntry(2)));
	std::cerr<"*****************\n";
   rb_tree.Print();
	std::cerr<<"*****************\n";
  rb_tree.InsertTail(new RedBlackTreeNode(new RedBlackEntry(3)));
	std::cerr<"*****************\n";
   rb_tree.Print();
	std::cerr<<"*****************\n";
  auto * _node4 = rb_tree.InsertTail(new RedBlackTreeNode(new RedBlackEntry(4)));
	std::cerr<"*****************\n";
   rb_tree.Print();
	std::cerr<<"*****************\n";
  auto * _node5 = rb_tree.InsertTail(new RedBlackTreeNode(new RedBlackEntry(5)));
	std::cerr<"*****************\n";
   rb_tree.Print();
	std::cerr<<"*****************\n";
  auto * _node6 = rb_tree.InsertTail(new RedBlackTreeNode(new RedBlackEntry(6)));
	std::cerr<<"*****************\n";
   rb_tree.Print();
	std::cerr<<"*****************\n";
	

  //rb_tree.DeleteNode(_node);

  auto * _node2 = rb_tree.InsertTail(new RedBlackTreeNode(new RedBlackEntry(7)));

	std::cerr<<"*****************\n";
   rb_tree.Print();
	std::cerr<<"*****************\n";

  auto * _node3 = rb_tree.InsertTail(new RedBlackTreeNode(new RedBlackEntry(8)));

	std::cerr<<"*****************\n";
   rb_tree.Print();
	std::cerr<<"*****************\n";

  rb_tree.DeleteNode(_node4);

  rb_tree.Print();
  int rd = rb_tree.GetDistance(_node6);
  std::cout << "searching for 1: found: \n";
  //res.second->GetEntry()->Print();
  std::cout << "\tsum: " << rd << "\n";
  return 0;
}
