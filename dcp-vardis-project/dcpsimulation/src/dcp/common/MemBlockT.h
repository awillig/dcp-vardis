// Copyright (C) 2024 Andreas Willig, University of Canterbury
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#pragma once

#include <cstdint>
#include <cstring>
#include <dcp/common/FoundationTypes.h>

namespace dcp {

template <typename SizeT> class MemBlockT {
 public:
  SizeT    length = 0;
  byte*    data   = 0;

 public:

  MemBlockT () : length(0), data(nullptr) {};

  MemBlockT (const MemBlockT& other) : length(0), data(nullptr)
  {
      if (other.length > 0 && other.data)
      {
          length  = other.length;
          data    = new byte [other.length];
          std::memcpy(data, other.data, length);
      }
  };

  MemBlockT (const MemBlockT&& other)
  {
      length = other.length;
      data   = other.data;
      other.length = 0;
      other.data   = nullptr;
  };

  MemBlockT (SizeT len, byte* pdata)
  {
      length = len;
      if (length > 0 && pdata)
      {
          data = new byte [length];
          std::memcpy(data, pdata, length);
      }
  };

  ~MemBlockT ()
  {
      if (data)
      {
          delete [] data;
      }
  };

  MemBlockT& operator= (const MemBlockT& other)
  {
      if (this != &other)
      {
          if (data)
          {
              delete [] data;
          }
          length = 0;
          data   = nullptr;
          if (other.length > 0 && other.data)
          {
              length = other.length;
              data   = new byte [length];
              std::memcpy(data, other.data, length);
          }
      }
      return *this;
  };
};

}; // namespace dcp
