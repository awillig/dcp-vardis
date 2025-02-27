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



#include <dcp/common/debug_helpers.h>


std::string byte_array_to_string (byte* bytes, size_t len)
{
  if (len > 0)
    {
      std::stringstream ss;
      ss << std::hex;
      
      ss << "{";
      for (unsigned int i=0; i<len-1; i++)
	ss << std::setw(2) << std::setfill('0') << (int) bytes[i] << ",";
      ss << std::setw(2) << std::setfill('0') << (int) bytes[len-1];
      ss << "}";
      
      return ss.str();
    }
  else
    {
      return "";
    }
}
