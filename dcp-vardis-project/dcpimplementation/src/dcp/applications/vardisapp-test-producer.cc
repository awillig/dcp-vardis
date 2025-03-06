#include <chrono>
#include <csignal>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <thread>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/program_options.hpp>
#include <dcp/applications/vardisapp-test-variabletype.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/services_status.h>
#include <dcp/vardis/vardis_constants.h>
#include <dcp/vardis/vardis_transmissible_types.h>
#include <dcp/vardis/vardisclient_configuration.h>
#include <dcp/vardis/vardisclient_lib.h>



using std::cout;
using std::endl;
using std::cerr;
using dcp::vardis::defaultVardisCommandSocketFileName;
using dcp::vardis::defaultVardisStoreShmName;

using namespace dcp;


const std::string defaultVardisClientShmName = "shm-vardisapp-test-producer";


void print_version ()
{
  cout << dcp::dcpHighlevelDescription
       << " -- version " << dcp::dcpVersionNumber
       << endl;
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
  std::string cmdsock_name  = defaultVardisCommandSocketFileName;
  std::string shmname_cli   = defaultVardisClientShmName;
  std::string shmname_glob  = defaultVardisStoreShmName;
  int     varIdTmp = 0;
  int     periodTmp = 0;
  double  average = 0;
  double  stddev = 0;

  
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h",         "produce help message and exit")
    ("version,v",      "show version information and exit")
    ("sockname,s",     po::value<std::string>(&cmdsock_name)->default_value(defaultVardisCommandSocketFileName), "filename of VarDis command socket (UNIX Domain Socket)")
    ("shmcli,c",       po::value<std::string>(&shmname_cli)->default_value(defaultVardisClientShmName), "Name of shared memory area for interfacing with Vardis")
    ("shmgdb,g",       po::value<std::string>(&shmname_glob)->default_value(defaultVardisStoreShmName), "Unique name of shared memory area for accessing VarDis variables (global database)")
    ("varid",          po::value<int>(&varIdTmp), "Variable identifier")
    ("period",         po::value<int>(&periodTmp), "Generation period (in ms)")
    ("average",        po::value<double>(&average), "Average of generated Gaussian")
    ("stddev",         po::value<double>(&stddev), "Standard deviation of generated Gaussian")
    ;

  po::positional_options_description desc_pos;
  desc_pos.add ("varid", 1);
  desc_pos.add ("period", 1);
  desc_pos.add ("average", 1);
  desc_pos.add ("stddev", 1);

  try {
    po::variables_map vm;
    //po::store(po::parse_command_line(argc, argv, desc), vm);
    po::store (po::command_line_parser(argc, argv).options(desc).positional(desc_pos).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
      {
	cout << std::string (argv[0]) << " [-s <sockname>] [-mc <shmcli>] [-mg <shmgdb>] <varId> <genperiodMS> <average> <stddev>" << endl;
	cout << desc << endl;
	return EXIT_SUCCESS;
      }

    if (vm.count("version"))
      {
	print_version();
	return EXIT_SUCCESS;
      }
    
    if ((varIdTmp < 0) || (varIdTmp > dcp::vardis::VarIdT::max_val()))
      {
	cout << "Varid outside allowed range. Aborting." << endl;
	return EXIT_FAILURE;
      }
    
    if ((periodTmp <= 0) || (periodTmp > std::numeric_limits<uint16_t>::max()))
      {
	cout << "Generation period outside allowed range. Aborting." << endl;
	return EXIT_FAILURE;
      }
    
    if (stddev < 0)
      {
	cout << "Stddev outside allowed range. Aborting." << endl;
	return EXIT_FAILURE;
      }
    
  }
  catch(std::exception& e) {
    cerr << e.what() << endl;
    cerr << desc << endl;
    return EXIT_FAILURE;
  }

  // ----------------------------------

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
  cl_conf.cmdsock_conf.commandSocketFile = cmdsock_name;
  cl_conf.shm_conf_client.shmAreaName    = shmname_cli;
  cl_conf.shm_conf_global.shmAreaName    = shmname_glob;

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
