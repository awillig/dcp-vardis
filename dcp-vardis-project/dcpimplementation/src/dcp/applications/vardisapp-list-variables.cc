#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <limits>
#include <list>
#include <thread>
#include <boost/program_options.hpp>
#include <dcp/common/exceptions.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/other_helpers.h>
#include <dcp/vardis/vardis_constants.h>
#include <dcp/vardis/vardis_service_primitives.h>
#include <dcp/vardis/vardis_transmissible_types.h>
#include <dcp/vardis/vardisclient_configuration.h>
#include <dcp/vardis/vardisclient_lib.h>


using std::cerr;
using std::cout;
using std::endl;
using dcp::vardis::defaultVardisStoreShmName;
using dcp::vardis::defaultVardisCommandSocketFileName;

using namespace dcp;


void print_version ()
{
  cout << dcp::dcpHighlevelDescription
       << " -- version " << dcp::dcpVersionNumber
       << endl;
}


int main (int argc, char* argv [])
{
  std::string cmdsock_name  = defaultVardisCommandSocketFileName;
  std::string shmname_cli   = "irrelevant";
  std::string shmname_glob  = defaultVardisStoreShmName;

  
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h",         "produce help message and exit")
    ("version,v",      "show version information and exit")
    ("sockname,s",     po::value<std::string>(&cmdsock_name)->default_value(defaultVardisCommandSocketFileName), "filename of VarDis command socket (UNIX Domain Socket)")
    ("shmgdb,g",       po::value<std::string>(&shmname_glob)->default_value(defaultVardisStoreShmName), "Unique name of shared memory area for accessing VarDis variables (global database)")
    ;

  try {
    po::variables_map vm;
    //po::store(po::parse_command_line(argc, argv, desc), vm);
    po::store (po::command_line_parser(argc, argv).options(desc).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
      {
	cout << std::string (argv[0]) << " [-s <sockname>]" << endl;
	cout << desc << endl;
	return EXIT_SUCCESS;
      }

    if (vm.count("version"))
      {
	print_version();
	return EXIT_SUCCESS;
      }    
  }
  catch(std::exception& e) {
    cerr << argv[0] << ": option error. Exiting." << endl;
    cerr << e.what() << endl;
    cerr << desc << endl;
    return EXIT_FAILURE;
  }

  // ----------------------------------
  // Register with Vardis
  VardisClientConfiguration cl_conf;
  cl_conf.cmdsock_conf.commandSocketFile = cmdsock_name;
  cl_conf.shm_conf_client.shmAreaName    = shmname_cli;;
  cl_conf.shm_conf_global.shmAreaName    = shmname_glob;

  try {
    VardisClientRuntime cl_rt (cl_conf, false);

    std::list<DescribeDatabaseVariableDescription> db_list;
    
    DcpStatus dd_status = cl_rt.describe_database (db_list);
    if (dd_status != VARDIS_STATUS_OK)
      {
	cout << "Obtaining database description failed with status " << vardis_status_to_string (dd_status) << ", Exiting." << endl;
	return EXIT_FAILURE;
      }
    
    for (const auto& descr : db_list)
      {
	cout << "varId = " << descr.varId
   	     << ", prodId = " << descr.prodId
	     << ", repCnt = " << descr.repCnt
	     << ", descr = " << descr.description
	     << ", tStamp = " << descr.tStamp
	     << ", toBeDeleted = " << descr.toBeDeleted
	<< endl;
      } 
  }
  catch (DcpException& e)
    {
      print_exiting_dcp_exception (e);
      return EXIT_FAILURE;
    }
  catch (std::exception& e)
    {
      cout << "Caught an exception, got " << e.what() << ", exiting." << endl;
      return EXIT_FAILURE;
    }  

  return EXIT_SUCCESS;
}
