#include <cstdint>
#include <gtest/gtest.h>
#include <dcp/common/area.h>
#include <dcp/vardis/vardis_protocol_data.h>
#include <dcp/vardis/vardis_store_array_shm.h>

namespace dcp::vardis {

  NodeIdentifierT addr1 ("01:02:03:04:05:06");
  NodeIdentifierT addr2 ("11:12:13:14:15:16");
  
  // ------------------------------------------------------------
  
  TEST(VardisProtDataTest, Basic) {
    ArrayVariableStoreShm<256,128> vstore ("shm-vardis-protocol-data-test", true, 20, 32, 32, 5, nullNodeIdentifier);
    VardisProtocolData protData (vstore);

    EXPECT_NE (addr1, addr2);
    
    EXPECT_FALSE (protData.variableExists (0));
    EXPECT_ANY_THROW (protData.producerIsMe (0));
    EXPECT_FALSE (protData.createQ.contains (0));
    EXPECT_FALSE (protData.updateQ.contains (0));
    EXPECT_FALSE (protData.deleteQ.contains (0));
    EXPECT_FALSE (protData.summaryQ.contains (0));
    EXPECT_FALSE (protData.reqCreateQ.contains (0));
    EXPECT_FALSE (protData.reqUpdQ.contains (0));

  }

  // ------------------------------------------------------------

  TEST(VardisProtDataTest, VardisActiveForCRUDServices) {
    ArrayVariableStoreShm<256,128> vstore ("shm-vardis-protocol-data-test", true, 20, 32, 32, 5, addr1);
    VardisProtocolData protData (vstore);
    protData.vardis_store.set_vardis_isactive (false);

    double dval  = 3.14;
    double ddval = 2*dval;
    RTDB_Create_Request cr_req;
    cr_req.spec.varId   = 10;
    cr_req.spec.prodId  = addr1;
    cr_req.spec.repCnt  = 3;
    cr_req.spec.descr   = StringT ("hello");
    cr_req.value        = VarValueT (sizeof(double), (byte*) &dval);
    RTDB_Update_Request upd_req;
    upd_req.varId = 10;
    upd_req.value = VarValueT (sizeof(double), (byte*) &ddval);
    RTDB_Delete_Request del_req;
    del_req.varId = 10;
    RTDB_Read_Request read_req;
    read_req.varId = 10;
    EXPECT_EQ (protData.handle_rtdb_create_request (cr_req).status_code, VARDIS_STATUS_INACTIVE);
    EXPECT_EQ (protData.handle_rtdb_update_request (upd_req).status_code, VARDIS_STATUS_INACTIVE);
    EXPECT_EQ (protData.handle_rtdb_read_request (read_req).status_code, VARDIS_STATUS_INACTIVE);
    EXPECT_EQ (protData.handle_rtdb_delete_request (del_req).status_code, VARDIS_STATUS_INACTIVE);

    protData.vardis_store.set_vardis_isactive (true);
    EXPECT_EQ (protData.handle_rtdb_create_request (cr_req).status_code, VARDIS_STATUS_OK);
    EXPECT_EQ (protData.handle_rtdb_update_request (upd_req).status_code, VARDIS_STATUS_OK);
    EXPECT_EQ (protData.handle_rtdb_read_request (read_req).status_code, VARDIS_STATUS_OK);
    EXPECT_EQ (protData.handle_rtdb_delete_request (del_req).status_code, VARDIS_STATUS_OK);
    
  }

  // ------------------------------------------------------------

  TEST(VardisProtDataTest, RTDBCreateLimits) {
    ArrayVariableStoreShm<256,128> vstore ("shm-vardis-protocol-data-test", true, 20, 32, 32, 5, addr1);
    VardisProtocolData protData (vstore);
    protData.vardis_store.set_vardis_isactive (true);

    // Verify that repeated creation of same variable returns that variable exists
    {
      double dval  = 3.14;
      RTDB_Create_Request cr_req1;
      cr_req1.spec.varId   = 10;
      cr_req1.spec.prodId  = addr1;
      cr_req1.spec.repCnt  = 3;
      cr_req1.spec.descr   = StringT ("hello");
      cr_req1.value        = VarValueT (sizeof(double), (byte*) &dval);
      EXPECT_EQ (protData.createQ.size(), 0);
      EXPECT_EQ (protData.summaryQ.size(), 0);
      EXPECT_EQ (protData.handle_rtdb_create_request (cr_req1).status_code, VARDIS_STATUS_OK);
      EXPECT_EQ (protData.createQ.size(), 1);
      EXPECT_EQ (protData.summaryQ.size(), 1);
      //EXPECT_EQ (protData.handle_rtdb_create_request (cr_req1).status_code, VARDIS_STATUS_VARIABLE_EXISTS);
    }

    // Check handling of maximum description length (default value 32)
    {
      double ddval  = 6.28;
      RTDB_Create_Request cr_req2;
      cr_req2.spec.varId   = 20;
      cr_req2.spec.prodId  = addr1;
      cr_req2.spec.repCnt  = 3;
      cr_req2.spec.descr   = StringT ("012345678901234567890123456789012");
      cr_req2.value        = VarValueT (sizeof(double), (byte*) &ddval);
      EXPECT_EQ (protData.handle_rtdb_create_request (cr_req2).status_code, VARDIS_STATUS_VARIABLE_DESCRIPTION_TOO_LONG);
      cr_req2.spec.descr   = StringT ("01234567890123456789012345678901");
      EXPECT_EQ (protData.handle_rtdb_create_request (cr_req2).status_code, VARDIS_STATUS_OK);
    }

    // Check handling of maximum value length (default value 32)
    {
      byte buffer [50];
      RTDB_Create_Request cr_req3;
      cr_req3.spec.varId   = 30;
      cr_req3.spec.prodId  = addr1;
      cr_req3.spec.repCnt  = 3;
      cr_req3.spec.descr   = StringT ("hello");
      cr_req3.value        = VarValueT (33, buffer);
      EXPECT_EQ (protData.handle_rtdb_create_request (cr_req3).status_code, VARDIS_STATUS_VALUE_TOO_LONG);
      cr_req3.value        = VarValueT (32, buffer);
      EXPECT_EQ (protData.handle_rtdb_create_request (cr_req3).status_code, VARDIS_STATUS_OK);
    }

    // Check handling of zero value length
    {
      byte buffer [50];
      RTDB_Create_Request cr_req4;
      cr_req4.spec.varId   = 40;
      cr_req4.spec.prodId  = addr1;
      cr_req4.spec.repCnt  = 3;
      cr_req4.spec.descr   = StringT ("hello");
      cr_req4.value        = VarValueT (0, buffer);
      EXPECT_EQ (protData.handle_rtdb_create_request (cr_req4).status_code, VARDIS_STATUS_EMPTY_VALUE);
      cr_req4.value        = VarValueT (1, buffer);
      EXPECT_EQ (protData.handle_rtdb_create_request (cr_req4).status_code, VARDIS_STATUS_OK);
    }

    // Check handling of repcount value (larger than zero, default maximum is five)
    {
      byte buffer [50];
      RTDB_Create_Request cr_req5;
      cr_req5.spec.varId   = 50;
      cr_req5.spec.prodId  = addr1;
      cr_req5.spec.repCnt  = 0;
      cr_req5.spec.descr   = StringT ("hello");
      cr_req5.value        = VarValueT (20, buffer);
      EXPECT_EQ (protData.handle_rtdb_create_request (cr_req5).status_code, VARDIS_STATUS_ILLEGAL_REPCOUNT);
      cr_req5.spec.repCnt  = 6;
      EXPECT_EQ (protData.handle_rtdb_create_request (cr_req5).status_code, VARDIS_STATUS_ILLEGAL_REPCOUNT);
      cr_req5.spec.repCnt  = 5;
      EXPECT_EQ (protData.handle_rtdb_create_request (cr_req5).status_code, VARDIS_STATUS_OK);
    }
    
  }

  
  // ------------------------------------------------------------

  TEST(VardisProtDataTest, RTDBUpdateLimits) {
    ArrayVariableStoreShm<256,128> vstore ("shm-vardis-protocol-data-test", true, 20, 32, 32, 5, addr1);
    VardisProtocolData protData (vstore);
    protData.vardis_store.set_vardis_isactive (true);

    double dval  = 3.14;
    RTDB_Create_Request cr_req;
    cr_req.spec.varId   = 10;
    cr_req.spec.prodId  = addr1;
    cr_req.spec.repCnt  = 3;
    cr_req.spec.descr   = StringT ("hello");
    cr_req.value        = VarValueT (sizeof(double), (byte*) &dval);
    EXPECT_EQ (protData.handle_rtdb_create_request (cr_req).status_code, VARDIS_STATUS_OK);

    // Check status of queues for successful update
    {
      RTDB_Update_Request upd_req;
      upd_req.varId = 10;
      upd_req.value = VarValueT (sizeof(double), (byte*) &dval);
      EXPECT_EQ (protData.summaryQ.size(), 1);
      EXPECT_EQ (protData.createQ.size(), 1);
      EXPECT_EQ (protData.updateQ.size(), 0);
      EXPECT_EQ (protData.handle_rtdb_update_request (upd_req).status_code, VARDIS_STATUS_OK);
      EXPECT_EQ (protData.summaryQ.size(), 1);
      EXPECT_EQ (protData.createQ.size(), 1);
      EXPECT_EQ (protData.updateQ.size(), 1);
    }
    
    // Check attempted update on non-existing variable
    {
      double dval = 3.14;
      RTDB_Update_Request upd_req;
      upd_req.varId = 20;
      upd_req.value = VarValueT (sizeof(double), (byte*) &dval);
      EXPECT_EQ (protData.handle_rtdb_update_request (upd_req).status_code, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST);
    }

    // Check attempted update on variable for which I am not the producer
    {
      double dval = 3.14;
      RTDB_Update_Request upd_req;
      upd_req.varId = 10;
      protData.ownNodeIdentifier = addr2;
      upd_req.value = VarValueT (sizeof(double), (byte*) &dval);
      EXPECT_EQ (protData.handle_rtdb_update_request (upd_req).status_code, VARDIS_STATUS_NOT_PRODUCER);
      protData.ownNodeIdentifier = addr1;
    }

    // Check attempted update with zero length value
    {
      double dval = 3.14;
      RTDB_Update_Request upd_req;
      upd_req.varId = 10;
      upd_req.value = VarValueT (0, (byte*) &dval);
      EXPECT_EQ (protData.handle_rtdb_update_request (upd_req).status_code, VARDIS_STATUS_EMPTY_VALUE);
    }

    // Check attempted update with a too long value
    {
      byte buffer [50];
      RTDB_Update_Request upd_req;
      upd_req.varId = 10;
      upd_req.value = VarValueT (33, buffer);
      EXPECT_EQ (protData.handle_rtdb_update_request (upd_req).status_code, VARDIS_STATUS_VALUE_TOO_LONG);
      upd_req.value = VarValueT (32, buffer);
      EXPECT_EQ (protData.handle_rtdb_update_request (upd_req).status_code, VARDIS_STATUS_OK);
    }
    
    // Check attempted update on a variable that is to be deleted
    {
      RTDB_Delete_Request del_req;
      del_req.varId = 10;
      EXPECT_EQ (protData.handle_rtdb_delete_request (del_req).status_code, VARDIS_STATUS_OK);
      double dval = 3.14;
      RTDB_Update_Request upd_req;
      upd_req.varId = 10;
      upd_req.value = VarValueT (sizeof(double), (byte*) &dval);
      EXPECT_EQ (protData.handle_rtdb_update_request (upd_req).status_code, VARDIS_STATUS_VARIABLE_IS_DELETED);
    }
    
  }

  // ------------------------------------------------------------

  TEST(VardisProtDataTest, RTDBDeleteLimits) {
    ArrayVariableStoreShm<256,128> vstore ("shm-vardis-protocol-data-test", true, 20, 32, 32, 5, addr1);
    VardisProtocolData protData (vstore);
    protData.vardis_store.set_vardis_isactive (true);

    double dval  = 3.14;
    RTDB_Create_Request cr_req;
    cr_req.spec.varId   = 10;
    cr_req.spec.prodId  = addr1;
    cr_req.spec.repCnt  = 3;
    cr_req.spec.descr   = StringT ("hello");
    cr_req.value        = VarValueT (sizeof(double), (byte*) &dval);
    EXPECT_EQ (protData.handle_rtdb_create_request (cr_req).status_code, VARDIS_STATUS_OK);

    // Check attempted delete on non-existing variable
    {
      RTDB_Delete_Request del_req;
      del_req.varId = 20;
      EXPECT_EQ (protData.handle_rtdb_delete_request (del_req).status_code, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST);
    }

    // Check attempted delete on variable for which I am not the owner
    {
      RTDB_Delete_Request del_req;
      del_req.varId = 10;
      protData.ownNodeIdentifier = addr2;
      EXPECT_EQ (protData.handle_rtdb_delete_request (del_req).status_code, VARDIS_STATUS_NOT_PRODUCER);
      protData.ownNodeIdentifier = addr1;
    }

    // Check attempted delete on a variable already being deleted
    {
      RTDB_Delete_Request del_req;
      del_req.varId = 10;
      EXPECT_EQ (protData.createQ.size(), 1);
      EXPECT_EQ (protData.summaryQ.size(), 1);
      EXPECT_EQ (protData.deleteQ.size(), 0);
      EXPECT_EQ (protData.handle_rtdb_delete_request (del_req).status_code, VARDIS_STATUS_OK);
      EXPECT_EQ (protData.createQ.size(), 0);
      EXPECT_EQ (protData.summaryQ.size(), 0);
      EXPECT_EQ (protData.deleteQ.size(), 1);
      EXPECT_EQ (protData.handle_rtdb_delete_request (del_req).status_code, VARDIS_STATUS_VARIABLE_IS_DELETED);
    }
    
  }

  // ------------------------------------------------------------

  TEST(VardisProtDataTest, RTDBReadLimits) {
    ArrayVariableStoreShm<256,128> vstore ("shm-vardis-protocol-data-test", true, 20, 32, 32, 5, addr1);
    VardisProtocolData protData (vstore);
    protData.vardis_store.set_vardis_isactive (true);

    double dval  = 3.14;
    RTDB_Create_Request cr_req;
    cr_req.spec.varId   = 10;
    cr_req.spec.prodId  = addr1;
    cr_req.spec.repCnt  = 3;
    cr_req.spec.descr   = StringT ("hello");
    cr_req.value        = VarValueT (sizeof(double), (byte*) &dval);
    EXPECT_EQ (protData.handle_rtdb_create_request (cr_req).status_code, VARDIS_STATUS_OK);

    // Check attempted read on non-existing variable
    {
      RTDB_Read_Request read_req;
      read_req.varId = 20;
      EXPECT_EQ (protData.handle_rtdb_read_request (read_req).status_code, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST);
    }

    // Check attempted read on a variable already being deleted
    {
      RTDB_Delete_Request del_req;
      del_req.varId = 10;
      RTDB_Read_Request read_req;
      read_req.varId = 10;
      EXPECT_EQ (protData.handle_rtdb_delete_request (del_req).status_code, VARDIS_STATUS_OK);
      EXPECT_EQ (protData.handle_rtdb_read_request (read_req).status_code, VARDIS_STATUS_VARIABLE_IS_DELETED);
    }
    
  }

  
  
  // ------------------------------------------------------------

  TEST(VardisProtDataTest, RTDBOperationOrder) {
    ArrayVariableStoreShm<256,128> vstore ("shm-vardis-protocol-data-test", true, 20, 32, 32, 5, addr1);    
    VardisProtocolData protData (vstore);
    protData.vardis_store.set_vardis_isactive (true);
    
    double dval  = 3.14;
    RTDB_Create_Request cr_req;
    cr_req.spec.varId   = 10;
    cr_req.spec.prodId  = addr1;
    cr_req.spec.repCnt  = 3;
    cr_req.spec.descr   = StringT ("hello");
    cr_req.value        = VarValueT (sizeof(double), (byte*) &dval);
    EXPECT_EQ (protData.handle_rtdb_create_request (cr_req).status_code, VARDIS_STATUS_OK);

    // Check that I can read back the initial value
    {
      RTDB_Read_Request read_req;
      read_req.varId = 10;
      RTDB_Read_Confirm read_conf = protData.handle_rtdb_read_request (read_req);
      EXPECT_EQ (read_conf.status_code, VARDIS_STATUS_OK);
      EXPECT_EQ (read_conf.s_type, stVardis_RTDB_Read);
      EXPECT_EQ (read_conf.value.length, sizeof(double));
      EXPECT_NE (read_conf.value.data, nullptr);
      double* pdval = (double*) read_conf.value.data;
      EXPECT_EQ (dval, *pdval);
    }


    // Update and then check that I can read back the updated value
    {
      double dval = 6.28;
      RTDB_Update_Request upd_req;
      upd_req.varId = 10;
      upd_req.value = VarValueT (sizeof(double), (byte*) &dval);
      EXPECT_EQ (protData.handle_rtdb_update_request (upd_req).status_code, VARDIS_STATUS_OK);

      RTDB_Read_Request read_req;
      read_req.varId = 10;
      RTDB_Read_Confirm read_conf = protData.handle_rtdb_read_request (read_req);
      EXPECT_EQ (read_conf.status_code, VARDIS_STATUS_OK);
      EXPECT_EQ (read_conf.s_type, stVardis_RTDB_Read);
      EXPECT_EQ (read_conf.value.length, sizeof(double));
      EXPECT_NE (read_conf.value.data, nullptr);
      double* pdval = (double*) read_conf.value.data;
      EXPECT_EQ (dval, *pdval);
    }    
  }

  // ------------------------------------------------------------
  
  TEST(VardisProtDataTest, processVarCreate) {
    ArrayVariableStoreShm<256,128> vstore ("shm-vardis-protocol-data-test", true, 20, 32, 32, 5, addr1);
    VardisProtocolData protData (vstore);
    protData.vardis_store.set_vardis_isactive (true);
    
    double dval  = 3.14;
    double ddval = 6.28;
    RTDB_Create_Request cr_req;
    cr_req.spec.varId   = 10;
    cr_req.spec.prodId  = addr1;
    cr_req.spec.repCnt  = 3;
    cr_req.spec.descr   = StringT ("hello");
    cr_req.value        = VarValueT (sizeof(double), (byte*) &dval);
    EXPECT_EQ (protData.handle_rtdb_create_request (cr_req).status_code, VARDIS_STATUS_OK);

    VarCreateT valid_create;
    valid_create.spec.varId      = 20;
    valid_create.spec.prodId     = addr2;
    valid_create.spec.repCnt     = 4;
    valid_create.spec.descr      = StringT ("hello");
    valid_create.update.varId    = 20;
    valid_create.update.seqno    = 0;
    valid_create.update.value    = VarValueT (sizeof(double), (byte*) &dval);

    // first check that valid VarCreate is processed correctly
    EXPECT_FALSE (protData.variableExists (20));
    EXPECT_EQ (protData.createQ.size(), 1);
    EXPECT_EQ (protData.summaryQ.size(), 1);
    protData.process_var_create (valid_create);
    EXPECT_TRUE (protData.variableExists (20));
    EXPECT_EQ (protData.createQ.size(), 2);
    EXPECT_EQ (protData.summaryQ.size(), 2);
    
    // As a side check, verify that the current node cannot update that variable
    RTDB_Update_Request test_upd_req;
    test_upd_req.varId  =  20;
    test_upd_req.value  =  VarValueT (sizeof(double), (byte*) &ddval);
    EXPECT_EQ (protData.handle_rtdb_update_request (test_upd_req).status_code, VARDIS_STATUS_NOT_PRODUCER);

    
    // Test attempted create on an already existing variable (by
    // checking that an updated value does not get accepted)
    {
      double dddval = 9.42;
      VarCreateT wrong_create    = valid_create;
      wrong_create.spec.varId    = 10;
      wrong_create.update.value  = VarValueT (sizeof(double), (byte*) & dddval);
      protData.process_var_create (wrong_create);
      RTDB_Read_Request read_req;
      read_req.varId = 10;
      auto rd_conf = protData.handle_rtdb_read_request (read_req);
      EXPECT_EQ (rd_conf.status_code, VARDIS_STATUS_OK);
      EXPECT_EQ (rd_conf.value.length, sizeof(double));
      EXPECT_NE (dddval, *((double*) rd_conf.value.data));
    }

    // Test attempted create on a new variable, but with descriptions
    // of illegal length
    {
      double dddval = 9.42;
      VarCreateT wrong_create    = valid_create;
      wrong_create.spec.varId    = 30;
      wrong_create.spec.descr    = StringT ("");
      wrong_create.update.value  = VarValueT (sizeof(double), (byte*) & dddval);
      protData.process_var_create (wrong_create);
      RTDB_Read_Request read_req;
      read_req.varId = 30;
      auto rd_conf = protData.handle_rtdb_read_request (read_req);
      EXPECT_EQ (rd_conf.status_code, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST);

      wrong_create.spec.descr  = StringT ("012345678901234567890123456789012");
      protData.process_var_create (wrong_create);
      rd_conf = protData.handle_rtdb_read_request (read_req);
      EXPECT_EQ (rd_conf.status_code, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST);

      wrong_create.spec.descr  = StringT ("01234567890123456789012345678901");
      protData.process_var_create (wrong_create);
      rd_conf = protData.handle_rtdb_read_request (read_req);
      EXPECT_EQ (rd_conf.status_code, VARDIS_STATUS_OK);
    }

    
    // Test attempted create on a new variable, but with values
    // of illegal length
    {
      byte buffer [50];
      VarCreateT wrong_create    = valid_create;
      wrong_create.spec.varId    = 40;
      wrong_create.spec.descr    = StringT ("Hello");
      wrong_create.update.value  = VarValueT (0, buffer);
      protData.process_var_create (wrong_create);
      RTDB_Read_Request read_req;
      read_req.varId = 40;
      auto rd_conf = protData.handle_rtdb_read_request (read_req);
      EXPECT_EQ (rd_conf.status_code, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST);

      wrong_create.update.value  = VarValueT (33, buffer);
      protData.process_var_create (wrong_create);
      rd_conf = protData.handle_rtdb_read_request (read_req);
      EXPECT_EQ (rd_conf.status_code, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST);

      wrong_create.update.value  = VarValueT (32, buffer);
      protData.process_var_create (wrong_create);
      rd_conf = protData.handle_rtdb_read_request (read_req);
      EXPECT_EQ (rd_conf.status_code, VARDIS_STATUS_OK);
    }


    // Test attempted create on a new variable, but with illegal
    // repCnt values
    {
      byte buffer [50];
      VarCreateT wrong_create    = valid_create;
      wrong_create.spec.varId    = 50;
      wrong_create.spec.descr    = StringT ("Hello");
      wrong_create.spec.repCnt   = 0;
      wrong_create.update.value  = VarValueT (10, buffer);
      protData.process_var_create (wrong_create);
      RTDB_Read_Request read_req;
      read_req.varId = 50;
      auto rd_conf = protData.handle_rtdb_read_request (read_req);
      EXPECT_EQ (rd_conf.status_code, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST);

      wrong_create.spec.repCnt  = 6;
      protData.process_var_create (wrong_create);
      rd_conf = protData.handle_rtdb_read_request (read_req);
      EXPECT_EQ (rd_conf.status_code, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST);

      wrong_create.spec.repCnt = 5;
      protData.process_var_create (wrong_create);
      rd_conf = protData.handle_rtdb_read_request (read_req);
      EXPECT_EQ (rd_conf.status_code, VARDIS_STATUS_OK);
    }
    
  }

  // ------------------------------------------------------------
  
  TEST(VardisProtDataTest, processVarDelete) {
    ArrayVariableStoreShm<256,128> vstore ("shm-vardis-protocol-data-test", true, 20, 32, 32, 5, addr1);
    VardisProtocolData protData (vstore);
    protData.vardis_store.set_vardis_isactive (true);

    double dval  = 3.14;
    RTDB_Create_Request cr_req;
    cr_req.spec.varId   = 10;
    cr_req.spec.prodId  = addr1;
    cr_req.spec.repCnt  = 3;
    cr_req.spec.descr   = StringT ("hello");
    cr_req.value        = VarValueT (sizeof(double), (byte*) &dval);
    EXPECT_EQ (protData.handle_rtdb_create_request (cr_req).status_code, VARDIS_STATUS_OK);

    // Check that a delete is not accepted when I am the producer
    VarDeleteT del_req;
    del_req.varId = 10;
    protData.process_var_delete (del_req);
    RTDB_Read_Request read_req;
    read_req.varId = 10;
    EXPECT_EQ (protData.handle_rtdb_read_request (read_req).status_code, VARDIS_STATUS_OK);

    // Check that a delete is accepted when I am not the producer
    protData.ownNodeIdentifier = addr2;
    protData.process_var_delete (del_req);
    EXPECT_EQ (protData.handle_rtdb_read_request (read_req).status_code, VARDIS_STATUS_VARIABLE_IS_DELETED);
    
  }
  
  // ------------------------------------------------------------
    
}
