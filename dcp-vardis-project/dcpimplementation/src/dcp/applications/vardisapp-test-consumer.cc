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
using std::cerr;
using dcp::vardis::defaultVardisCommandSocketFileName;
using dcp::vardis::defaultVardisStoreShmName;

using namespace dcp;


const std::string defaultVardisClientShmName = "shm-vardisapp-test-consumer";


void print_version ()
{
  cout << dcp::dcpHighlevelDescription
       << " -- version " << dcp::dcpVersionNumber
       << endl;
}

bool       exitFlag = false;

void signalHandler (int signum)
{
  cout << "Caught signal code " << signum << " (" << strsignal(signum) << "). Exiting." << endl;
  exitFlag = true;
}



int main (int argc, char* argv [])
{
  std::string cmdsock_name  = defaultVardisCommandSocketFileName;
  std::string shmname_cli   = defaultVardisClientShmName;
  std::string shmname_glob  = defaultVardisStoreShmName;
  int     periodTmp = 0;

  
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h",         "produce help message and exit")
    ("version,v",      "show version information and exit")
    ("sockname,s",     po::value<std::string>(&cmdsock_name)->default_value(defaultVardisCommandSocketFileName), "filename of VarDis command socket (UNIX Domain Socket)")
    ("shmcli,c",       po::value<std::string>(&shmname_cli)->default_value(defaultVardisClientShmName), "Name of shared memory area for interfacing with Vardis")
    ("shmgdb,g",       po::value<std::string>(&shmname_glob)->default_value(defaultVardisStoreShmName), "Unique name of shared memory area for accessing VarDis variables (global database)")
    ("period",         po::value<int>(&periodTmp), "Generation period (in ms)")
    ;

  po::positional_options_description desc_pos;
  desc_pos.add ("period", 1);

  try {
    po::variables_map vm;
    po::store (po::command_line_parser(argc, argv).options(desc).positional(desc_pos).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
      {
	cout << std::string (argv[0]) << " [-s <sockname>] [-mc <shmcli>] [-mg <shmgdb>] <queryperiodMS>" << endl;
	cout << desc << endl;
	return EXIT_SUCCESS;
      }

    if (vm.count("version"))
      {
	print_version();
	return EXIT_SUCCESS;
      }
    
    if ((periodTmp <= 0) || (periodTmp > std::numeric_limits<uint16_t>::max()))
      {
	cout << "Query period outside allowed range. Aborting." << endl;
	return EXIT_FAILURE;
      }
    
  }
  catch(std::exception& e) {
    cerr << e.what() << endl;
    cerr << desc << endl;
    return EXIT_FAILURE;
  }
  
  // ----------------------------------
  
  uint16_t  periodMS = (uint16_t) periodTmp;

  // ----------------------------------
  // Install signal handlers
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGINT, signalHandler);
  std::signal(SIGABRT, signalHandler);
  

  // ----------------------------------
  // Register with Vardis
  VardisClientConfiguration cl_conf;
  cl_conf.cmdsock_conf.commandSocketFile = cmdsock_name;
  cl_conf.shm_conf_client.shmAreaName    = shmname_cli;
  cl_conf.shm_conf_global.shmAreaName    = shmname_glob;

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
