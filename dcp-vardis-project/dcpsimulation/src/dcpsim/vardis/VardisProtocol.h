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

#include <queue>
#include <omnetpp.h>
#include <inet/common/InitStages.h>
#include <dcp/common/area.h>
#include <dcp/common/services_status.h>
#include <dcp/vardis/vardis_protocol_data.h>
#include <dcp/vardis/vardis_rtdb_entry.h>
#include <dcp/vardis/vardis_store_array_inmemory.h>
#include <dcp/vardis/vardis_transmissible_types.h>
#include <dcpsim/bp/BPClientProtocol.h>
#include <dcpsim/bp/BPReceivePayload_m.h>
#include <dcpsim/bp/BPPayloadTransmitted_m.h>
#include <dcpsim/bp/BPQueryNumberBufferedPayloads_m.h>
#include <dcpsim/vardis/VardisRTDBConfirmation_m.h>
#include <dcpsim/vardis/VardisRTDBCreate_m.h>
#include <dcpsim/vardis/VardisRTDBDelete_m.h>
#include <dcpsim/vardis/VardisRTDBDescribeDatabase_m.h>
#include <dcpsim/vardis/VardisRTDBDescribeVariable_m.h>
#include <dcpsim/vardis/VardisRTDBRead_m.h>
#include <dcpsim/vardis/VardisRTDBUpdate_m.h>

using namespace omnetpp;
using namespace inet;

// ------------------------------------------------------------------------

namespace dcp::vardis {

  /**
   * @brief This module implements the VarDis (or Vardis) protocol as
   *        a BP client protocol, generally (but not in all detail)
   *        following the VarDis specification document. Broadly, it
   *        constructs outgoing Vardis payloads and processes incoming
   *        Vardis payloads. The actual protocol behaviour is drawn in
   *        from the DCP implementation.
   */
  
  
  class VardisProtocol : public BPClientProtocol
  {
  public:

    /**
     * @brief Destructor, cancels and deletes self messages
     */
    virtual ~VardisProtocol();
    
    virtual int numInitStages() const override { return NUM_INIT_STAGES; };


    /**
     * @brief Initialization, reads and checks module parameters, gate
     *        identifiers, and registers protocol / service with
     *        relevant INET multiplexers, starts required self
     *        messages
     */
    virtual void initialize(int stage) override;


    /**
     * @brief Top-level message dispatcher
     */
    virtual void handleMessage (cMessage* msg) override;


    /**
     * @brief Sends message to BP to register Vardis protocol
     */
    virtual void registerAsBPClient(void) override;


    /**
     * @brief Processing confirmation of BP registration, creates
     *        Vardis variable store and initializes Vardis protocol
     *        data (both drawn in from the DCP implementation)
     */
    virtual bool handleBPRegisterProtocol_Confirm (BPRegisterProtocol_Confirm* pConf);
    
  protected:
    
    // -------------------------------------------
    // data members
    // -------------------------------------------
    
    
    // Module parameters
    size_t         vardisMaxValueLength;          /*!< maximum length of a variable value in bytes */
    size_t         vardisMaxDescriptionLength;    /*!< maximum length of variable description text in bytes */
    uint8_t        vardisMaxRepetitions;          /*!< maximum allowed repCnt for a variable */
    uint8_t        vardisMaxSummaries;            /*!< maximum number of summaries included in a payload */
    double         vardisBufferCheckPeriod;       /*!< how often is buffer occupancy of BP checked */
    
    // Gate identifiers
    int   gidFromApplication, gidToApplication;   /*!< OMNeT++ gate identifiers */
  
    // Timer self messages
    cMessage*     bufferCheckMsg;  /*!< Self-message for periodic check of BP buffer occupancy (in response, generate payload if buffer is empty) */
    cMessage*     sendPayloadMsg;  /*!< Self message for generating and sending payload to BP (shortly before beacon construction) */
    

    /**
     * @brief Points to the Vardis variable store, taken from DCP implementation
     */
    std::unique_ptr<VardisVariableStoreInMemory> vardis_store_p;


    /**
     * @brief Points to the VardisProtocolData object, containing key
     *        runtime data for Vardis protocol and performing all
     *        protocol actions
     */
    std::unique_ptr<VardisProtocolData> vardis_protocol_data_p;
  

    // other data members
    bool   payloadSent = false;  /*!< Helps prevent generation of payloads if there is already one in the BP buffer */

protected:


    // ---------------------------------------------------------------------
    // top-level message handlers
    // ---------------------------------------------------------------------

    /**
     * @brief Queries buffer occupancy from BP, schedules next buffer
     *        check self-message
     */
    virtual void handleBufferCheckMsg ();

    
    /**
     * @brief Triggers generation of a Vardis payload and hands it
     *        over to BP
     */
    virtual void handleSendPayloadMsg ();

    
    /**
     * @brief Second-level message dispatcher for any message coming
     *        from Vardis applications
     *
     * @param msg: the message to handle
     */
    virtual void handleApplicationMessage(cMessage* msg);

    
    /**
     * @brief Second-level message dispatcher for any message coming
     *        from BP
     *
     * @param msg: the message to handle
     */
    virtual void handleBPMessage(cMessage* msg);


    // ---------------------------------------------------------------------
    // individual message handlers for messages from BP
    // ---------------------------------------------------------------------

    /**
     * @brief Processes BPPayloadTransmitted.indication (checks
     *        outcome) and schedules next payload generation time to
     *        happen shortly before BP generates next beacon
     *
     * @param ptInd: the BP payload transmitted indication message to
     *        handle
     */
    virtual void handleBPPayloadTransmittedIndication (BPPayloadTransmitted_Indication* ptInd);

    
    /**
     * @brief Processes BPQueryNumberBufferedPayloads.confirm message,
     *        by which BP reports buffer occupancy. If buffer is
     *        empty, trigger generation of a Vardis payload for
     *        transmission
     *
     * @param confMsg: the confirmation message to handle
     */
    virtual void handleBPQueryNumberBufferedPayloadsConfirm (BPQueryNumberBufferedPayloads_Confirm* confMsg);

    
    /**
     * @brief Processes BPReceivedPayload.indication
     *        message. Top-level method for processing incoming
     *        payloads
     *
     * @param payload: the received payload to handle
     */
    virtual void handleBPReceivedPayloadIndication (BPReceivePayload_Indication* payload);


    // ---------------------------------------------------------------------
    // individual message handlers for messages from applications
    // ---------------------------------------------------------------------

    /**
     * @brief Processes an incoming RTDBCreate.request message. Checks
     *        its validity, creates entry into RTDB, adds variable for
     *        transmission of VarCreate to neighbors and generates
     *        confirmation message for application.
     *
     * @param createReq: the create request message to handle
     */
    virtual void handleRTDBCreateRequest (RTDBCreate_Request* createReq);

    
    /**
     * @brief Processes an incoming RTDBUpdate.request message. Checks
     *        its validity, updates RTDB entry, adds variable for
     *        transmission of VarUpdate to neighbors, and generates
     *        confirmation message for application.
     *
     * @param updateReq: the update request message to handle
     */
    virtual void handleRTDBUpdateRequest (RTDBUpdate_Request* updateReq);

    
    /**
     * @brief Processes an incoming RTDBRead.request message. Checks
     *        its validity, retrieves variable variable and returns
     *        confirmation message to application that includes this
     *        value.
     *
     * @param readReq: the read request message to handle
     */
    virtual void handleRTDBReadRequest (RTDBRead_Request* readReq);

    
    /**
     * @brief Processes an incoming RTDBDelete.request message. Checks
     *        its validity, adds variable for transmission of
     *        VarDelete to neighbors, and generates confirmation
     *        message for application
     *
     * @param delReq: the delete request message to handle
     */
    virtual void handleRTDBDeleteRequest (RTDBDelete_Request* delReq);


    /**
     * @brief Processes RTDBDescribeDatabase.request
     *        message. Generates and sends a confirmation that
     *        contains a short summary description for each currently
     *        known variable in the RTDB
     *
     * @param descrDbReq: the describe-database request message to
     *        handle
     */
    virtual void handleRTDBDescribeDatabaseRequest (RTDBDescribeDatabase_Request* descrDbReq);

    
    /**
     * @brief Processes RTDBDescribeVariable.request
     *        message. Generates and sends a confirmation that
     *        contains detailed information about the requested
     *        variable (value and metadata)
     *
     * @param descrVarReq: the describe-variable request message to
     *        handle
     */
    virtual void handleRTDBDescribeVariableRequest (RTDBDescribeVariable_Request* descrVarReq);
    
    
    // ---------------------------------------------------------------------
    // Construction of outgoing Vardis payloads
    // ---------------------------------------------------------------------


    /**
     * @brief Almost top-level method to construct the overall Vardis
     *        payload as a byte vector (called from
     *        'generatePayload'). Inserts the full instruction
     *        containers in the order given in the protocol
     *        specification
     *
     * @param bv: output parameter containing target byte vector into
     *        which to construct the payload
     * @param containers_added: output parameter indicating the number
     *        of instruction containers added to the payload
     */
    void constructPayload (bytevect& bv, unsigned int& containers_added);
    

    
    /**
     * @brief This top-level method constructs a Vardis payload as a
     *        byte vector and hands it over to BP for transmission.
     */
    void generatePayload();


    // ---------------------------------------------------------------------
    // Helpers for deconstructing and processing received packets
    // ---------------------------------------------------------------------
    
    
    /**
     * @brief These methods iterate over the records of a received
     *        instruction container (given as a list) and call the
     *        'processX' methods (from the VardisProtocolData type of
     *        the DCP implementation) to process individual records
     */
    void processVarCreateList(const std::deque<VarCreateT>& creates);
    void processVarDeleteList(const std::deque<VarDeleteT>& deletes);
    void processVarUpdateList(const std::deque<VarUpdateT>& updates);
    void processVarSummaryList(const std::deque<VarSummT>& summs);
    void processVarReqUpdateList(const std::deque<VarReqUpdateT>& requpdates);
    void processVarReqCreateList(const std::deque<VarReqCreateT>& reqcreates);
    

    /**
     * @brief Extracts / parses an entire instruction container (its
     *        instruction records, which are stored in a list)
     *
     * @tparam T: data type of instruction records
     * @param area: disassembly area to extract records from, will be
     *        modified
     * @param icHeader: header of instruction container
     * @param result_list: accumulates the deserialized instruction
     *        records
     */
    template <typename T>
    void extractInstructionContainerElements (DisassemblyArea& area, const ICHeaderT& icHeader, std::deque<T>& result_list)
    {
      dbg_enter ("extractInstructionContainerElements");
      if (icHeader.icNumRecords == 0)
	{
	  EV << "extractInstructionContainerElements: number of records is zero";
	  error ("extractInstructionContainerElements: number of records is zero");
	}
      
      for (int i=0; i<icHeader.icNumRecords; i++)
	{
	  T element;
	  element.deserialize (area);
	  result_list.push_back(element);
	}
      dbg_leave();
    };
    
    
    // ---------------------------------------------------------------------
    // Helpers for sending standard confirmations to higher layers
    // ---------------------------------------------------------------------
    
    /**
     * @brief Sends the given confirmation with given return status to
     *        the indicated application through INET message
     *        dispatcher
     *
     * @param confMsg: the confirmation message to send
     * @param status: the status code to set in the confirmation message
     * @param theProtocol: the OMNeT++/INET protocol object to send
     *        the message to (through message dispatcher)
     */
    void sendConfirmation(VardisConfirmation *confMsg, DcpStatus status, Protocol* theProtocol);

    
    /**
     * @brief Creates RTDBCreate.confirm message with given status and
     *        hands it over to 'sendConfirmation' for sending to given
     *        protocol
     *
     * @param status: the status code to insert into confirmation message
     * @param varId: the variable identifier this message relates to
     * @param theProtocol: the OMNeT++/INET protocol object to send
     *        the message to (through message dispatcher)     */
    void sendRTDBCreateConfirm (DcpStatus status, VarIdT varId, Protocol* theProtocol);

    
    /**
     * @brief Creates RTDBUpdate.confirm message
     *
     * @param status: the status code to insert into confirmation message
     * @param varId: the variable identifier this message relates to
     * @param theProtocol: the OMNeT++/INET protocol object to send
     *        the message to (through message dispatcher)     */
    void sendRTDBUpdateConfirm (DcpStatus status, VarIdT varId, Protocol* theProtocol);



    // ---------------------------------------------------------------------
    // Miscellaneous helpers
    // ---------------------------------------------------------------------
    
    /**
     * @brief Retrieves the sender protocol from an incoming message,
     *        throws error if none is found
     *
     * @param message: the message under consideration
     */
    Protocol* fetchSenderProtocol(Message* message);

    
    /**
     * @brief Checks if the given variable exists in the RTDB
     *
     * @param varId: identifier of the variable to check for
     */
    bool variableExists(VarIdT varId);

    
    /**
     * @brief Checks if this node is itself the producer of the given
     *        variable
     *
     * @param varId: identifier of the variable to check
     */
    bool producerIsMe(VarIdT varId);
    

    /**
     * @brief Returns whether Vardis protocol is currently active or
     *        not
     */
    inline bool vardisActive () const {return vardis_store_p->get_vardis_isactive();};
    
};

} // namespace

