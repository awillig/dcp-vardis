#include <functional>
#include <list>
#include <random>
#include <gtest/gtest.h>
#include <dcp/common/fixedmem_avl_tree.h>
#include <dcp/common/global_types_constants.h>

using dcp::FixedMemAVLTree;
using dcp::NodeIdentifierT;
using dcp::nullNodeIdentifier;

std::mt19937 generator;


std::list<int> get_random_permutation (int length, int num_shuffles)
{
  // initialize linear list
  std::list<int> lst;
  for (int i=0; i<length; i++)
    lst.push_back (i);
  
  // now perform top-to-random shuffles
  std::uniform_int_distribution<> u_distr (0, length);
  for (int i=0; i<num_shuffles; i++)
    {
      int hd_elem = lst.front();
      lst.pop_front ();
      auto it = lst.begin();
      int steps = u_distr(generator);
      for (int j=0; j<steps; j++)  it++;
      lst.insert (it, hd_elem);
    }

  return lst;
}


const uint64_t arraySize = 300;
const uint64_t treeSize  = 250;
const uint64_t perm_iterations = 2000;
const uint64_t iterations = 500;


TEST (AVLTest, initialTest) {
  FixedMemAVLTree<int, std::string, arraySize>  avl_tree;

  for (uint64_t i=0; i<iterations; i++)
    {
      EXPECT_EQ (avl_tree.get_number_elements (), 0);
      std::list perm = get_random_permutation (150, 1000);
      uint64_t cnt = 0;
      for (const int j : perm)
	{
	  cnt++;
	  std::string str (std::format ("str-{}", j));
	  EXPECT_FALSE (avl_tree.is_member (j));
	  avl_tree.insert (j, str);
	  EXPECT_TRUE (avl_tree.is_member (j));
	  avl_tree.insert (j, str);
	  EXPECT_EQ (avl_tree.get_number_elements (), cnt);
	  EXPECT_EQ (avl_tree.get_number_elements (), avl_tree.number_reachable ());
	  EXPECT_TRUE (avl_tree.is_consistent ());
	}

      perm.reverse ();
      uint64_t perm_size = perm.size();
      cnt = 0;
      for (const int j : perm)
	{
	  cnt++;
	  avl_tree.remove (j);
	  avl_tree.remove (j);
	  EXPECT_EQ (avl_tree.get_number_elements (), perm_size - cnt);
	  EXPECT_EQ (avl_tree.get_number_elements (), avl_tree.number_reachable ());
	  EXPECT_TRUE (avl_tree.is_consistent ());
	}
    }  
}

TEST (AVLTest, find_matching_data_test) {
  FixedMemAVLTree<int, int, arraySize>  avl_tree;

  for (int j = 0; j < 20; j++)
    {
      avl_tree.insert (j, j);
    }

  EXPECT_EQ (avl_tree.get_number_elements (), 20);
  EXPECT_EQ (avl_tree.number_reachable (), 20);
  EXPECT_TRUE (avl_tree.is_consistent ());

  std::function<bool (int, const int&)> predicate = [] (int, const int& val) { return (val % 2 == 0); };
  std::function<double (int, const int&)> transform = [] (int, const int& val) { return ((double) val); };
  std::list<double> result_list;

  avl_tree.find_matching_data<double> (predicate, transform, result_list);

  EXPECT_EQ (result_list.size(), 10);
}
