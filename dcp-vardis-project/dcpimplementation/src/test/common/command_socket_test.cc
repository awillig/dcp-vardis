#include <cstdint>
#include <gtest/gtest.h>
#include <dcp/common/command_socket.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/logging_helpers.h>
#include <dcp/common/services_status.h>


using dcp::byte;
using dcp::CommandSocket;
using dcp::SocketException;
using dcp::ServiceConfirm;
using dcp::BP_STATUS_OK;
using dcp::DcpServiceType;
using dcp::ScopedClientSocket;

namespace keywords = boost::log::keywords;


/**********************************************************************
 * Tests constructor for proper error checking and also test reaction
 * of is_open depending on whether the socket is opened or
 * not.
 *********************************************************************/

logger_type log_null (keywords::channel = "NULL");

const std::string testsocketname = "/tmp/dcptestcommandsocket-googletest-tmpfile0785321";

TEST (CmdSockTest, BasicTest) {
  EXPECT_THROW (CommandSocket ("", 200), SocketException);
  EXPECT_THROW (CommandSocket ("test", 0), SocketException);

  CommandSocket cs1 (testsocketname, 200);
  EXPECT_FALSE (cs1.is_open());
  cs1.open_owner (log_null);
  EXPECT_TRUE (cs1.is_open());

  std::string testname108 = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567";
  std::string testname107 = "01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456";
  EXPECT_THROW (CommandSocket(testname108, 200).open_owner(log_null), SocketException);
  EXPECT_NO_THROW (CommandSocket(testname107, 200).open_owner(log_null));
  
}


typedef struct TestServiceConfirm : public ServiceConfirm {
  TestServiceConfirm () : ServiceConfirm (5555) {};
  int theval;
} TestServiceConfirm;


void csclient (int startval, int numvals)
{
  CommandSocket cs (testsocketname, 200);
  byte buffer [100];
  
  for (int i=0; i<numvals; i++)
    {
      ScopedClientSocket cl_sock (cs);
      int theval = startval + i;
      int nbytes = cl_sock.sendRequestAndReadResponseBlock<int> (theval, buffer, 100);
      EXPECT_EQ (nbytes, sizeof(TestServiceConfirm));
      TestServiceConfirm* pConf = (TestServiceConfirm*) buffer;
      EXPECT_EQ (pConf->theval, theval);
      EXPECT_EQ (pConf->s_type, 5555);
      EXPECT_EQ (pConf->status_code, BP_STATUS_OK);
    }
}



TEST (CmdSockTest, ClientServerTest) {
  CommandSocket cs (testsocketname, 200);
  cs.open_owner(log_null);

  EXPECT_TRUE(cs.is_open());
  
  std::thread thr_client_one (csclient, 100, 30);
  std::thread thr_client_two (csclient, 200, 30);

  int rbytes = -1;
  int reads  = 0;
  byte buffer [100];
  do {
    bool exitFlag = false;
    DcpServiceType serv_t = 0x7050;
    rbytes = cs.start_read_command (log_null, buffer, 100, serv_t, exitFlag);
    EXPECT_FALSE (exitFlag);
    if (rbytes == sizeof(int))
      {
	int* pInt = (int*) buffer;
	TestServiceConfirm conf;
	conf.theval = *pInt;
	conf.status_code = BP_STATUS_OK;
	cs.send_raw_data (log_null, (byte*) &conf, sizeof(TestServiceConfirm), exitFlag);
	EXPECT_FALSE (exitFlag);
	cs.stop_read_command (log_null, exitFlag);
	EXPECT_FALSE (exitFlag);
	reads++;
      }
  } while (rbytes > 0);

  thr_client_one.join ();
  thr_client_two.join ();
  
  EXPECT_EQ (reads, 60);
}
