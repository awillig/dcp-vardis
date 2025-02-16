#include <chrono>
#include <csignal>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <thread>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <dcp/applications/vardisapp-test-variabletype.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/services_status.h>
#include <dcp/vardis/vardis_transmissible_types.h>
#include <dcp/vardis/vardisclient_configuration.h>
#include <dcp/vardis/vardisclient_lib.h>


using std::cout;
using std::endl;
using namespace dcp;

void print_help_and_exit (std::string execname)
{
  cout << execname << " <sockname> <shmname> <varId> <genperiodMS> <average> <stddev>" << endl
       << endl
       << "Creates and periodically updates a Vardis variable. The variable includes the" << endl
       << "generation timestamp and random values generated from a Gaussian distribution" << endl
       << "with given average and standard deviation" << endl
       << endl
       << "Parameters:" << endl
       << "    sockname:     filename of Vardis command socket (UNIX Domain socket)" << endl
       << "    shmname:      unique name of shared memory area for interfacing with VarDis" << endl
       << "    varId:        variable identifier, unique value between 0 and " << (int) dcp::vardis::VarIdT::max_val() << endl
       << "    genPeriodMS:  time period between two variable updates (in ms, positive, no more than 65 s)" << endl
       << "    average:      average value of the Gaussian" << endl
       << "    stddev:       standard deviation of the Gaussian" << endl;
  exit (EXIT_SUCCESS);
}

bool       exitFlag = false;
uint64_t   seqno    = 0;

boost::mt19937 generator;

void signalHandler (int signum)
{
  cout << "Caught signal code " << signum << " (" << strsignal(signum) << "). Exiting." << endl;
  exitFlag = true;
}


VardisTestVariable generate_new_value (boost::normal_distribution<> distribution)
{
  VardisTestVariable rv;
  rv.seqno  = seqno++;
  rv.tstamp = TimeStampT::get_current_system_time ();
  rv.value  = distribution (generator);

  return rv;
}


int main (int argc, char* argv [])
{
  // ----------------------------------
  // Check parameters
  
  if (argc != 7)
    print_help_and_exit (std::string (argv[0]));

  std::string sockname (argv[1]);
  std::string shmname  (argv[2]);
  int         varIdTmp   (std::stoi(std::string(argv[3])));
  int         periodTmp  (std::stoi(std::string(argv[4])));
  double      average    (std::stod(std::string(argv[5])));
  double      stddev     (std::stod(std::string(argv[6])));

  if ((varIdTmp < 0) || (varIdTmp > dcp::vardis::VarIdT::max_val()))
    {
      cout << "Parameter varId outside allowed range. Aborting." << endl;
      return EXIT_FAILURE;
    }

  if ((periodTmp <= 0) || (periodTmp > std::numeric_limits<uint16_t>::max()))
    {
      cout << "Parameter genPeriodMS outside allowed range. Aborting." << endl;
      return EXIT_FAILURE;
    }

  if (stddev <= 0)
    {
      cout << "Parameter stddev outside allowed range. Aborting." << endl;
      return EXIT_FAILURE;
    }

  VarIdT    varId (varIdTmp);
  uint16_t  periodMS = (uint16_t) periodTmp;
  boost::normal_distribution<> distribution (average, stddev);

  // ----------------------------------
  // Install signal handlers
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGINT, signalHandler);
  std::signal(SIGABRT, signalHandler);
  

  // ----------------------------------
  // Register with Vardis and create variable
  VardisClientConfiguration cl_conf;
  cl_conf.cmdsock_conf.commandSocketFile = sockname;
  cl_conf.shm_conf.shmAreaName           = shmname;

  try {
    VardisClientRuntime cl_rt (cl_conf);

    std::stringstream ssDescr;
    ssDescr << "vardisapp-testvariable-varId = " << (int) varId.val;
  
    VarSpecT spec;
    spec.varId   = varId;
    spec.prodId  = cl_rt.get_own_node_identifier();
    spec.repCnt  = 1;
    spec.descr   = StringT (ssDescr.str());
    
    VardisTestVariable initial_varval = generate_new_value (distribution);
    VarValueT initial_value (sizeof(VardisTestVariable), (byte*) &initial_varval);
    
    DcpStatus create_status = cl_rt.rtdb_create (spec, initial_value);
    if (create_status != VARDIS_STATUS_OK)
      {
	cout << "Creation of Vardis variable failed with status " << vardis_status_to_string (create_status) << ", exiting." << endl;
	return EXIT_FAILURE;
      }
  
    // ============================================
    // Main loop
    // ============================================
    
    cout << "Successfully registered variable " << varId << ", entering update loop. Stop with <Ctrl-C>." << endl;
    
    while (not exitFlag)
      {
	std::this_thread::sleep_for (std::chrono::milliseconds (periodMS));
	
	VardisTestVariable varval = generate_new_value (distribution);
	VarValueT value (sizeof(VardisTestVariable), (byte*) &varval);
	
	DcpStatus update_status = cl_rt.rtdb_update (varId, value);
	if (update_status != VARDIS_STATUS_OK)
	  {
	    cout << "Update of Vardis variable failed with status " << vardis_status_to_string (update_status) << ", exiting." << endl;
	    exitFlag = true;
	  }
      }
    

    // ============================================
    // Leave: delete variable
    // ============================================
    
    DcpStatus delete_status = cl_rt.rtdb_delete (varId);
    if (delete_status != VARDIS_STATUS_OK)
      {
	cout << "Deleting variable failed with status " << vardis_status_to_string (delete_status) << ", exiting." << endl;
	return EXIT_FAILURE;
      }
    
    
    return EXIT_SUCCESS;
  }
  catch (std::exception& e)
    {
      cout << "Caught an exception, got " << e.what() << ", exiting." << endl;
      return EXIT_FAILURE;
    }  
}
