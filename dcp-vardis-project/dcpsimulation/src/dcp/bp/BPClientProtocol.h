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

#ifndef DCP_BP_BPCLIENTPROTOCOL_H_
#define DCP_BP_BPCLIENTPROTOCOL_H_

#include <inet/common/packet/Message.h>
#include <inet/common/packet/Packet.h>
#include <dcp/common/DcpProtocol.h>
#include <dcp/common/DcpTypesGlobals.h>
#include <dcp/bp/BPQueueingMode_m.h>
#include <dcp/bp/BPRegisterProtocol_m.h>
#include <dcp/bp/BPDeregisterProtocol_m.h>
#include <dcp/bp/BPConfirmation_m.h>

// --------------------------------------------------------------------

namespace dcp {


/**
 * This module implements basic functionalities that any client protocol
 * of the BP should have, and all client protocols (e.g. VarDis, SRP)
 * should inherit from this class. This class contains logic for registration
 * and deregistration of the client protocol, provides methods to send
 * packets or messages to the BP, and provides handlers for confirmation
 * messages coming from the BP. It also holds the information about the
 * maximum payload size for the client protocol.
 *
 * This is an abstract base class.
 */

class BPClientProtocol : public DcpProtocol {

public:
    virtual ~BPClientProtocol();
    virtual void initialize(int stage) override;

    bool isSuccessfullyRegisteredWithBP () {return _successfullyRegistered;};

private:

    // data members for keeping track of registration status
    bool        _registrationRequested  = false;
    bool        _successfullyRegistered = false;
    cMessage   *_registerMsg = nullptr;

protected:

    // --------------------------------------------
    // data members
    // --------------------------------------------

    // Parameters
    BPLengthT  maxPayloadSize;

    // gate identifiers for communication with BP
    int gidFromBP = -1;
    int gidToBP = -1;


    // --------------------------------------------
    // method to override
    // --------------------------------------------

    /**
     * Derived class needs to override this method. Nothing more needs to be done
     * than calling 'sendRegisterProtocolRequest' with the right data for the
     * client protocol to be registered.
     */
    virtual void registerAsBPClient(void) = 0;


    // --------------------------------------------
    // helpers
    // --------------------------------------------


    /**
     * This method tests an incoming message whether it is related to the registration
     * procedure. If so, the message is processed and deleted, otherwise it will remain
     * unmodified.
     *
     * This method should be called (directly or indirectly) by the 'handleMessage'
     * method of any client protocol.
     */
    bool hasHandledMessageBPClient(cMessage* msg);


    /**
     * Sending packets or messages down to the BP. A client protocol preparing
     * a payload for transmission will use these to hand over the finished
     * payload.
     */
    void sendToBP (Message* message);
    void sendToBP (Packet* packet);

    /**
     * Helper messages to send client protocol registration message,
     * the protocol is specified through parameters. Derived classes
     * call this method in their overloaded version of 'registerAsBPClient'.
     */
    void sendRegisterProtocolRequest (BPProtocolIdT protId,
                                      std::string protName,
                                      BPLengthT maxPayloadLen,
                                      BPQueueingMode queueingMode,
                                      unsigned int maxEntries);

    /**
     * Sends a request to deregister the client protocol to the BP.
     */
    void sendDeregisterProtocolRequest (BPProtocolIdT protId);


    /**
     * Simple default handlers for confirmation messages sent by the BP
     * concerning registration and deregistration.
     * handleStatus just prints the returned status value as debug output
     * The other methods call handleStatus (and are in themselves called
     * when a confirmation is to be processed) and return whether or not
     * the confirmation status was BP_STATUS_OK (return value of true).
     */
    void handleStatus (BPStatus status);
    virtual bool handleBPRegisterProtocol_Confirm (BPRegisterProtocol_Confirm *pConf);
    virtual bool handleBPDeregisterProtocol_Confirm (BPDeregisterProtocol_Confirm *pConf);
    virtual bool handleBPTransmitPayload_Confirm (BPTransmitPayload_Confirm *pConf);
};

} // namespace

#endif /* DCP_BP_BPCLIENTPROTOCOL_H_ */
