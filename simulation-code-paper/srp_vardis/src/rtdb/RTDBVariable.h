/*
 * RTDBVariable.h
 *
 *  Created on: Jul 12, 2022
 *      Author: spe107
 */

#ifndef RTDBVARIABLE_H_
#define RTDBVARIABLE_H_

#include "../srp_vardis_config.h"

#include <omnetpp.h>
#include <inet/linklayer/common/MacAddress.h>
#include "RTDBInformationElements.h"

class RTDBVariable {
public:
    RTDBVariable(inet::MacAddress producerID, varID_t varID, repCnt_t repCnt,
                 varDescrLen_t descrLen, uint8_t* descr, varLen_t len,
                 uint8_t* buf, seqNo_t seqNo);
    virtual ~RTDBVariable();

    varID_t getVarID(void);
    varLen_t getVarLen(void);
    repCnt_t getRepetitionCount(void);
    seqNo_t getSeqNo(void);
    bool isForDeletion(void);
    void markForDeletion(void);
    inet::MacAddress getProducer(void);
    omnetpp::simtime_t getDataTimestamp(void);

    //Return a pointer to a new copy of the variable data. You are responsible
    //for freeing this memory.
    uint8_t* getVar(void);

    //Returns a pointer to the description of the object. You are responsible
    //for freeing this memory.
    uint8_t* getDescription(void);
    varDescrLen_t getDescrLen(void);

    //Update the variable.
    void update(uint8_t* buf, varLen_t len);
    void update(uint8_t* buf, varLen_t len, seqNo_t seqNo);

    //Returns a new summary variable. You are responsible for freeing this.
    RTDBVarSummary* getSummary(void);

private:
    inet::MacAddress varProducer;
    varID_t varID;
    varLen_t varLen;
    varDescrLen_t varDescrLen;
    uint8_t* varDescr;
    repCnt_t varRepCnt;
    uint8_t* varBuffer;
    seqNo_t varSeqNo;
    bool toBeDeleted = false;
    omnetpp::simtime_t lastUpdateTime;

};

#endif /* RTDBVARIABLE_H_ */
