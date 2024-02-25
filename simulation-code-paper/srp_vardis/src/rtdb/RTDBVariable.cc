/*
 * RTDBVariable.cpp
 *
 *  Created on: Jul 12, 2022
 *      Author: spe107
 */

#include "RTDBVariable.h"
#include <cstdlib>
#include <cstring>
#include <new>

RTDBVariable::RTDBVariable(inet::MacAddress producerID, varID_t varID,
                           repCnt_t repCnt, varDescrLen_t descrLen,
                           uint8_t* descr, varLen_t len, uint8_t* buf,
                           seqNo_t seqNo) {
    this->varProducer = producerID;
    this->varID = varID;
    this->varRepCnt = repCnt;
    this->varDescrLen = descrLen;
    this->varLen = len;
    this->varSeqNo = seqNo;

    this->varDescr = new uint8_t[descrLen];
    memcpy(this->varDescr, descr, descrLen);

    this->varBuffer = new uint8_t[len];
    memcpy(this->varBuffer, buf, len);

    this->lastUpdateTime = omnetpp::simTime();
    this->toBeDeleted = false;
}

RTDBVariable::~RTDBVariable() {
    delete[] this->varDescr;
    delete[] this->varBuffer;
}

varID_t RTDBVariable::getVarID(void) {
    return this->varID;
}

varLen_t RTDBVariable::getVarLen(void) {
    return this->varLen;
}

varDescrLen_t RTDBVariable::getDescrLen(void) {
    return this->varDescrLen;
}

seqNo_t RTDBVariable::getSeqNo(void) {
    return this->varSeqNo;
}


uint8_t* RTDBVariable::getDescription(void) {
    uint8_t* ret = new uint8_t[this->varDescrLen];
    memcpy(ret, this->varDescr, this->varDescrLen);
    return ret;
}

repCnt_t RTDBVariable::getRepetitionCount(void) {
    return this->varRepCnt;
}

uint8_t* RTDBVariable::getVar(void) {
    uint8_t* ret = new uint8_t[this->varLen];
    memcpy(ret, this->varBuffer, this->varLen);
    return ret;
}

void RTDBVariable::update(uint8_t* buf, varLen_t len) {
    delete[] this->varBuffer;

    this->varBuffer = new uint8_t[len];
    memcpy(this->varBuffer, buf, len);
    this->varLen = len;

    this->varSeqNo++;
    if (this->varSeqNo == 0) {
        //Do not allow sequence number to be 0 as it can cause issues
        //determining recency after sequence number roll overs.
        this->varSeqNo++;
    }
    this->lastUpdateTime = omnetpp::simTime();
}

void RTDBVariable::update(uint8_t* buf, varLen_t len, seqNo_t seqNo) {
    delete[] this->varBuffer;

    this->varBuffer = new uint8_t[len];
    memcpy(this->varBuffer, buf, len);
    this->varLen = len;

    this->varSeqNo = seqNo;
    this->lastUpdateTime = omnetpp::simTime();
}

bool RTDBVariable::isForDeletion(void) {
    return this->toBeDeleted;
}

void RTDBVariable::markForDeletion(void) {
    this->toBeDeleted = true;
}

inet::MacAddress RTDBVariable::getProducer(void) {
    return this->varProducer;
}

omnetpp::simtime_t RTDBVariable::getDataTimestamp(void) {
    return this->lastUpdateTime;
}

RTDBVarSummary* RTDBVariable::getSummary(void) {
    return new RTDBVarSummary(this->varID, this->varSeqNo, this->varRepCnt);
}
