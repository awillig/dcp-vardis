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

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <dcp/vardis/vardis_protocol_statistics.h>
#include <dcp/vardis/vardis_rtdb_entry.h>
#include <dcp/vardis/vardis_transmissible_types.h>


namespace dcp::vardis {

  /**
   * @brief This class defines the abstraction of a Vardis variable
   *        store, providing key operations on Vardis variables
   *
   * This was born in an effort to make operations on Vardis variables
   * faster, particularly in a multiprocess implementation, but also
   * allowing for a single-process implementation.
   *
   * The variable store contains for each variable three different
   * components and manages them separately:
   *   - The DBEntry record for the variable (specification, runtime information)
   *   - The variable value (variable-length byte array)
   *   - The variable description (variable-length char array)
   *
   * Furthermore, the variable store contains configuration data
   * relevant for protocol processing (e.g. maxSummaries,
   * ownNodeIdentifier), as well as certain pieces of runtime data (in
   * particular the statistics).
   */

  
  class VariableStoreI {
  public:
    
    /***************************************************************
     * Allocation and deallocation of identifiers
     **************************************************************/
    
    /**
     * @brief Allocates space in the store for previously unused
     *        variable identifier.
     *
     * @param varId: variable identifier
     *
     * Throws when identifier already in use
     */
    virtual void allocate_identifier (const VarIdT varId) = 0;


    /**
     * @brief Deallocates a previously allocated identifier.
     *
     * @param varId: variable identifier
     *
     * Throws when the identifier is not in use.
     */
    virtual void deallocate_identifier (const VarIdT varId) = 0;


    /**
     * @brief Queries whether given identifier is currently allocated
     *
     * @param varId: variable identifier
     */
    virtual bool identifier_is_allocated (const VarIdT varId) = 0;



    /***************************************************************
     * Getters and setters for key configuration data and runtime data
     * (vardisActive, statistics) used in Vardis protocol processing
     **************************************************************/

    /**
     * @brief Returns maximum number of summary instruction records in
     *        a payload
     */
    virtual uint16_t get_conf_max_summaries () const = 0;


    /**
     * @brief Returns maximum length of a variable description
     */
    virtual size_t   get_conf_max_description_length () const = 0;


    /**
     * @brief Returns maximum length of a variable value
     */
    virtual size_t   get_conf_max_value_length () const = 0;


    /**
     * @brief Returns maximum number of repetitions of modifying
     *        instruction records
     */
    virtual uint8_t  get_conf_max_repetitions () const = 0;


    /**
     * @brief Returns the own node identifier
     */
    virtual NodeIdentifierT get_own_node_identifier () const = 0;


    /**
     * @brief Returns current number of registered variables
     */
    virtual unsigned int get_number_variables () const = 0;
    

    /**
     * @brief Return the current vardis_isActive flag
     */
    virtual bool     get_vardis_isactive () const = 0;


    /**
     * @brief Sets the vardis_isActive flag to given value
     *
     * @param active: new value for vardis_isActive flag
     */
    virtual void     set_vardis_isactive (bool active) = 0;


    /**
     * @brief Returns reference to statistics object
     */
    virtual VardisProtocolStatistics& get_vardis_protocol_statistics_ref () const = 0;



    
    /***************************************************************
     * Locking/Unlocking access to variable store
     *
     * Note: these need only be implemented in multi-process
     * implementations
     **************************************************************/


    /**
     * @brief Lock access to variable store
     */
    virtual void lock () {};


    /**
     * @brief Unlock access to variable store
     */
    virtual void unlock () {};


    /***************************************************************
     * Management of DBEntry for a variable
     **************************************************************/

    /**
     * @brief Set the DBEntry for the given varId, but not variable
     *        description or value
     */
    virtual void set_db_entry (const VarIdT varId, const DBEntry& new_entry) = 0;


    /**
     * @brief Get a reference to the DBEntry for the given variable
     *        identifier.
     *
     * Calling code can change key variable settings, but not variable
     * value or description.
     */
    virtual DBEntry& get_db_entry_ref (const VarIdT varId) const = 0;


    /***************************************************************
     * Operations on variable values
     **************************************************************/


    /**
     * @brief Updates variable value from memory location
     *
     * @param varId: variable identifier to modify
     * @param newval: pointer to memory location to copy new
     *        value from into the store
     * @param nvsize: length of new value
     *
     * Throws upon irregularities (e.g. varId not allocated, new value
     * is too long etc).
     */
    virtual void       update_value (const VarIdT varId, byte* newval, VarLenT nvsize) = 0;


    /**
     * @brief Updates variable value from VarValueT
     *
     * @param varId: variable identifier to modify
     * @param newval: VarValueT containing new value to copy into store
     *
     * Throws upon irregularities (e.g. varId not allocated, new value
     * is too long etc).
     */
    virtual void       update_value (const VarIdT varId, const VarValueT& newval) = 0;


    /**
     * @brief Returns a VarValueT containing the variable value
     *
     * @param varId: variable identifier
     * @return VarValueT element for variable value
     *
     * Throws upon irregularities (e.g. varId not allocated)
     */
    virtual VarValueT  read_value (const VarIdT varId) const = 0;


    /**
     * @brief Copies variable value into given buffer. The given
     *        buffer must be large enough, its size is not checked.
     *
     * @param varId: variable identifier
     * @param output_buffer_size: size of application-provided
     *        output buffer for variable value
     * @param output_buffer: memory location to copy value into
     * @param output_size: output parameter containing size of the
     *        variable value
     *
     * Throws upon irregularities (e.g. varId not allocated, given buffer too
     * small)
     */
    virtual void       read_value (const VarIdT varId,
				   size_t output_buffer_size,
				   byte* output_buffer,
				   VarLenT& output_size) const = 0;


    /**
     * @brief Returns size of variable value
     *
     * @param varId: variable identifier
     * @return size of variable value.
     *
     * Throws upon irregularities (e.g. varId not allocated)
     */
    virtual size_t     size_of_value (const VarIdT varId) const = 0;


    /***************************************************************
     * Operations on variable descriptions
     **************************************************************/

    
    /**
     * @brief Updates variable description from StringT
     *
     * @param varId: variable identifier
     * @param new_descr: StringT containing new description to copy into store
     *
     * Throws upon irregularities (e.g. varId not allocated, new description
     * is too long etc).
     */
    virtual void     update_description (const VarIdT varId, const StringT& new_descr) = 0;


    /**
     * @brief Returns StringT value containing variable description
     *
     * @param varId: variable identifier
     * @return StringT value containing variab le description
     *
     * Throws upon irregularities (e.g. varId not allocated).
     */
    virtual StringT  read_description (const VarIdT varId) const = 0;


    /**
     * @brief Copies variable description into given buffer as
     *        zero-terminated string. The buffer must be large enough
     *        (not checked by this method).
     *
     * @param varId: variable identifier
     * @param buf: buffer to copy description into
     *
     * Throws upon irregularities (e.g. varId not allocated).
     */
    virtual void     read_description (const VarIdT varId, char* buf) const = 0;


    /**
     * @brief Returns size of variable description
     *
     * @param varId: variable identifier
     * @return size of variable description.
     *
     * Throws upon irregularities (e.g. varId not allocated)
     */
    virtual size_t   size_of_description (const VarIdT varId) const = 0;
  };

  
};  // namespace dcp::vardis
