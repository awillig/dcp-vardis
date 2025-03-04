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

#include <concepts>
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <type_traits>
#include <dcp/common/ring_buffer.h>

using std::cout;
using std::endl;


namespace dcp {

  const int A_NULL     = -1;
  const int A_NOTFOUND = -2;
  const int A_EXISTS   = -3;

  /**
   * @brief Balanced binary search tree over an array
   *
   * See https://www.bradapp.net/ftp/src/libs/C++/AvlTrees.html
   * See https://www.geeksforgeeks.org/cpp-program-to-implement-avl-tree/
   */
  template <typename KeyT, typename DataT, uint64_t arraySize>
  class ArrayAVLTree {

    static_assert (std::totally_ordered<KeyT>, "ArrayAVLTree: KeyT must be totally ordered");
    static_assert (arraySize >= 3, "ArrayAVLTree: arraySize must be at least three");
    
  protected:

    class NodeT {    
    public:
      int  left    = A_NULL;
      int  right   = A_NULL;
      int  height  = 0;
    };

    
    // types
    typedef struct ANode {
      KeyT   key;
      DataT  data;
      NodeT  nd;
      bool   used = false;
    } ANode;
    
    // data members

    RingBufferBase<int, arraySize+1>  freeList;
    ANode      the_array [arraySize];
    uint64_t   number_elements = 0;
    int        root            = A_NULL;  

    // helpers    
    inline int left_ch (int idx) const { return the_array[idx].nd.left; };
    inline int right_ch (int idx) const { return the_array[idx].nd.right; };
    inline bool has_left_child (int idx) const { return the_array[idx].nd.left != A_NULL; };
    inline bool has_right_child (int idx) const { return the_array[idx].nd.right != A_NULL; };
    inline void set_left_ch (int idx, int new_ch) { if (not is_null (idx)) the_array[idx].nd.left = new_ch; };
    inline void set_right_ch (int idx, int new_ch) { if (not is_null (idx)) the_array[idx].nd.right = new_ch; };
    inline bool is_null (int idx) const { return idx < 0; };
    inline int n_height (int idx) const { return is_null(idx) ? 0 : the_array[idx].nd.height; };
    inline void set_height (int idx, int height) { if (not is_null (idx))  the_array[idx].nd.height = height; };
    inline int n_balance (int idx) const { return is_null(idx) ? 0 : (n_height(left_ch(idx)) - n_height(right_ch(idx)));  };
    inline KeyT  n_key (int idx) const { return the_array[idx].key; };
    inline DataT n_data (int idx) const { return the_array[idx].data; };


    inline void new_node_at (int idx, KeyT key, DataT& data)
    {
      the_array[idx].used       = true;
      the_array[idx].key        = key;
      the_array[idx].data       = data;
      the_array[idx].nd.left    = A_NULL;
      the_array[idx].nd.right   = A_NULL;
      the_array[idx].nd.height  = 1;
    };
    
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

    
    int min_key_node (int idx)
    {
      int current = idx;
      while (not is_null (left_ch (current)))
	current = left_ch (current);
      return current;
    };
    
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
	return idx;

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
    
 
    inline int lookup (int idx, KeyT key) const
    {
      if (idx < 0) return A_NOTFOUND;
      if (key == n_key(idx)) return idx;
      if (key < n_key(idx)) return lookup (left_ch (idx), key);
      return lookup (right_ch (idx), key);
    };

    
    inline int calc_height (int idx) const
    {
      if (is_null (idx)) return 0;
      return 1 + std::max (calc_height(left_ch(idx)), calc_height(right_ch(idx)));
    };

    
    inline uint64_t number_reachable (int idx) const
    {
      if (is_null(idx)) return 0;
      return 1 + number_reachable (left_ch(idx)) + number_reachable (right_ch (idx));
    };
    
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
    
  public:
    
    ArrayAVLTree ()
      : freeList ("AVL", arraySize)
    {
      clear ();
    };

    inline uint64_t get_array_size () const { return arraySize; };
    inline uint64_t get_number_elements () const { return number_elements; };
    
    inline uint64_t number_reachable () const
    {
      return number_reachable (root);
    };
    
    inline int lookup (KeyT key) const
    {
      return lookup (root, key);
    };

    inline bool is_member (KeyT key) const
    {
      return (not is_null (lookup (root, key)));
    };
    
    void insert (KeyT key, DataT& data)
    {
      root = insert (root, key, data);
    };
    

    void remove (KeyT key)
    {
      root = remove (root, key, true);
    };

    bool is_consistent ()
    {
      return is_consistent (root);
    };
    
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
	  the_array[i].used      = false;
	}
    };
    
  };  
    
};  // namespace dcp
