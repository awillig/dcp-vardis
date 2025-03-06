/**
 * Copyright (C) 2025 Andreas Willig, University of Canterbury
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */


#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <dcp/common/exceptions.h>
#include <dcp/common/ring_buffer.h>


/**
 * @brief This module provides an AVL tree (i.e. a balanced binary
 *        search tree) over a fixed-size array of entries.
 *
 * Normal AVL tree implementations use pointers, which makes them
 * unsuitable for storing the tree in a shared memory segment. This
 * AVL tree implementation implements the tree over a fixed-size
 * array.
 *
 * The tree entries contain a key and an associated data element.
 */


namespace dcp {

  const int A_NULL     = -1;    /*!< Index corresponding to a null pointer */

  /**
   * @brief Balanced binary search tree over an array
   *
   * @tparam KeyT: type name of the key type. Must be a totally
   *         ordered type.
   * @tparam DataT: type name of data entries. Must have copy
   *         constructor.
   * @tparam arraySize: size of the array to be used, gives
   *         maximum number of nodes in the tree.
   */
  template <typename KeyT, typename DataT, uint64_t arraySize>
  class ArrayAVLTree {

    static_assert (std::totally_ordered<KeyT>, "ArrayAVLTree: KeyT must be totally ordered");
    static_assert (arraySize >= 3, "ArrayAVLTree: arraySize must be at least three");
    
  protected:
    
    /*********************************************************************
     * Types and data members
     ********************************************************************/
    
    /**
     * @brief Type containing tree management data
     */
    class NodeT {
    public:
      int  left    = A_NULL;   /*!< Index of left child */
      int  right   = A_NULL;   /*!< Index of right child */
      int  height  = 0;        /*!< Height of this node */
    };


    /**
     * @brief Type representing a tree node
     */
    typedef struct ANode {
      KeyT   key;             /*!< Key, must be of totally ordered type */
      DataT  data;            /*!< Data field */
      NodeT  nd;              /*!< Tree management data */
    } ANode;
    

    
    RingBufferBase<int, arraySize+1>  freeList;    /*!< List, elements are indices of free array entries */
    ANode      the_array [arraySize];              /*!< Array storing tree nodes */
    uint64_t   number_elements = 0;                /*!< Number of current elements in the tree */
    int        root            = A_NULL;           /*!< Index of tree root */



    /*********************************************************************
     * Getters and setters
     ********************************************************************/


    /**
     * @brief Return index of left child
     *
     * @param idx: array index of tree node
     */
    inline int left_ch (int idx) const { return the_array[idx].nd.left; };


    /**
     * @brief Return index of right child
     *
     * @param idx: array index of tree node
     */
    inline int right_ch (int idx) const { return the_array[idx].nd.right; };


    /**
     * @brief Tells whether node has valid (non-null) left child
     *
     * @param idx: array index of tree node
     */
    inline bool has_left_child (int idx) const { return the_array[idx].nd.left != A_NULL; };


    /**
     * @brief Tells whether node has valid (non-null) right child
     *
     * @param idx: array index of tree node
     */
    inline bool has_right_child (int idx) const { return the_array[idx].nd.right != A_NULL; };


    /**
     * @brief Set left child to given array index
     *
     * @param idx: array index of tree node
     * @param new_ch: index of new left child node
     */
    inline void set_left_ch (int idx, int new_ch) { if (not is_null (idx)) the_array[idx].nd.left = new_ch; };


    /**
     * @brief Set right child to given array index
     *
     * @param idx: array index of tree node
     * @param new_ch: index of new right child node
     */
    inline void set_right_ch (int idx, int new_ch) { if (not is_null (idx)) the_array[idx].nd.right = new_ch; };


    /**
     * @brief Tests is given index is null
     *
     * @param idx: array index of tree node
     */
    inline bool is_null (int idx) const { return idx < 0; };


    /**
     * @brief Return height value of node
     *
     * @param idx: array index of tree node
     */
    inline int n_height (int idx) const { return is_null(idx) ? 0 : the_array[idx].nd.height; };


    /**
     * @brief Set height value of node
     *
     * @param idx: array index of tree node
     * @param height: new height of tree node
     */
    inline void set_height (int idx, int height) { if (not is_null (idx))  the_array[idx].nd.height = height; };


    /**
     * @brief Calculates balance of given tree node
     *
     * @param idx: array index of tree node
     */
    inline int n_balance (int idx) const { return is_null(idx) ? 0 : (n_height(left_ch(idx)) - n_height(right_ch(idx)));  };


    /**
     * @brief Retrieves key of given tree node
     *
     * @param idx: array index of tree node
     */
    inline KeyT  n_key (int idx) const { return the_array[idx].key; };


    /**
     * @brief Retrieves data field of given tree node
     *
     * @param idx: array index of tree node
     */    
    inline DataT n_data (int idx) const { return the_array[idx].data; };


    /*********************************************************************
     * Helpers
     ********************************************************************/

    
    /**
     * @brief Initializes a new node at given array index
     *
     * @param idx: array index of new node
     * @param key: key of new node
     * @param data: data value of new node
     */
    inline void new_node_at (int idx, KeyT key, DataT& data)
    {
      the_array[idx].key        = key;
      the_array[idx].data       = data;
      the_array[idx].nd.left    = A_NULL;
      the_array[idx].nd.right   = A_NULL;
      the_array[idx].nd.height  = 1;
    };


    /**
     * @brief Performs rotate-right operation at given node index
     *
     * @param y: array index of node to rotate around
     */
    inline int rotate_right (int y)
    {
      int x = left_ch (y);
      int z = right_ch (x);
      
      KeyT   y_key   = n_key (y);
      DataT  y_data  = n_data (y);
      int    y_left  = z;
      int    y_right = right_ch (y);
      
      the_array[y].key  = the_array[x].key;
      the_array[y].data = the_array[x].data;
      set_left_ch (y, left_ch (x));
      set_right_ch (y, x);

      the_array[x].key   = y_key;
      the_array[x].data  = y_data;
      set_left_ch (x, y_left);
      set_right_ch (x, y_right);

      set_height (x, calc_height (x));
      set_height (y, calc_height (y));
      
      return y;
    };

    
    /**
     * @brief Performs rotate-left operation at given node index
     *
     * @param x: array index of tree node to rotate around
     */
    inline int rotate_left (int x)
    {
      int y = right_ch (x);
      int z = left_ch (y);

      KeyT   x_key    = n_key (x);
      DataT  x_data   = n_data (x);
      int    x_left   = left_ch (x);
      int    x_right  = z;
      
      the_array[x].key  = the_array[y].key;
      the_array[x].data = the_array[y].data;
      set_left_ch (x, y);
      set_right_ch (x, right_ch(y));

      the_array[y].key    = x_key;
      the_array[y].data   = x_data;
      set_left_ch (y, x_left);
      set_right_ch (y, x_right);

      set_height (x, calc_height (x));
      set_height (y, calc_height (y));

      return x;
    };


    /**
     * @brief Finds the minimum key value of the sub-tree starting at
     *        given index
     *
     * @param idx: array index of root of the sub-tree
     */
    int min_key_node (int idx)
    {
      int current = idx;
      while (not is_null (left_ch (current)))
	current = left_ch (current);
      return current;
    };


    /**
     * @brief Insert given key and data into the sub-tree starting at
     *        the given indes, allocating new array element when the
     *        key does not yet exist, otherwise just updating the data
     *        at the given index
     *
     * @param idx: array index of root of the sub-tree
     * @param key: key value to insert
     * @param data: data value to insert
     *
     * @return New root node
     */
    int insert (int idx, KeyT key, DataT& data)
    {
      if (is_null (idx))
	{
	  int newidx  = freeList.pop ();
	  new_node_at (newidx, key, data);
	  number_elements += 1;
	  return newidx;
	}

      if (key < n_key (idx))
	set_left_ch (idx, insert (left_ch (idx), key, data));
      else if (key > n_key (idx))
	set_right_ch (idx, insert (right_ch (idx), key, data));
      else
	{
	  the_array[idx].data = data;
	  return idx;
	}

      set_height (idx, 1 + std::max( n_height (left_ch(idx)), n_height (right_ch(idx))));
      int balance = n_balance (idx);

      if ((balance > 1) and (key < n_key (left_ch (idx))))
	{
	  return rotate_right (idx);
	}

      if ((balance < -1) and (key > n_key (right_ch (idx))))
	{
	  return rotate_left (idx);
	}

      if ((balance > 1) and (key > n_key (left_ch (idx))))
	{
	  set_left_ch (idx, rotate_left (left_ch (idx)));
	  return rotate_right (idx);
	}

      if ((balance < -1) and (key < n_key (right_ch (idx))))
	{
	  int rr_result = rotate_right (right_ch (idx));
	  set_right_ch (idx, rr_result);
	  int rl_result = rotate_left (idx);
	  return rl_result;
	}

      return idx;
    };


    /**
     * @brief Removes the tree node with the given key from the
     *        sub-tree starting at the given root note
     *
     * @param root: array index of the root of the sub-tree
     * @param key: key value to remove
     * @param do_decrement: flag indicating whether number_elements
     *        member should be decremented
     *
     * @return New root node
     */
    int remove (int root, KeyT key, bool do_decrement)
    {
      if (is_null(root)) return root;

      if (key < n_key (root))
	set_left_ch (root, remove (left_ch (root), key, do_decrement));
      else if (key > n_key (root))
	set_right_ch (root, remove (right_ch (root), key, do_decrement));
      else
	{
	  if (do_decrement)
	    number_elements--;
	  if ((not has_left_child (root)) or (not has_right_child (root)))
	    {
	      int tmp = has_left_child(root) ? left_ch (root) : right_ch (root);
	      
	      if (is_null (tmp))
		{
		  freeList.push (root);
		  root = A_NULL;
		}
	      else
		{
		  the_array[root] = the_array[tmp];
		  freeList.push (tmp);
		}
	    }
	  else
	    {
	      int   tmp = min_key_node (right_ch (root));
	      KeyT  tmpkey = n_key (tmp);
	      the_array[root].key  =  the_array[tmp].key;
	      the_array[root].data =  the_array[tmp].data;
	      set_right_ch(root, remove (right_ch (root), tmpkey, false));
	    }
	}

      if (is_null(root))
	return root;

      set_height (root, 1 + std::max( n_height (left_ch(root)), n_height (right_ch(root))));
      int balance = n_balance (root);

      if ((balance > 1) and (n_balance (left_ch (root)) >= 0))
	return rotate_right(root);

      if ((balance > 1) and (n_balance(left_ch (root)) < 0))
	{
	  set_left_ch (root, rotate_left (left_ch (root)));
	  return rotate_right (root);
	}

      if ((balance < -1) and n_balance (right_ch (root)) <= 0)
	return rotate_left (root);

      if ((balance < -1) and n_balance (right_ch (root)) > 0)
	{
	  set_right_ch (root, rotate_right (right_ch (root)));
	  return rotate_left (root);
	}
      
      return root;
    };
    

    /**
     * @brief Search node with the given key in the given sub-tree.
     *
     * @param idx: index of root node of sub-tree
     * @param key: key to lookup up
     *
     * @return: index of tree node containing given key, or A_NULL
     *          when such a node does not exist
     */
    inline int lookup (int idx, KeyT key) const
    {
      if (idx < 0) return A_NULL;
      if (key == n_key(idx)) return idx;
      if (key < n_key(idx)) return lookup (left_ch (idx), key);
      return lookup (right_ch (idx), key);
    };


    /**
     * @brief Calculates the actual height of given sub-tree
     *
     * @param idx: index of the sub-tree root
     *
     * @return height value
     */
    inline int calc_height (int idx) const
    {
      if (is_null (idx)) return 0;
      return 1 + std::max (calc_height(left_ch(idx)), calc_height(right_ch(idx)));
    };


    /**
     * @brief Calculates the number of nodes reachable in the given
     *        sub-tree
     *
     * @param idx: index of the sub-tree root
     *
     * @return number of nodes in the sub-tree
     */
    inline uint64_t number_reachable (int idx) const
    {
      if (is_null(idx)) return 0;
      return 1 + number_reachable (left_ch(idx)) + number_reachable (right_ch (idx));
    };


    /**
     * @brief Checks consistency of given sub-tree, used for testing
     *
     * @param idx: index of the sub-tree root
     *
     * @return Flag whether sub-tree is consistent or not
     *
     * Checks are: correct key values (key of node is larger than key
     * of left child and smaller than key of right child), sub-tree
     * has correct balance (-1, 0 or 1), the stored height is equal to
     * calculated height, and left and right sub-trees are consistent.
     */
    inline bool is_consistent (int idx) const
    {
      if (is_null (idx)) return true;

      bool left_cons = is_null (left_ch(idx)) ? true : n_key (idx) > n_key (left_ch (idx));
      bool right_cons = is_null (left_ch(idx)) ? true : n_key (idx) > n_key (left_ch (idx));

      if (not left_cons)
	{
	  return false;
	}

      if (not right_cons)
	{
	  return false;
	}

      int bal = n_balance(idx);
      if ((bal < -1) or (bal > 1))
	{
	  return false;
	}

      if (n_height(idx) != calc_height(idx))
	{
	  return false;
	}
      
      return (is_consistent(left_ch(idx)) and is_consistent(right_ch(idx)));
    };


    /**
     * @brief Retrieves a list of all the tree node indices in the
     *        given sub-tree for which their data satisfies a given
     *        predicate
     *
     * @param idx: index of the sub-tree root
     * @param predicate: checks given data field whether or not predicate
     *        is satisfied
     * @param lst: output parameter containing the list of all node indices
     *        of the nodes satisfying the predicate
     *
     */
    void find_matching_keys (int idx, std::function<bool (const DataT&)> predicate, std::list<KeyT>& lst) const
    {
      if (is_null(idx)) return;
      
      if (predicate (the_array[idx].data))
	lst.push_back (the_array[idx].key);
      
      find_matching_keys (left_ch (idx), predicate, lst);
      find_matching_keys (right_ch (idx), predicate, lst);
    };
   
  public:

    /*********************************************************************
     * Public methods
     ********************************************************************/


    /**
     * @brief Constructor, initializes array, root and free list
     */
    ArrayAVLTree ()
      : freeList ("AVL", arraySize)
    {
      clear ();
    };


    /**
     * @brief Returns size of the array containing the tree
     */
    inline uint64_t get_array_size () const { return arraySize; };


    /**
     * @brief Returns current number of nodes in the tree
     */
    inline uint64_t get_number_elements () const { return number_elements; };


    /**
     * @brief Check whether node with given index can be found and
     *        returns its index
     *
     * @param key: key value to find
     *
     * @return either index of node when exists, or A_NULL
     */
    inline int lookup (KeyT key) const
    {
      return lookup (root, key);
    };


    /**
     * @brief Returns reference to the data field of an existing node
     *        in the tree
     *
     * @param key: key value for which to return reference to data field
     *
     * @return Reference to data field.
     *
     * Throws when no node with given key exists.
     */
    inline DataT& lookup_data_ref (KeyT key)
    {
      int idx = lookup (key);
      if (is_null (idx))
	throw AVLTreeException ("lookup_data_ref: unknown key");
      return the_array[idx].data;
    };


    /**
     * @brief Checks whether node with given index can be found
     *
     * @param key: key value to find
     *
     * @return flag saying whether node exists or not
     */
    inline bool is_member (KeyT key) const
    {
      return (not is_null (lookup (root, key)));
    };


    /**
     * @brief Insert given key / data pair into the tree, if there is
     *        space available
     *
     * @param key: new key value
     * @param data: new data value
     *
     * If the key already exists in the tree, its data value is
     * updated with the new data.
     */
    void insert (KeyT key, DataT& data)
    {
      if (number_elements < arraySize)
	root = insert (root, key, data);
    };
    

    /**
     * @brief Removes node with given key from the tree
     *
     * @param key: key to remove
     */
    void remove (KeyT key)
    {
      root = remove (root, key, true);
    };


    /**
     * @brief Clears the tree, it is freshly initialized after this operation.
     */
    inline void clear ()
    {
      root             = A_NULL;
      number_elements  = 0;
      freeList.reset();
      for (uint64_t i = 0; i < arraySize; ++i)
	{
	  freeList.push (i);
	  the_array[i].nd.left   = A_NULL;
	  the_array[i].nd.right  = A_NULL;
	  the_array[i].nd.height = 0;
	}
    };


    /**
     * @brief Retrieves a list of all the tree node indices in the
     *        tree for which their data satisfies a given predicate
     *
     * @param predicate: checks given data field whether or not predicate
     *        is satisfied
     * @param result_lst: output parameter containing the list of all
     *        node indices of the nodes satisfying the predicate
     *
     */
    void find_matching_keys (std::function<bool (const DataT&)> predicate, std::list<KeyT>& result_lst) const
    {
      find_matching_keys (root, predicate, result_lst);
    };

    /*********************************************************************
     * Public methods only relevant for unit testing
     ********************************************************************/

    /**
     * @brief Returns current number of nodes reachable from the root
     */    
    inline uint64_t number_reachable () const
    {
      return number_reachable (root);
    };
    

    /**
     * @brief Runs consistency check on the entire tree
     *
     * @return Flag saying whether any of the consistency rules has been violated
     */
    bool is_consistent ()
    {
      return is_consistent (root);
    };

    
  };  
    
};  // namespace dcp
