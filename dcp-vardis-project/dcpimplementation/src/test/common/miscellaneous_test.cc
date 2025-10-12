#include <cstdint>
#include <exception>
#include <gtest/gtest.h>
#include <dcp/common/memblock.h>
#include <dcp/common/services_status.h>

using namespace dcp;

TEST (CommonMiscTest, ServiceTypeConversions) {

  for (uint16_t i = 0; i < 0x8000; i++)
    {
      switch(i) {
      case stBP_RegisterProtocol:
      case stBP_DeregisterProtocol:
      case stBP_ListRegisteredProtocols:
      case stBP_ClearBuffer:
      case stBP_QueryNumberBufferedPayloads:
      case stBP_ReceivePayload:
      case stBP_TransmitPayload:
      case stBP_ShutDown:
      case stBP_Activate:
      case stBP_Deactivate:
      case stBP_GetStatistics:
	{
	  EXPECT_NO_THROW (bp_service_type_to_string (i));
	  EXPECT_THROW (vardis_service_type_to_string (i), std::invalid_argument);
	  break;
	}

      case stVardis_RTDB_DescribeDatabase:
      case stVardis_RTDB_DescribeVariable:
      case stVardis_RTDB_Create:
      case stVardis_RTDB_Delete:
      case stVardis_RTDB_Update:
      case stVardis_RTDB_Read:
      case stVardis_Register:
      case stVardis_Deregister:
      case stVardis_Shutdown:
      case stVardis_Activate:
      case stVardis_Deactivate:
      case stVardis_GetStatistics:
	{
	  EXPECT_THROW (bp_service_type_to_string (i), std::invalid_argument);
	  EXPECT_NO_THROW (vardis_service_type_to_string (i));
	  break;
	}

      default:
	{
	  EXPECT_THROW (bp_service_type_to_string(i), std::invalid_argument);
	  EXPECT_THROW (vardis_service_type_to_string(i), std::invalid_argument);
	}
      }
    }
}



TEST (CommonMiscTest, StatusCodeConversions) {
  for (uint16_t i = 0; i<0x8000; i++)
    {
      switch(i) {
      case BP_STATUS_OK:
      case BP_STATUS_PROTOCOL_ALREADY_REGISTERED:
      case BP_STATUS_ILLEGAL_MAX_PAYLOAD_SIZE:
      case BP_STATUS_UNKNOWN_PROTOCOL:
      case BP_STATUS_PAYLOAD_TOO_LARGE:
      case BP_STATUS_EMPTY_PAYLOAD:
      case BP_STATUS_ILLEGAL_DROPPING_QUEUE_SIZE:
      case BP_STATUS_UNKNOWN_QUEUEING_MODE:
      case BP_STATUS_INACTIVE:
      case BP_STATUS_INTERNAL_ERROR:
      case BP_STATUS_INTERNAL_SHARED_MEMORY_ERROR:
      case BP_STATUS_ILLEGAL_SERVICE_TYPE:
      case BP_STATUS_WRONG_PROTOCOL_TYPE:
	{
	  EXPECT_NO_THROW (bp_status_to_string (i));
	  EXPECT_THROW (vardis_status_to_string (i), std::exception);
	  EXPECT_THROW (srp_status_to_string (i), std::exception);
	  break;
	}

      case VARDIS_STATUS_OK:
      case VARDIS_STATUS_VARIABLE_EXISTS:
      case VARDIS_STATUS_VARIABLE_DESCRIPTION_TOO_LONG:
      case VARDIS_STATUS_VALUE_TOO_LONG:
      case VARDIS_STATUS_EMPTY_VALUE:
      case VARDIS_STATUS_ILLEGAL_REPCOUNT:
      case VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST:
      case VARDIS_STATUS_NOT_PRODUCER:
      case VARDIS_STATUS_VARIABLE_IS_DELETED:
      case VARDIS_STATUS_INACTIVE: 
      case VARDIS_STATUS_INTERNAL_ERROR:
      case VARDIS_STATUS_APPLICATION_ALREADY_REGISTERED:
      case VARDIS_STATUS_INTERNAL_SHARED_MEMORY_ERROR:
      case VARDIS_STATUS_UNKNOWN_APPLICATION:
      	{
	  EXPECT_THROW (bp_status_to_string (i), std::exception);
	  EXPECT_NO_THROW (vardis_status_to_string (i));
	  EXPECT_THROW (srp_status_to_string (i), std::exception);
	  break;
	}

      case SRP_STATUS_OK:
      	{
	  EXPECT_THROW (bp_status_to_string (i), std::exception);
	  EXPECT_THROW (vardis_status_to_string (i), std::exception);
	  EXPECT_NO_THROW (srp_status_to_string (i));
	  break;
	}
	
      default:
	{
	  EXPECT_THROW (bp_status_to_string (i), std::exception);
	  EXPECT_THROW (vardis_status_to_string (i), std::exception);
	  EXPECT_THROW (srp_status_to_string (i), std::exception);
	  break;
	}
	
      }
    }
}


TEST (CommonMiscTest, MemBlock) {

  byte testarray[] = {1,2,3,4,5,6,7,8,9,10,0};

  MemBlock mb0;
  EXPECT_EQ (mb0.data, nullptr);
  EXPECT_EQ (mb0.length, 0);
  EXPECT_TRUE (mb0.do_delete);
  
  MemBlock mb1 (mb0);
  EXPECT_EQ (mb1.data, nullptr);
  EXPECT_EQ (mb1.length, 0);
  EXPECT_EQ (mb1.do_delete, mb0.do_delete);

  
  MemBlock mb2 (11, testarray);
  EXPECT_NE (mb2.data, nullptr);
  EXPECT_EQ (mb2.length, 11);
  EXPECT_TRUE (mb2.do_delete);
  EXPECT_EQ (std::memcmp(testarray, mb2.data, 11), 0);
  EXPECT_NE (testarray, mb2.data);

  byte* mb2data     = mb2.data;
  size_t mb2length  = mb2.length;
  bool mb2dodelete  = mb2.do_delete;
  MemBlock mb3 = std::move(mb2);
  EXPECT_EQ (mb2.data, nullptr);
  EXPECT_EQ (mb2.length, 0);
  EXPECT_EQ (mb2.do_delete, true);
  EXPECT_EQ (mb3.data, mb2data);
  EXPECT_EQ (mb3.length, mb2length);
  EXPECT_EQ (mb3.do_delete, mb2dodelete);


  MemBlock mb4 (11, testarray);

  EXPECT_EQ (mb3, mb4);
}
