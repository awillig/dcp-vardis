#include <cstdint>
#include <gtest/gtest.h>
#include <dcp/common/area.h>
#include <dcp/vardis/vardis_transmissible_types.h>

namespace dcp::vardis {

  // ------------------------------------------------------------
  
  TEST(VardisTTTest, VardisTransmissibleTest_Basic) {
    VarIdT id0 (10);
    VarIdT id1 (id0);
    VarIdT id2 = id1;
    EXPECT_EQ (id0, id1);
    EXPECT_EQ (id0, id2);
    EXPECT_EQ (id0, 10);

    VarLenT len0 (20);
    VarLenT len1 (len0);
    VarLenT len2 = len1;
    EXPECT_EQ (len0, len1);
    EXPECT_EQ (len0, len2);
    EXPECT_EQ (len0, 20);

    VarRepCntT rc0 (30);
    VarRepCntT rc1 (rc0);
    VarRepCntT rc2 = rc1;
    EXPECT_EQ (rc0, rc1);
    EXPECT_EQ (rc0, rc2);
    EXPECT_EQ (rc0, 30);

    VarSeqnoT seq0 (40);
    VarSeqnoT seq1 (seq0);
    VarSeqnoT seq2 = seq1;
    EXPECT_EQ (seq0, seq1);
    EXPECT_EQ (seq0, seq2);
    EXPECT_EQ (seq0, 40);


    double d0 = 3.14;
    VarValueT val0 (VarLenT (sizeof (double)), (byte*) &d0);
    EXPECT_EQ (val0.total_size(), val0.fixed_size() + sizeof(double));
    EXPECT_EQ (val0.total_size(), VarLenT::fixed_size() + sizeof(double));

    EXPECT_EQ (VarSummT::fixed_size(), VarIdT::fixed_size() + VarSeqnoT::fixed_size());

    VarUpdateT upd0;
    upd0.varId = VarIdT (44);
    upd0.seqno = VarSeqnoT (99);
    upd0.value = val0;
    EXPECT_EQ (upd0.total_size(), VarIdT::fixed_size() + VarSeqnoT::fixed_size() + val0.total_size());

    StringT descr ("hello");
    EXPECT_EQ (descr.length, 5);
    NodeIdentifierT nodeId;
    nodeId.nodeId[0] = 1;
    nodeId.nodeId[1] = 2;
    nodeId.nodeId[2] = 3;
    nodeId.nodeId[3] = 4;
    nodeId.nodeId[4] = 5;
    nodeId.nodeId[5] = 6;
    VarSpecT spec;
    spec.varId   = VarIdT (10);
    spec.prodId  = nodeId;
    spec.repCnt  = VarRepCntT (20);
    spec.descr   = descr;
    EXPECT_EQ (spec.varId, 10);
    EXPECT_EQ (spec.prodId.nodeId[0], 1);
    EXPECT_EQ (spec.prodId.nodeId[1], 2);
    EXPECT_EQ (spec.prodId.nodeId[2], 3);
    EXPECT_EQ (spec.prodId.nodeId[3], 4);
    EXPECT_EQ (spec.prodId.nodeId[4], 5);
    EXPECT_EQ (spec.prodId.nodeId[5], 6);
    EXPECT_EQ (spec.repCnt, 20);
    EXPECT_NE (spec.descr.data, descr.data);
    EXPECT_EQ (spec.descr, descr);
    EXPECT_EQ (spec.fixed_size(), VarIdT::fixed_size() + NodeIdentifierT::fixed_size() + VarRepCntT::fixed_size() + TimeStampT::fixed_size() + VarTimeoutT::fixed_size() + VarLenT::fixed_size());
    EXPECT_EQ (spec.total_size(), spec.fixed_size() + descr.length);

    EXPECT_EQ (VarCreateT::fixed_size(), VarSpecT::fixed_size() + VarUpdateT::fixed_size());
    EXPECT_EQ (VarDeleteT::fixed_size(), VarIdT::fixed_size());
    EXPECT_EQ (VarReqUpdateT::fixed_size(), VarSummT::fixed_size());
    EXPECT_EQ (VarReqCreateT::fixed_size(), VarIdT::fixed_size());
    EXPECT_EQ (ICHeaderT::fixed_size(), InstructionContainerT::fixed_size() + sizeof(byte));
    
  }

  // ------------------------------------------------------------

  TEST(VardisTTTest, VardisTransmissibleTest_MoreRecentTest) {
    EXPECT_FALSE (more_recent_seqno (0, 1));
    EXPECT_FALSE (more_recent_seqno (VarSeqnoT(0), VarSeqnoT(1)));
    EXPECT_FALSE (more_recent_seqno (255, 1));
    EXPECT_FALSE (more_recent_seqno (VarSeqnoT(255), VarSeqnoT(1)));
  
    EXPECT_FALSE (more_recent_seqno (0, 0));
    EXPECT_TRUE (more_recent_seqno (1,0));
    EXPECT_TRUE (more_recent_seqno (VarSeqnoT(1), VarSeqnoT(0)));
    EXPECT_TRUE (more_recent_seqno (127, 0));
    EXPECT_TRUE (more_recent_seqno (VarSeqnoT(127), VarSeqnoT(0)));
    EXPECT_FALSE (more_recent_seqno (128, 0));
    EXPECT_FALSE (more_recent_seqno (VarSeqnoT(128), VarSeqnoT(0)));
    
    EXPECT_TRUE (more_recent_seqno (0, 255));
    EXPECT_TRUE (more_recent_seqno (VarSeqnoT(0), VarSeqnoT(255)));
    EXPECT_TRUE (more_recent_seqno (126, 255));
    EXPECT_TRUE (more_recent_seqno (VarSeqnoT(126), VarSeqnoT(255)));
    EXPECT_FALSE (more_recent_seqno (127, 255));
    EXPECT_FALSE (more_recent_seqno (VarSeqnoT(127), VarSeqnoT(255)));

    EXPECT_TRUE (more_recent_seqno (1, 230));
    EXPECT_TRUE (more_recent_seqno (VarSeqnoT(1), VarSeqnoT(230)));
  }

  // ------------------------------------------------------------
  
  TEST(VardisTTTest, VardisTransmissibleTest_Serialization) {

    byte buffer [1000];
    MemoryChunkAssemblyArea      ass_area ("ass_area", 1000, buffer);
    VarIdT aid (10);
    aid.serialize (ass_area);
    VarLenT alen (20);
    alen.serialize (ass_area);
    VarSeqnoT aseq (30);
    aseq.serialize (ass_area);
    VarRepCntT arc (40);
    arc.serialize (ass_area);
    NodeIdentifierT anodeid;
    anodeid.serialize (ass_area);
    double d0 = 3.14;
    VarValueT aval (VarLenT(sizeof(double)), (byte*) &d0);
    aval.serialize (ass_area);
    VarSummT asumm;
    asumm.varId = VarIdT (50);
    asumm.seqno = VarSeqnoT (60);
    asumm.serialize (ass_area);
    VarUpdateT aupd;
    aupd.varId = VarIdT (37);
    aupd.seqno = VarSeqnoT (38);
    aupd.value = aval;
    aupd.serialize (ass_area);
    StringT descr ("hello");
    VarSpecT aspec;
    aspec.varId = VarIdT (83);
    aspec.prodId = nullNodeIdentifier;
    aspec.repCnt = VarRepCntT (84);
    aspec.descr = descr;
    aspec.serialize (ass_area);
    VarCreateT acreate;
    acreate.spec = aspec;
    acreate.update = aupd;
    acreate.serialize (ass_area);
    VarDeleteT adel;
    adel.varId = VarIdT (123);
    adel.serialize (ass_area);
    VarReqUpdateT arequpd;
    arequpd.updSpec = asumm;
    arequpd.serialize (ass_area);
    VarReqCreateT areqcr;
    areqcr.varId = VarIdT (233);
    areqcr.serialize (ass_area);
    ICHeaderT aichdr;
    aichdr.icType = 44;
    aichdr.icNumRecords = 55;
    aichdr.serialize (ass_area);
    
    
    MemoryChunkDisassemblyArea   disass_area ("disass_area", ass_area.used(), buffer);
    VarIdT did;
    did.deserialize (disass_area);
    EXPECT_EQ (aid, did);
    VarLenT dlen;
    dlen.deserialize (disass_area);
    EXPECT_EQ (alen, dlen);
    VarSeqnoT dseq;
    dseq.deserialize (disass_area);
    EXPECT_EQ (aseq, dseq);
    VarRepCntT drc;
    drc.deserialize (disass_area);
    EXPECT_EQ (arc, drc);
    NodeIdentifierT dnodeid;
    dnodeid.deserialize (disass_area);
    EXPECT_EQ (anodeid, dnodeid);
    VarValueT dval;
    dval.deserialize (disass_area);
    EXPECT_EQ (aval, dval);
    EXPECT_NE (aval.data, dval.data);
    VarSummT dsumm;
    dsumm.deserialize (disass_area);
    EXPECT_EQ (asumm, dsumm);
    VarUpdateT dupd;
    dupd.deserialize (disass_area);
    EXPECT_EQ (aupd, dupd);
    VarSpecT dspec;
    dspec.deserialize (disass_area);
    EXPECT_EQ (aspec, dspec);
    VarCreateT dcreate;
    dcreate.deserialize (disass_area);
    EXPECT_EQ (acreate, dcreate);
    VarDeleteT ddel;
    ddel.deserialize (disass_area);
    EXPECT_EQ (adel, ddel);
    VarReqUpdateT drequpd;
    drequpd.deserialize (disass_area);
    EXPECT_EQ (arequpd, drequpd);
    VarReqCreateT dreqcr;
    dreqcr.deserialize (disass_area);
    EXPECT_EQ (areqcr, dreqcr);
    ICHeaderT dichdr;
    dichdr.deserialize (disass_area);
    EXPECT_EQ (aichdr, dichdr);
    

    EXPECT_EQ (disass_area.available(), 0);
  }

  // ------------------------------------------------------------
    
}
