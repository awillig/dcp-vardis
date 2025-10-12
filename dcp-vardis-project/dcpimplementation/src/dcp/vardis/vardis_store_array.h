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

#include <atomic>
#include <cstdint>
#include <format>
#include <type_traits>
#include <dcp/common/exceptions.h>
#include <dcp/common/fixedmem_ring_buffer.h>
#include <dcp/vardis/vardis_store_interface.h>


using dcp::FixedMemRingBuffer;


/**
 * @brief This module provides a generic array-based variable store
 *        template, with all its contents being stored in a memory
 *        block with fixed memory location (no heap-based data
 *        structures are being used). The actual memory block will
 *        need to be managed by derived classes (e.g. in a shared
 *        memory area or just a block on the heap).
 *
 * The key structure is simply an array with one entry per variable
 * identifier. This entry contains information about the variable
 * itself (its DBEntry), a reference to buffer holding its value, and
 * a reference to a buffer holding its description. The bufffers are
 * contained in the fixed memory region, and free buffers are
 * contained in a free list (a ring buffer). The fixed memory contents
 * also contains certain configuration data used in protocol
 * processing, as well as certain other protocol runtime data
 * (e.g. statistics, vardis_isActive flag).
 *
 * The approach to store information on a per-variable-identifier
 * basis in an array means that the variable identifier space should
 * not be too large so as not to take up too much memory, so for now a
 * 16-bit restriction to the size of VarIdT has been added.
 */



namespace dcp::vardis {


  /**
   * @brief Contains all the relevant information for one single
   *        variable identifier
   */
  class IdentifierState {
  public:
    DBEntry             db_entry;              /*!< DBEntry for variable identifier */    
    std::atomic<bool>   used       =  false;   /*!< Whether or not identifier has been allocated */
    uint64_t            val_offs   =  0;       /*!< Offset (relative to start of memory block) for storing variable value */
    uint64_t            descr_offs =  0;       /*!< Offset (relative to start of memory block) for storing variable description */
    size_t              val_size   =  0;       /*!< Size of variable value (in bytes) */
    size_t              descr_size =  0;       /*!< Size of variable description (in bytes) */
  };


  /**
   * @brief Contains all non-per-identifier data stored in the fixed
   *        memory block that is useful for protocol processing
   *        (configuration data, statistics, vardis_isActive flag),
   *        but not including highly dynamic structures like the
   *        queues.
   */
  class GlobalStateBase {
  public:
    std::atomic<bool>  vardis_isActive = true;                     /*!< flag indicating whether Vardis protocol processing is active */
    uint16_t                   _conf_max_summaries          = 0;   /*!< Maximum number of VarSummT records included in Vardis payload */
    size_t                     _conf_max_description_length = 0;   /*!< Maximum length of variable description text */
    size_t                     _conf_max_value_length       = 0;   /*!< Maximum length of variable value */
    uint8_t                    _conf_max_repetitions        = 0;   /*!< Maximum allowed repCnt value for variables */
    NodeIdentifierT            _own_node_identifier;               /*!< ownNodeIdentifier */
    VardisProtocolStatistics   _vardis_stats;                      /*!< Vardis runtime statistics */
  };


  /**
   * @brief C++ concept allowing to check whether a class is derived
   *        from class GlobalStateBase
   */
  template <typename T>
  concept GlobalStateT = std::is_base_of<GlobalStateBase, T>::value;



  /**
   * @brief Template class for an array-based variable store.
   *
   * @tparam GlobalState: type parameter for the global state data
   *         (has to be derived from class GlobalStateBase)
   * @tparam valueBufferSize: maximum size of a buffer for storing
   *         a variable value. The actual buffer size will be a
   *         little larger to have some slack, and will be a multiple
   *         of sizeof(uint64_t)
   * @tparam descrBufferSize: same, but for variable description
   *
   * Note that the allocation of the actual memory block has to happen
   * outside of this class template (by a derived class).
   */
  template <GlobalStateT GlobalState, size_t valueBufferSize, size_t descrBufferSize>
  class ArrayVariableStoreBase : public VariableStoreI {
  public:

    /**
     * @brief Limit size of VarIdT space, and therefore memory
     *        consumption of the array-based variable store
     */
    static_assert (VarIdT::max_number_identifiers() <= (1<<16), "ArrayVariableStoreBase: identifier space too large");


    /**
     * @brief Returns the number of available identifiers
     */
    static constexpr uint64_t get_number_identifiers () { return VarIdT::max_number_identifiers(); };


    /**
     * @brief Returns the number of buffers being used (same for
     *        variable values and descriptions)
     */
    static constexpr uint64_t get_number_buffers () { return VarIdT::max_number_identifiers(); };


    /**
     * @brief Returns the value of the template parameter valueBufferSize
     */
    static constexpr size_t   get_value_buffer_size () { return valueBufferSize; };


    /**
     * @brief Returns the value of the template parameter descrBufferSize
     */

    static constexpr size_t   get_descr_buffer_size () { return descrBufferSize; };


    /**
     * @brief Returns the actual size of a value buffer, given by the
     *        valueBufferSize plus some additional slack. Returned
     *        size will be a multiple of sizeof(uint64_t).
     */
    static constexpr size_t   get_actual_value_buffer_size ()
    {
      const size_t s = sizeof (uint64_t);
      
      return s * ( (valueBufferSize + 2*s) / s);
    };


    /**
     * @brief Returns the actual size of a description buffer, given
     *        by the valueBufferSize plus some additional
     *        slack. Returned size will be a multiple of
     *        sizeof(uint64_t).
     */
    static constexpr size_t   get_actual_description_buffer_size ()
    {
      const size_t s = sizeof (uint64_t);
      
      return s * ( (descrBufferSize + 2*s) / s);
    };


    /**
     * @brief Shorthand for the exception type we use
     */
    typedef VardisStoreException VSE;

    
  protected:


    /**
     * @brief One entry of the free list
     *
     * The fixed memory block includes a free list, collecting
     * information about all the buffers that have not yet been
     * allocated to a variable identifier. The information about one
     * such buffer contains the memory offset (relative to start of
     * the memory block) for the variable value, and the memory offset
     * for the variable description.
     */
    typedef struct FreeListEntry {
      uint64_t  buffer_offs = 0;  /*!< Offset for variable value */
      uint64_t  descr_offs  = 0;  /*!< Offset for variable description */
    } FreeListEntry;


    /**
     * @brief This class specifies the actual structure that is stored
     *        in the given memory block.
     */
    class ArrayContents {
    public:

      /**
       * @brief Constructor, initializes free list
       */
      ArrayContents () : freeList ("ArrayContents::freeList", get_number_buffers()) {};


      /**
       * @brief Global state to be stored (of class GlobalStateBase or derived)
       */
      GlobalState global_state;

      /**
       * @brief Counts how many variables are currently registered
       */
      unsigned int number_current_variables = 0;
      
      /**
       * @brief Array containing all the per-identifier information
       */
      IdentifierState id_states [VarIdT::max_number_identifiers()];


      /**
       * @brief Free list for buffers
       */
      FixedMemRingBuffer<FreeListEntry, get_number_buffers()+1>  freeList;


      /**
       * @brief Actual buffer area for variable descriptions
       */
      char description_buffer [get_number_buffers() * get_actual_description_buffer_size()];


      /**
       * @brief Actual buffer for variable values
       */
      byte value_buffer [get_number_buffers() * get_actual_value_buffer_size()];
    };


    /**
     * @brief Pointer to the actual start address of the memory block
     *        (given by calling code)
     *
     * The ArrayContents structure will be instantiated at this
     * address. The given memory area needs to be large enough to
     * contain everything.
     */
    byte*            memory_start_address = nullptr;


    /**
     * @brief Points to same address as memory_start_address, just
     *        with the right type
     */
    ArrayContents*   pContents            = nullptr;

    
  public:

    // ---------------------------------------
    
    ArrayVariableStoreBase () {};


    // ---------------------------------------


    /**
     * @brief Initializes the array-based store, essentially
     *        establishes the ArrayContents structure at the given
     *        memory location and initializes free list for buffers.
     *
     * @param mem_start_addr: start address in memory for the memory
     *        block. The ArrayContents structure will be instantiated
     *        here
     * @param maxsumm: value of maxSummaries configuration parameter
     * @param maxdescrlen: value of maxDescriptionLength configuration parameter
     * @param maxvallen: value of maxValueLength configuration parameter
     * @param maxrep: value of maxRepetitions configuration parameter
     * @param own_node_id: ownNodeIdentifier value of the present node
     *
     * Instantiates ArrayContent structure at the given memory
     * location and stores the supplied configuration data. In
     * particular, sets the pContents and AC members that all other
     * methods rely on. User code *must* call this method shortly
     * after it has allocated the fixed memory block, as all the other
     * methods do not check the pContents parameter.
     *
     * Throws when some sanity checks (maxvallen and maxdescrlen)
     * fail, but does not sanity-check all parameters.
     */
    void initialize_array_store (byte* mem_start_addr,
				 uint16_t maxsumm,
				 size_t maxdescrlen,
				 size_t maxvallen,
				 uint8_t maxrep,
				 NodeIdentifierT own_node_id
				 )
    {
      memory_start_address = mem_start_addr;
      
      if (memory_start_address == nullptr)
	throw VSE ("initialize_array_store",
		   "memory start address is null");
      if (maxdescrlen >= descrBufferSize - sizeof (uint64_t))
	throw VSE ("initialize_array_store",
		   std::format("maximum description length {} is too large", maxdescrlen));
      if (maxvallen >= valueBufferSize - sizeof(uint64_t))
	throw VSE ("initialize_array_store",
		   std::format("maximum value length {} is too large", maxvallen));

      pContents = new (memory_start_address) ArrayContents;

      for (uint64_t i = 0; i < get_number_buffers(); i++)
	{
	  FreeListEntry entry;
	  entry.buffer_offs = i * get_actual_value_buffer_size();
	  entry.descr_offs  = i * get_actual_description_buffer_size();
	  pContents->freeList.push (entry);
	}

      pContents->global_state._conf_max_summaries = maxsumm;
      pContents->global_state._conf_max_description_length = maxdescrlen;
      pContents->global_state._conf_max_value_length = maxvallen;
      pContents->global_state._conf_max_repetitions = maxrep;
      pContents->global_state._own_node_identifier = own_node_id;
    };

    // ---------------------------------------

    /**
     * @brief Returns the actual size needed for the ArrayContents
     *        structure in the given memory block.
     */
    static constexpr size_t get_array_contents_size () { return sizeof(ArrayContents); };


    // ---------------------------------------


    /**
     * @brief Returns value of maxSummaries configuration parameter
     */
    virtual uint16_t get_conf_max_summaries () const { return pContents->global_state._conf_max_summaries; };


    /**
     * @brief Returns value of maxDescriptionLength configuration parameter
     */
    virtual size_t   get_conf_max_description_length () const { return pContents->global_state._conf_max_description_length; };


    /**
     * @brief Returns value of maxValueLength configuration parameter
     */
    virtual size_t   get_conf_max_value_length () const { return pContents->global_state._conf_max_value_length; };


    /**
     * @brief Returns value of maxRepetitions configuration parameter
     */
    virtual uint8_t  get_conf_max_repetitions () const { return pContents->global_state._conf_max_repetitions; };


    /**
     * @brief Returns current value of vardis_isActive flag
     */
    virtual bool     get_vardis_isactive () const { return pContents->global_state.vardis_isActive; };

    
    /**
     * @brief Sets current value of vardis_isActive flag
     */
    virtual void     set_vardis_isactive (bool active) { pContents->global_state.vardis_isActive = active; };


    /**
     * @brief Returns value of ownNodeIdentifier parameter
     */
    virtual NodeIdentifierT get_own_node_identifier () const { return pContents->global_state._own_node_identifier; };


    /**
     * @brief Returns current number of registered variables
     */
    virtual unsigned int get_number_variables () const
    {
      ArrayContents&  AC = *pContents;
      return AC.number_current_variables;
    };
    
    
    /**
     * @brief Returns reference to Vardis runtime statistics
     */
    virtual VardisProtocolStatistics& get_vardis_protocol_statistics_ref () const { return pContents->global_state._vardis_stats; };
    

    
    // ---------------------------------------


    /**
     * @brief Allocates a variable identifier.
     *
     * @param varId: variable identifier
     *
     * Allocates entry for variable identifier, including allocating a
     * buffer from the free list.  Does not set variable value,
     * description or anything from its DBEntry. Throws upon
     * irregularities (e.g. identifier already allocated, no free
     * buffer)
     */
    virtual void allocate_identifier (const VarIdT varId)
    {
      ArrayContents&  AC = *pContents;

      if (AC.id_states[varId.val].used)
	throw VSE ("allocate_identifier",
		   std::format("variable {} exists", (int) varId.val));
      if (AC.freeList.isEmpty())
	throw VSE ("allocate_identifier",
		   "no free buffer available");
    
      FreeListEntry fl_entry  = AC.freeList.pop();
      AC.id_states[varId.val].used        = true;
      AC.id_states[varId.val].val_offs    = fl_entry.buffer_offs;
      AC.id_states[varId.val].descr_offs  = fl_entry.descr_offs;
      AC.number_current_variables++;
    };


    /**
     * @brief Deallocates a variable identifier.
     *
     * @param varId: variable identifier
     *
     * Deallocates entry for variable identifier, including returning
     * its buffer into free list.  Does not modify variable value,
     * description or anything from its DBEntry. Throws upon
     * irregularities (e.g. identifier not allocated)
     */
    virtual void deallocate_identifier (const VarIdT varId)
    {
      ArrayContents&  AC = *pContents;

      if (not AC.id_states[varId.val].used)
	throw VSE ("deallocate_identifier",
		   std::format("unused varId {}", (int) varId.val));

      AC.id_states[varId.val].used = false;
      FreeListEntry fl_entry;
      fl_entry.buffer_offs = AC.id_states[varId.val].val_offs;
      fl_entry.descr_offs  = AC.id_states[varId.val].descr_offs;
      AC.id_states[varId.val].val_size   = 0;
      AC.id_states[varId.val].descr_size = 0;
      AC.freeList.push (fl_entry);
      AC.number_current_variables--;
    };


    /**
     * @brief Returns whether given variable identifier is allocated or not
     *
     * @param varId: variable identifier
     */
    virtual bool identifier_is_allocated (const VarIdT varId)
    {
      ArrayContents&  AC = *pContents;      
      return AC.id_states[varId.val].used;
    };
    
    // ---------------------------------------

    
    /**
     * @brief Sets all fields of DBEntry to contents of given entry,
     *        except description and value
     *
     * @param varId: variable identifier
     * @param new_entry: all fields
     *        of the entry for the variable identifier are set to the
     *        fields of new_entry
     *
     * Throws if variable is not allocated.
     */
    virtual void set_db_entry (const VarIdT varId, const DBEntry& new_entry)
    {
      ArrayContents&  AC = *pContents;

      if (not AC.id_states[varId.val].used)
	throw VSE ("set_db_entry",
		   std::format("unused varId {}", (int) varId.val));

      DBEntry& existing_entry = AC.id_states[varId.val].db_entry;
      existing_entry.varId         = varId;
      existing_entry.prodId        = new_entry.prodId;
      existing_entry.repCnt        = new_entry.repCnt;
      existing_entry.seqno         = new_entry.seqno;
      existing_entry.tStamp        = new_entry.tStamp;
      existing_entry.countUpdate   = new_entry.countUpdate;
      existing_entry.countCreate   = new_entry.countCreate;
      existing_entry.countDelete   = new_entry.countDelete;
      existing_entry.isDeleted     = new_entry.isDeleted;
    };
    

    /**
     * @brief Returns reference to the DBEntry record for given variable identifier
     *
     * @param varId: variable identifier
     *
     * Throws if variable is not allocated.
     */
    virtual DBEntry& get_db_entry_ref (const VarIdT varId) const
    {
      ArrayContents&  AC = *pContents;

      if (not AC.id_states[varId.val].used)
	throw VSE ("get_db_entry_ref",
		   std::format("unused varId {}", (int) varId.val));

      return AC.id_states[varId.val].db_entry;
    };
    
    // ---------------------------------------


    /**
     * @brief Updates variable value from given memory block
     *
     * @param varId: variable identifier
     * @param newval: pointer to memory from which to copy new variable value
     * @param nvsize: size of new variable value
     *
     * Throws upon irregularities (e.g. variable identifier not
     * allocated, check on value size failed, invalid buffer).
     */
    virtual void update_value (const VarIdT varId, byte* newval, VarLenT nvsize)
    {
      ArrayContents&  AC = *pContents;
      
      if (not AC.id_states[varId.val].used)
	throw VSE ("update_value", std::format("unused varId {}", (int) varId.val));
      if (nvsize.val == 0)
	throw VSE ("update_value", std::format("new value size is zero"));
      if (nvsize.val > valueBufferSize)
	throw VSE ("update_value", std::format("new value size {} is too large", (int) nvsize.val));
      if (newval == nullptr)
	throw VSE ("update_value", std::format("new value is null"));

      byte* effective_addr = AC.value_buffer + AC.id_states[varId.val].val_offs;
      std::memcpy (effective_addr, newval, nvsize.val);
      AC.id_states[varId.val].val_size = nvsize.val;
    };


    /**
     * @brief Updates variable value from given VarValueT record
     *
     * @param varId: variable identifier
     * @param newval: VarValueT record from which to copy variable value
     *
     * Throws upon irregularities (e.g. variable identifier not
     * allocated, check on value size failed).
     */
    virtual void update_value (const VarIdT varId, const VarValueT& newval)
    {
      ArrayContents&  AC = *pContents;
      
      if (not AC.id_states[varId.val].used)
	throw VSE ("update_value", std::format("unused varId {}", (int) varId.val));
      if (newval.length == 0)
	throw VSE ("update_value", std::format("new value size is zero"));
      if (newval.length > valueBufferSize)
	throw VSE ("update_value", std::format("new value size {} is too large", (int) newval.length));

      byte* effective_addr = AC.value_buffer + AC.id_states[varId.val].val_offs;
      std::memcpy (effective_addr, newval.data, newval.length);
      AC.id_states[varId.val].val_size = newval.length;
    };
    

    /**
     * @brief Copies variable value into given memory buffer, which
     *        must be large enough (not checked)
     *
     * @param varId: variable identifier
     * @param output_buffer: memory address into which to copy variable value
     * @param output_size: output parameter indicating size of variable value
     *
     * Throws upon irregularities (e.g. variable identifier not
     * allocated, invalid output_buffer).
     */
    virtual void read_value (const VarIdT varId, byte* output_buffer, VarLenT& output_size) const
    {
      ArrayContents&  AC = *pContents;

      if (not AC.id_states[varId.val].used)
	throw VSE ("read_value", std::format("unused varId {}", (int) varId.val));
      if (output_buffer == nullptr)
	throw VSE ("read_value", std::format("output buffer is null"));
            
      byte* effective_addr = AC.value_buffer + AC.id_states[varId.val].val_offs;
      std::memcpy (output_buffer, effective_addr, AC.id_states[varId.val].val_size);
      output_size = AC.id_states[varId.val].val_size;
    };


    /**
     * @brief Returns VarValueT containing the variable value for
     *        given variable identifier
     *
     * @param varId: variable identifier
     *
     * Throws upon irregularities (variable identifier not allocated).
     */
    virtual VarValueT read_value (const VarIdT varId) const
    {
      ArrayContents&  AC = *pContents;

      if (not AC.id_states[varId.val].used)
	throw VSE ("read_value", std::format("unused varId {}", (int) varId.val));

      byte* effective_addr = (byte*) AC.value_buffer + AC.id_states[varId.val].val_offs;
      VarValueT rv;
      rv.do_delete = false;
      rv.data      = effective_addr;
      rv.length    = AC.id_states[varId.val].val_size;
      rv.len       = AC.id_states[varId.val].val_size;

      return rv;
    };
    

    /**
     * @brief Returns size of variable value (independent of whether
     *        variable identifier is allocated or not)
     */
    virtual size_t size_of_value (const VarIdT varId) const
    {
      ArrayContents&  AC = *pContents;
      return AC.id_states[varId.val].val_size;
    };
    
    // ---------------------------------------


    /**
     * @brief Updates variable description from given StringT record
     *
     * @param varId: variable identifier
     * @param new_descr: StringT record from which to copy variable
     *        description
     *
     * Throws upon irregularities (e.g. variable identifier not
     * allocated, check on description size failed).
     */

    virtual void update_description (const VarIdT varId, const StringT& new_descr)
    {
      ArrayContents&  AC = *pContents;
      
      if (not AC.id_states[varId.val].used)
	throw VSE ("update_description", std::format("unused varId {}", (int) varId.val));
      if (new_descr.length == 0)
	throw VSE ("update_description", std::format("new description size is zero"));
      if (new_descr.length > descrBufferSize)
	throw VSE ("update_description", std::format("new description size {} is too large", (int) new_descr.length));

      byte* effective_addr = (byte*) AC.description_buffer + AC.id_states[varId.val].descr_offs;
      std::memcpy (effective_addr, new_descr.data, new_descr.length);
      AC.id_states[varId.val].descr_size = new_descr.length;
    };


    /**
     * @brief Returns StringT containing the variable description for
     *        given variable identifier
     *
     * @param varId: variable identifier
     *
     * Throws upon irregularities (variable identifier not allocated).
     */
    virtual StringT read_description (const VarIdT varId) const
    {
      ArrayContents&  AC = *pContents;

      if (not AC.id_states[varId.val].used)
	throw VSE ("read_description", std::format("unused varId {}", (int) varId.val));

      byte* effective_addr = (byte*) AC.description_buffer + AC.id_states[varId.val].descr_offs;
      StringT rv;
      rv.do_delete = false;
      rv.data      = effective_addr;
      rv.length    = AC.id_states[varId.val].descr_size;

      return rv;
    };


    /**
     * @brief Copies variable description as a zero-terminated string
     *        into given memory buffer (which must be large enough,
     *        not checked)
     *
     * @param varId: variable identifier
     * @param buf: memory address into which to copy variable description
     *
     * Throws upon irregularities (e.g. variable identifier not
     * allocated, invalid output_buffer).
     */
    virtual void read_description (const VarIdT varId, char* buf) const
    {
      ArrayContents&  AC = *pContents;
      
      if (not AC.id_states[varId.val].used)
	throw VSE ("read_description", std::format("unused varId {}", (int) varId.val));
      if (buf == nullptr)
	throw VSE ("read_description", std::format("empty buffer"));

      byte* effective_addr = (byte*) AC.description_buffer + AC.id_states[varId.val].descr_offs;
      std::memcpy (buf, effective_addr, AC.id_states[varId.val].descr_size);
      buf[AC.id_states[varId.val].descr_size] = 0;
    };
    

    /**
     * @brief Returns size of variable description (independent of
     *        whether variable identifier is allocated or not)
     */
    virtual size_t size_of_description (const VarIdT varId) const
    {
      ArrayContents&  AC = *pContents;
      return AC.id_states[varId.val].descr_size;
    };
    
  };


  
};  // namespace dcp::vardis
