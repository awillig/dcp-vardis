#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <list>
#include <thread>
#include <dcp/applications/vardisapp-test-variabletype.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/vardis/vardis_service_primitives.h>
#include <dcp/vardis/vardis_transmissible_types.h>
#include <dcp/vardis/vardisclient_configuration.h>
#include <dcp/vardis/vardisclient_lib.h>


using std::cout;
using std::endl;
using namespace dcp;

void print_help_and_exit (std::string execname)
{
  cout << execname << " <sockname> <shmname> <queryPeriodMS>" << endl
       << endl
       << "Periodically queries all current Vardis variables and displays their values," << endl
       << "assuming that they are test variables (including sequence number, timestamp" << endl
       << "and double value)." << endl
       << endl
       << "Parameters:" << endl
       << "    sockname:       filename of Vardis command socket (UNIX Domain socket)" << endl
       << "    shmname:        unique name of shared memory area for interfacing with VarDis" << endl
       << "    queryPeriodMS:  time period between two queries of the variable database (in ms, positive, no more than 65 s)" << endl;
  exit (EXIT_SUCCESS);
}

bool       exitFlag = false;

void signalHandler (int signum)
{
  cout << "Caught signal code " << signum << " (" << strsignal(signum) << "). Exiting." << endl;
  exitFlag = true;
}



int main (int argc, char* argv [])
{
  // ----------------------------------
  // Check parameters
  
  if (argc != 4)
    print_help_and_exit (std::string (argv[0]));

  std::string sockname (argv[1]);
  std::string shmname  (argv[2]);
  int         periodTmp  (std::stoi(std::string(argv[3])));


  if ((periodTmp <= 0) || (periodTmp > std::numeric_limits<uint16_t>::max()))
    {
      cout << "Parameter genPeriodMS outside allowed range. Aborting." << endl;
      return EXIT_FAILURE;
    }

  uint16_t  periodMS = (uint16_t) periodTmp;

  // ----------------------------------
  // Install signal handlers
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGINT, signalHandler);
  std::signal(SIGABRT, signalHandler);
  

  // ----------------------------------
  // Register with Vardis
  VardisClientConfiguration cl_conf;
  cl_conf.cmdsock_conf.commandSocketFile = sockname;
  cl_conf.shm_conf.shmAreaName           = shmname;

  try {
    VardisClientRuntime cl_rt (cl_conf);

    // ============================================
    // Main loop
    // ============================================
    
    cout << "Entering update loop. Stop with <Ctrl-C>." << endl;
    
    int counter = 0;
    
    while (not exitFlag)
      {
	std::this_thread::sleep_for (std::chrono::milliseconds (periodMS));
	
	cout << "----------------------------------------------------" << endl;
	cout << "Round " << counter << endl;
	counter++;
	
	std::list<DescribeDatabaseVariableDescription> db_list;
	
	DcpStatus dd_status = cl_rt.describe_database (db_list);
	if (dd_status != VARDIS_STATUS_OK)
	  {
	    cout << "Obtaining database description failed with status " << vardis_status_to_string (dd_status) << ", exiting." << endl;
	    return EXIT_FAILURE;
	  }
	
	for (const auto& descr : db_list)
	  {
	    VarIdT      respVarId;
	    VarLenT     respVarLen;
	    TimeStampT  respTimeStamp;
	    const size_t read_buffer_size = 1000;
	    byte     read_buffer [read_buffer_size];
	    DcpStatus read_status = cl_rt.rtdb_read (descr.varId, respVarId, respVarLen, respTimeStamp, read_buffer_size, read_buffer);
	    
	    if (read_status != VARDIS_STATUS_OK)
	      {
		cout << "Reading varId " << descr.varId << " failed with status " << vardis_status_to_string (read_status) << endl;
		continue;
	      }
	    
	    if (respVarId != descr.varId)
	      {
		cout << "Submitted read request for varId " << descr.varId << " but got response for varId " << respVarId << ", exiting." << endl;
		return EXIT_FAILURE;
	      }
	    
	    if (respVarLen != sizeof(VardisTestVariable))
	      {
		cout << "Submitted read request for varId " << descr.varId << ", got respVarLen = " << respVarLen << " but expected length " << sizeof(VardisTestVariable) << ", exiting." << endl;
		return EXIT_FAILURE;
	      }
	    
	    VardisTestVariable* tv_ptr = (VardisTestVariable*) read_buffer;
	    
	    TimeStampT start_time  = tv_ptr->tstamp;
	    TimeStampT rcvd_time   = respTimeStamp;
	    auto age = rcvd_time.milliseconds_passed_since (start_time);
	    
	    cout << "id = " << (int) descr.varId.val
   	         << ", prod = " << descr.prodId
	         << ", repCnt = " << (int) descr.repCnt.val
	         //<< ", descr = " << descr.description
	         //<< ", tStamp = " << descr.tStamp
	         << ", toBeDeleted = " << descr.toBeDeleted
	         << ", tv.seqno = " << tv_ptr->seqno
	         //<< ", tv.tstamp = " << tv_ptr->tstamp
	         << ", tv.value = " << tv_ptr->value
	         << ", age (ms) = " << age
	         << endl;
	    
	  }
      }
    return EXIT_SUCCESS;
    
  }
  catch (std::exception& e)
    {
      cout << "Caught an exception, got " << e.what() << ", exiting." << endl;
      return EXIT_FAILURE;
    }  
}
