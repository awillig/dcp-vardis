#include <iostream>
#include <dcp/common/exceptions.h>
#include <dcp/common/other_helpers.h>
#include <dcp/common/services_status.h>
#include <dcp/vardis/vardisclient_configuration.h>
#include <dcp/vardis/vardisclient_lib.h>

using std::cerr;
using std::cout;
using std::endl;

using dcp::vardis::defaultVardisCommandSocketFileName;
using dcp::vardis::defaultVardisStoreShmName;

using namespace dcp;

const std::string defaultVardisClientShmName = "shm-vardisapp-delete-variable";

void print_version ()
{
  cout << dcp::dcpHighlevelDescription
       << " -- version " << dcp::dcpVersionNumber
       << endl;
}

void output_cmdline_guidance (char* argv[])
{
  cout << std::string (argv[0]) << " [-s <sockname>] [-mg <shmgdb>] <varId>" << endl;
}


int main (int argc, char* argv [])
{
  std::string cmdsock_name  = defaultVardisCommandSocketFileName;
  std::string shmname_cli   = "irrelevant";
  std::string shmname_glob  = defaultVardisStoreShmName;
  int     varIdTmp = 0;
  
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h",         "produce help message and exit")
    ("version,v",      "show version information and exit")
    ("sockname,s",     po::value<std::string>(&cmdsock_name)->default_value(defaultVardisCommandSocketFileName), "filename of VarDis command socket (UNIX Domain Socket)")
    ("shmgdb,g",       po::value<std::string>(&shmname_glob)->default_value(defaultVardisStoreShmName), "Unique name of shared memory area for accessing VarDis variables (global database)")
    ("varid",          po::value<int>(&varIdTmp), "Variable identifier")
    ;

  po::positional_options_description desc_pos;
  desc_pos.add ("varid", 1);

  try {
    po::variables_map vm;
    po::store (po::command_line_parser(argc, argv).options(desc).positional(desc_pos).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
      {
	output_cmdline_guidance(argv);
	cout << desc << endl;
	return EXIT_SUCCESS;
      }

    if (vm.count("version"))
      {
	print_version();
	return EXIT_SUCCESS;
      }

    if (vm.count("varid") == 0)
      {
	cout << "Insufficient arguments." << endl;
	output_cmdline_guidance (argv);
	cout << desc << endl;
	return EXIT_FAILURE;
      }
    
    if ((varIdTmp < 0) || (varIdTmp > dcp::vardis::VarIdT::max_val()))
      {
	cout << "Varid outside allowed range. Aborting." << endl;
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
  
  // ----------------------------------
  // Register with Vardis and delete variable
  VardisClientConfiguration cl_conf;
  cl_conf.cmdsock_conf.commandSocketFile = cmdsock_name;
  cl_conf.shm_conf_client.shmAreaName    = shmname_cli;
  cl_conf.shm_conf_global.shmAreaName    = shmname_glob;


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
