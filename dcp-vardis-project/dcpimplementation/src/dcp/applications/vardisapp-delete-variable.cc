#include <iostream>
#include <dcp/common/services_status.h>
#include <dcp/vardis/vardisclient_configuration.h>
#include <dcp/vardis/vardisclient_lib.h>

using std::cout;
using std::endl;
using namespace dcp;

void print_help_and_exit (std::string execname)
{
  cout << execname << " <sockname> <shmname> <varId>" << endl
       << endl
       << "Deletes a Vardis variable." << endl
       << endl
       << "Parameters:" << endl
       << "    sockname:     filename of Vardis command socket (UNIX Domain socket)" << endl
       << "    shmname:      unique name of shared memory area for interfacing with VarDis" << endl
       << "    varId:        variable identifier, unique value between 0 and " << (int) dcp::vardis::VarIdT::max_val() << endl;
  exit (EXIT_SUCCESS);
}

int main (int argc, char* argv [])
{
  // ----------------------------------
  // Check parameters
  
  if (argc != 4)
    print_help_and_exit (std::string (argv[0]));

  std::string sockname (argv[1]);
  std::string shmname  (argv[2]);
  int         varIdTmp   (std::stoi(std::string(argv[3])));

  if ((varIdTmp < 0) || (varIdTmp > dcp::vardis::VarIdT::max_val()))
    {
      cout << "Parameter varId outside allowed range. Aborting." << endl;
      return EXIT_FAILURE;
    }

  VarIdT    varId (varIdTmp);
  
  // ----------------------------------
  // Register with Vardis and delete variable
  VardisClientConfiguration cl_conf;
  cl_conf.cmdsock_conf.commandSocketFile = sockname;
  cl_conf.shm_conf.shmAreaName           = shmname;

  try {
    VardisClientRuntime cl_rt (cl_conf);

    DcpStatus delete_status = cl_rt.rtdb_delete (varId);
    if (delete_status != VARDIS_STATUS_OK)
      {
	cout << "Deleting variable " << varId << " failed with status " << vardis_status_to_string (delete_status) << ", Exiting." << endl;
	return EXIT_FAILURE;
      }

    cout << "Deleted variable " << varId << " successfully, exiting." << endl;
  }
  catch (std::exception& e)
    {
      cout << "Caught an exception, got " << e.what() << ", exiting." << endl;
      return EXIT_FAILURE;
    }  
    
  return EXIT_SUCCESS;
}
