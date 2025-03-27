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

#include <queue>
#include <iostream>
#include <memory>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/memblock.h>
#include <dcp/common/sharedmem_structure_base.h>
#include <dcp/vardis/vardis_shm_control_segment.h>

using std::ostream;

/**
 * @brief The class VardisClientProtocolData holds the data that the
 *        Vardis demon maintains about a client protocol / application
 */


namespace dcp::vardis {


  /**
   * @brief Holds the data that the Vardis demon maintains about a
   *        client protocol or application
   */
  typedef struct VardisClientProtocolData {

    /**************************************************************
     * Main entries required for core BP operation
     *************************************************************/
    
    /**
     * @brief Textual name of client protocol
     */
    std::string       clientName;


    /**************************************************************
     * Entries for inter-process communication with client protocol
     *************************************************************/


    /**
     * @brief Pointer to the shared memory area descriptor between
     *        Vardis demon and the client
     */
    std::shared_ptr<ShmStructureBase> pSSB;


    /**
     * @brief Pointer to the actual shared memory area for
     *        client. Valid after successful registration
     */
    VardisShmControlSegment*          pSCS = nullptr;
    


    VardisClientProtocolData () {};

    VardisClientProtocolData (const char* area_name);

    
    /**************************************************************
     * Auxiliary methods
     *************************************************************/
    
    friend ostream& operator<<(ostream& os, const VardisClientProtocolData& client);
    
  } VardisClientProtocolData;
  
};  // namespace dcp::vardis
