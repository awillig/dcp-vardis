/*
 * RTDBInformationElements.h
 *
 *  Created on: Jul 13, 2022
 *      Author: spe107
 */

#ifndef RTDBINFORMATIONELEMENTS_H_
#define RTDBINFORMATIONELEMENTS_H_

#include "../srp_vardis_config.h"

#include "../messages/vardis_types/VarDisUpdate_m.h"
#include "../messages/vardis_types/VarDisCreate_m.h"
#include "../messages/vardis_types/VarDisSpecification_m.h"
#include "../messages/vardis_types/VarDisSummary_m.h"
#include "../messages/vardis_types/VarDisReqUpdate_m.h"
#include "../messages/vardis_types/VarDisReqCreate_m.h"
#include "../messages/vardis_types/VarDisDelete_m.h"
#include <inet/common/Ptr.h>

class RTDBVarSummary {
public:
    RTDBVarSummary(varID_t id, seqNo_t seqNo, repCnt_t repCnt);
    RTDBVarSummary(RTDBVarSummary*& v);
    varID_t varID;
    seqNo_t varSeqNo;
    int remainingRepetitions;
    int getIETypeLen(void);
    inet::Ptr<VarDisSummary> getPacketElement(void);
};

class RTDBVarUpdate {
public:
    RTDBVarUpdate(varID_t id, seqNo_t seqNo, varLen_t len, uint8_t* buf, repCnt_t repCnt);
    RTDBVarUpdate(RTDBVarUpdate*& v);
    ~RTDBVarUpdate();
    varID_t varID;
    seqNo_t varSeqNo;
    varLen_t varLen;
    uint8_t* varBuf;
    int remainingRepetitions;
    int getIETypeLen(void);
    inet::Ptr<VarDisUpdate> getPacketElement(void);
};

class RTDBVarSpec {
public:
    RTDBVarSpec(varID_t id, inet::MacAddress producer, seqNo_t seqNo, varDescrLen_t descrLen, uint8_t* descrBuf, repCnt_t repCnt);
    RTDBVarSpec(RTDBVarSpec*& v);
    ~RTDBVarSpec();
    varID_t varID;
    inet::MacAddress producerID;
    seqNo_t varSeqNo;
    varDescrLen_t varDescrLen;
    uint8_t* varDescrBuf;
    uint8_t repCnt;
    int remainingRepetitions;
    int getIETypeLen(void);
    inet::Ptr<VarDisSpecification> getPacketElement(void);
};

class RTDBVarCreate {
public:
    RTDBVarCreate(RTDBVarSpec* spec, RTDBVarUpdate* update);
    RTDBVarCreate(RTDBVarCreate*& v);
    ~RTDBVarCreate();
    int remainingRepetitions;
    RTDBVarSpec* varSpec;
    RTDBVarUpdate* varUpdate;
    int getIETypeLen(void);
    inet::Ptr<VarDisCreate> getPacketElement(void);
};

class RTDBVarReqUpdate {
public:
    RTDBVarReqUpdate(varID_t id, seqNo_t seqNo, repCnt_t repCnt);
    RTDBVarReqUpdate(RTDBVarReqUpdate*& v);
    varID_t varID;
    seqNo_t varSeqNo;
    int remainingRepetitions;
    int getIETypeLen(void);
    inet::Ptr<VarDisReqUpdate> getPacketElement(void);
};

class RTDBVarReqCreate {
public:
    RTDBVarReqCreate(varID_t id, repCnt_t repCnt);
    RTDBVarReqCreate(RTDBVarReqCreate*& v);
    varID_t varID;
    int remainingRepetitions;
    int getIETypeLen(void);
    inet::Ptr<VarDisReqCreate> getPacketElement(void);
};

class RTDBVarDelete {
public:
    RTDBVarDelete(varID_t id, repCnt_t repCnt);
    RTDBVarDelete(RTDBVarDelete*& v);
    varID_t varID;
    int remainingRepetitions;
    int getIETypeLen(void);
    inet::Ptr<VarDisDelete> getPacketElement(void);
};

#endif /* RTDBINFORMATIONELEMENTS_H_ */
