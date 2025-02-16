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

#include <iostream>
#include <dcp/common/area.h>

/**
 * @brief This module contains base classes for transmissible data
 *        types.
 *
 * The main feature of a transmissible data type is its ability to
 * serialize / deserialize itself. The 'TransmissibleIntegral' type in
 * addition offers a range of arithmetic operations.
 */


namespace dcp {

  /**
   * @brief Base class for transmissible data types, providing
   *        serialization and deserialization.
   *
   * A transmissible type can generally be of variable size, but is
   * assumed to have fixed component and a variable component. The
   * numerical template parameter gives the serialized size of the
   * fixed component in bytes.
   */
  template <size_t FS>
  class TransmissibleType {
  public:

    /**
     * @brief Returns the serialized size of the fixed component in bytes
     */
    static constexpr size_t fixed_size() { return FS; };

    
    /**
     * @brief Return the serialized total size in bytes (sum of fixed
     *        and variable components)
     *
     * This provides a default implementation that just considers the
     * fixed size. Transmissible data types of variable size need to
     * overload this.
     */
    virtual size_t total_size() const { return fixed_size(); };

    
    /**
     * @brief Serializes instance into given assembly area. Should be
     *        an abstract method, but is a concrete one to enable
     *        concept-based implementation of class
     *        TransmissibleIntegral
     */
    void serialize (AssemblyArea& area) const {};

    
    /**
     * @brief Deserializes instance form given disassembly
     *        area. Should also have been abstract.
     */
    void deserialize (DisassemblyArea& area) {};
  };


  // =============================================================================
  // =============================================================================

  /**
   * @brief The following template is for 8-, 16-, 32- or 64-bit
   *        header fields that additionally need arithmetic operations
   *        support (e.g. comparison operators)
   */
  
  template <typename T>
  class TransmissibleIntegral : public TransmissibleType<sizeof(T)> {
  private:
    static_assert(std::is_integral<T>::value, "Template TransmissibleIntegral: T must be an integral type");
  public:
    T  val;

    /**
     * @brief Constructors (default, initialization from value type, copy constructor)
     */
    TransmissibleIntegral () : val(0) {};
    TransmissibleIntegral (T v) : val(v) {};
    TransmissibleIntegral (const TransmissibleIntegral<T>& other) : val(other.val) {};

    /**
     * @brief Comparison operators
     */
    inline bool operator== (const TransmissibleIntegral<T>& other) const { return val == other.val; };
    inline bool operator== (const T& other) const { return val == other; };
    inline bool operator!= (const TransmissibleIntegral<T>& other) const { return val != other.val; };
    inline bool operator!= (const T& other) const { return val != other; };
    inline bool operator<= (const TransmissibleIntegral<T>& other) const { return val <= other.val; };
    inline bool operator<= (const T& other) const { return val <= other; };
    inline bool operator>= (const TransmissibleIntegral<T>& other) const { return val >= other.val; };
    inline bool operator>= (const T& other) const { return val >= other; };
    inline bool operator< (const TransmissibleIntegral<T>& other) const { return val < other.val; };
    inline bool operator< (const T& other) const { return val < other; };
    inline bool operator> (const TransmissibleIntegral<T>& other) const { return val > other.val; };
    inline bool operator> (const T& other) const { return val > other; };

    /**
     * @brief Assignment and other modifying operators
     */
    inline TransmissibleIntegral<T>& operator= (const TransmissibleIntegral<T>& other) { val = other.val; return *this; };
    inline TransmissibleIntegral<T>& operator= (const T& v)     { val = v; return *this; };
    inline TransmissibleIntegral<T>& operator+= (const TransmissibleIntegral<T>& rhs) { val += rhs.val; return *this; };
    inline TransmissibleIntegral<T>& operator+= (T v) { val += v; return *this; };
    inline TransmissibleIntegral<T>& operator-= (const TransmissibleIntegral<T>& rhs) { val -= rhs.val; return *this; };
    inline TransmissibleIntegral<T>& operator-= (T v) { val -= v; return *this; };
    inline TransmissibleIntegral<T> operator-- (int) { val = val-1; return *this; };
    inline TransmissibleIntegral<T> operator++ (int) { val = val+1; return *this; };


    /**
     * @brief Friend arithmetic operators
     */
    friend inline TransmissibleIntegral<T> operator+ (TransmissibleIntegral<T> lhs, const TransmissibleIntegral<T>& rhs) { lhs += rhs; return lhs; };
    friend inline TransmissibleIntegral<T> operator+ (TransmissibleIntegral<T> lhs, const T& rhs) { lhs += rhs; return lhs; };
    friend inline TransmissibleIntegral<T> operator- (TransmissibleIntegral<T> lhs, const T& rhs) { lhs -= rhs; return lhs; };
    friend inline TransmissibleIntegral<T> operator- (TransmissibleIntegral<T> lhs, const TransmissibleIntegral<T>& rhs) { lhs -= rhs; return lhs; };


    /**
     * @brief Serialization functions for the different supported
     *        sizes of the integral type
     */
    inline void serialize (AssemblyArea& area) const requires (sizeof(T) == 1) { area.serialize_byte ((byte) val); };
    inline void serialize (AssemblyArea& area) const requires (sizeof(T) == 2) { area.serialize_uint16_n ((uint16_t) val); };
    inline void serialize (AssemblyArea& area) const requires (sizeof(T) == 4) { area.serialize_uint32_n ((uint32_t) val); };
    inline void serialize (AssemblyArea& area) const requires (sizeof(T) == 8) { area.serialize_uint64_n ((uint64_t) val); };

    /**
     * @brief Deserialization functions for the different supported
     *        sizes of the integral type
     */
    inline void deserialize (DisassemblyArea& area) requires (sizeof(T)==1) {val = area.deserialize_byte ();};
    inline void deserialize (DisassemblyArea& area) requires (sizeof(T)==2) {area.deserialize_uint16_n (val);};
    inline void deserialize (DisassemblyArea& area) requires (sizeof(T)==4) {area.deserialize_uint32_n (val);};
    inline void deserialize (DisassemblyArea& area) requires (sizeof(T)==8) {area.deserialize_uint64_n (val);};    
    
  };

  
  // =============================================================================
  // =============================================================================
  
};  // namespace dcp
