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

#include <dcpsim/common/AssemblyArea.h>

namespace dcp {


class SerializationException : public std::exception {
 public:
  SerializationException (const std::string& message) : message_(message) {};
  const char* what() const throw() { return message_.c_str(); };
 private:
  std::string message_;
};



template <size_t FS>
class TransmissibleType {
 public:
    // sizes of serialized data type.
  // fixed_size reports the combined size of the static (always-present) parts
  // of a tranmissible data type
  // total_size reports the combined size of the static (always-present) and
  // variable parts of a transmissible data type
  static size_t fixed_size() { return FS; };
  virtual size_t total_size() const { return fixed_size(); };

  virtual void serialize (AssemblyArea& area) = 0;
  virtual void deserialize (DisassemblyArea& area) = 0;
};


};  // namespace dcp
