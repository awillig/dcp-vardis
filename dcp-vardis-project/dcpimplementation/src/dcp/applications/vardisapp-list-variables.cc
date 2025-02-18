#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <limits>
#include <list>
#include <thread>
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
  cout << execname << " <sockname>" << endl
       << endl
       << "Retrieves list of currently known variables from Vardis instance and prints it." << endl
       << endl
       << "Parameters:" << endl
       << "    sockname:       filename of Vardis command socket (UNIX Domain socket)" << endl;
  exit (EXIT_SUCCESS);
}


int main (int argc, char* argv [])
{
  // ----------------------------------
  // Check parameters
  
  if (argc != 2)
    print_help_and_exit (std::string (argv[0]));

  std::string sockname (argv[1]);

  // ----------------------------------
  // Register with Vardis
  VardisClientConfiguration cl_conf;
  cl_conf.cmdsock_conf.commandSocketFile = sockname;

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
  catch (std::exception& e)
    {
      cout << "Caught an exception, got " << e.what() << ", exiting." << endl;
      return EXIT_FAILURE;
    }  

  return EXIT_SUCCESS;
}
