/*
 * RTDBInformationElements.cc
 *
 *  Created on: Jul 13, 2022
 *      Author: spe107
 */

#include "RTDBInformationElements.h"

#include <cstdlib>
#include <new>
#include <cstring>

#define TYPE_LEN        1                     //width of the record type indicator
#define LEN_LEN         2                     //width of this record length
#define ID_LEN          sizeof(varID_t)       //width of the variable ID (default to 16 bits)
#define PRODUCER_ID_LEN MAC_ADDRESS_SIZE      //width of the producer ID field (MAC address)
#define SEQ_NO_LEN      sizeof(seqNo_t)       //Assume sequence no is 32 bits
#define DESCR_LEN_LEN   sizeof(varDescrLen_t) //width of the variable description length field
#define REP_CNT_LEN     sizeof(repCnt_t)      //Width of the variable repetition count field.
#define VAR_LEN_LEN     sizeof(varLen_t)      //Width of the variable length field

RTDBVarSummary::RTDBVarSummary(varID_t id, seqNo_t seqNo, repCnt_t repCnt) {
    this->varID = id;
    this->varSeqNo = seqNo;
    this->remainingRepetitions = repCnt;
}

RTDBVarSummary::RTDBVarSummary(RTDBVarSummary*& v) {
    this->varID = v->varID;
    this->varSeqNo = v->varSeqNo;
    this->remainingRepetitions = v->remainingRepetitions;
}

int RTDBVarSummary::getIETypeLen(void) {
    return ID_LEN + SEQ_NO_LEN;
}

inet::Ptr<VarDisSummary> RTDBVarSummary::getPacketElement(void) {
    auto sum = inet::makeShared<VarDisSummary>();
    sum->setVarID(varID);
    sum->setVarSeqNo(varSeqNo);
    sum->setChunkLength(inet::B(getIETypeLen()));
    return sum;
}

RTDBVarUpdate::RTDBVarUpdate(varID_t id, seqNo_t seqNo, varLen_t len, uint8_t* buf, repCnt_t repCnt) {
    this->varID = id;
    this->varSeqNo = seqNo;
    this->remainingRepetitions = repCnt;
    this->varLen = len;
    this->varBuf = buf;
}

RTDBVarUpdate::RTDBVarUpdate(RTDBVarUpdate*& v) {
    this->varID = v->varID;
    this->varSeqNo = v->varSeqNo;
    this->remainingRepetitions = v->remainingRepetitions;
    this->varLen = v->varLen;

    this->varBuf = new uint8_t[this->varLen];
    memcpy(this->varBuf, v->varBuf, this->varLen);
}

RTDBVarUpdate::~RTDBVarUpdate() {
    delete[] this->varBuf;
}

int RTDBVarUpdate::getIETypeLen(void) {
    return ID_LEN + SEQ_NO_LEN + VAR_LEN_LEN + this->varLen;
}

inet::Ptr<VarDisUpdate> RTDBVarUpdate::getPacketElement(void) {
    auto upd = inet::makeShared<VarDisUpdate>();
    upd->setVarID(varID);
    upd->setDataLen(varLen);
    upd->getVarDataForUpdate().copyFromBuffer(varBuf, varLen);
    upd->setVarSeqNo(varSeqNo);
    upd->setChunkLength(inet::B(getIETypeLen()));
    return upd;
}

RTDBVarSpec::RTDBVarSpec(varID_t id, inet::MacAddress producerID, seqNo_t seqNo, varDescrLen_t descrLen, uint8_t* descrBuf, repCnt_t repCnt) {
    this->varID = id;
    this->producerID = inet::MacAddress(producerID);
    this->varSeqNo = seqNo;
    this->remainingRepetitions = repCnt;
    this->varDescrLen = descrLen;
    this->varDescrBuf = descrBuf;
    this->repCnt = repCnt;
}

RTDBVarSpec::RTDBVarSpec(RTDBVarSpec*& v) {
    this->varID = v->varID;
    this->producerID = v->producerID;
    this->varSeqNo = v->varSeqNo;
    this->remainingRepetitions = v->remainingRepetitions;
    this->varDescrLen = v->varDescrLen;

    this->varDescrBuf = new uint8_t[this->varDescrLen];
    memcpy(this->varDescrBuf, v->varDescrBuf, this->varDescrLen);

    this->repCnt = v->repCnt;
}

RTDBVarSpec::~RTDBVarSpec() {
    delete[] this->varDescrBuf;
}

int RTDBVarSpec::getIETypeLen(void) {
    return ID_LEN + PRODUCER_ID_LEN + DESCR_LEN_LEN + this->varDescrLen + REP_CNT_LEN;
}

inet::Ptr<VarDisSpecification> RTDBVarSpec::getPacketElement(void) {
    auto spec = inet::makeShared<VarDisSpecification>();
    spec->setVarID(varID);
    spec->setProducer(producerID);
    spec->setVarRepCnt(repCnt);
    spec->setVarDescrLen(varDescrLen);
    spec->getVarDescrForUpdate().copyFromBuffer(varDescrBuf, varDescrLen);
    spec->setChunkLength(inet::B(getIETypeLen()));
    return spec;
}

RTDBVarCreate::RTDBVarCreate(RTDBVarSpec* spec, RTDBVarUpdate* update) {
    this->varSpec = spec;
    this->varUpdate = update;
    this->remainingRepetitions = spec->repCnt;
}

RTDBVarCreate::RTDBVarCreate(RTDBVarCreate*& v) {
    this->varSpec = new RTDBVarSpec(v->varSpec);
    this->varUpdate = new RTDBVarUpdate(v->varUpdate);
    this->remainingRepetitions = v->remainingRepetitions;
}

RTDBVarCreate::~RTDBVarCreate() {
    delete this->varSpec;
    delete this->varUpdate;
}

int RTDBVarCreate::getIETypeLen(void) {
    return this->varSpec->getIETypeLen() + this->varUpdate->getIETypeLen();
}

inet::Ptr<VarDisCreate> RTDBVarCreate::getPacketElement(void) {
    auto spec = *varSpec->getPacketElement();
    auto upd = *varUpdate->getPacketElement();

    auto create = inet::makeShared<VarDisCreate>();
    create->setSpec(spec);
    create->setUpdate(upd);
    create->setChunkLength(inet::B(getIETypeLen()));

    return create;
}

RTDBVarReqUpdate::RTDBVarReqUpdate(varID_t id, seqNo_t seqNo, repCnt_t repCnt) {
    this->varID = id;
    this->varSeqNo = seqNo;
    this->remainingRepetitions = repCnt;
}

RTDBVarReqUpdate::RTDBVarReqUpdate(RTDBVarReqUpdate*& v) {
    this->varID = v->varID;
    this->varSeqNo = v->varSeqNo;
    this->remainingRepetitions = v->remainingRepetitions;
}

int RTDBVarReqUpdate::getIETypeLen(void) {
    return ID_LEN + SEQ_NO_LEN;
}

inet::Ptr<VarDisReqUpdate> RTDBVarReqUpdate::getPacketElement(void) {
    auto sum = inet::makeShared<VarDisReqUpdate>();
    sum->setVarID(varID);
    sum->setVarSeqNo(varSeqNo);
    sum->setChunkLength(inet::B(getIETypeLen()));
    return sum;
}

RTDBVarReqCreate::RTDBVarReqCreate(varID_t id, repCnt_t repCnt) {
    this->varID = id;
    this->remainingRepetitions = repCnt;
}

RTDBVarReqCreate::RTDBVarReqCreate(RTDBVarReqCreate*& v) {
    this->varID = v->varID;
    this->remainingRepetitions = v->remainingRepetitions;
}

int RTDBVarReqCreate::getIETypeLen(void) {
    return ID_LEN;
}

inet::Ptr<VarDisReqCreate> RTDBVarReqCreate::getPacketElement(void) {
    auto sum = inet::makeShared<VarDisReqCreate>();
    sum->setVarID(varID);
    sum->setChunkLength(inet::B(getIETypeLen()));
    return sum;
}

RTDBVarDelete::RTDBVarDelete(varID_t id, repCnt_t repCnt) {
    this->varID = id;
    this->remainingRepetitions = repCnt;
}

RTDBVarDelete::RTDBVarDelete(RTDBVarDelete*& v) {
    this->varID = v->varID;
    this->remainingRepetitions = v->remainingRepetitions;
}

int RTDBVarDelete::getIETypeLen(void) {
    return ID_LEN;
}

inet::Ptr<VarDisDelete> RTDBVarDelete::getPacketElement(void) {
    auto sum = inet::makeShared<VarDisDelete>();
    sum->setVarID(varID);
    sum->setChunkLength(inet::B(getIETypeLen()));
    return sum;
}
