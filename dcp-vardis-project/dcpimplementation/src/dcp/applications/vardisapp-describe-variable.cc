#include <iostream>
#include <dcp/common/debug_helpers.h>
#include <dcp/common/services_status.h>
#include <dcp/vardis/vardisclient_configuration.h>
#include <dcp/vardis/vardisclient_lib.h>

using std::cerr;
using std::cout;
using std::endl;

using namespace dcp;
using namespace dcp::vardis;


void print_version ()
{
  cout << dcp::dcpHighlevelDescription
       << " -- version " << dcp::dcpVersionNumber
       << endl;
}

void output_cmdline_guidance (char* argv[])
{
  cout << std::string (argv[0]) << " [-s <sockname>] <varid>" << endl;
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
	cout << std::string (argv[0]) << " [-s <sockname>]" << endl;
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
    cerr << argv[0] << ": option error. Exiting." << endl;
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
    VardisClientRuntime cl_rt (cl_conf, false);
    DescribeVariableDescription var_descr;
    byte buffer [MAX_maxValueLength + 128];

    DcpStatus describe_status = cl_rt.describe_variable (varId, var_descr, buffer);
    if (describe_status != VARDIS_STATUS_OK)
      {
	cout << "Describing variable " << varId << " failed with status " << vardis_status_to_string (describe_status) << ", Exiting." << endl;
	return EXIT_FAILURE;
      }

    cout << "Describing variable:\n"
	 << "   varId        = " << (int) var_descr.varId.val  << "\n"
	 << "   prodId       = " << var_descr.prodId << "\n"
	 << "   repCnt       = " << (int) var_descr.repCnt.val << "\n"
	 << "   description  = " << var_descr.description << "\n"
	 << "   seqno        = " << (int) var_descr.seqno.val << "\n"
	 << "   tStamp       = " << var_descr.tStamp << "\n"
	 << "   countUpdate  = " << (int) var_descr.countUpdate.val << "\n"
	 << "   countCreate  = " << (int) var_descr.countCreate.val << "\n"
	 << "   countDelete  = " << (int) var_descr.countDelete.val << "\n"
	 << std::boolalpha
	 << "   toBeDeleted  = " << var_descr.toBeDeleted << "\n"
	 << "   value_length = " << (int) var_descr.value_length.val << "\n"
	 << "   data         = " << byte_array_to_string (buffer, std::min (32, (int) var_descr.value_length.val)) << "\n"
	 << endl;
    
  }
  catch (std::exception& e)
    {
      cout << "Caught an exception, got " << e.what() << ", exiting." << endl;
      return EXIT_FAILURE;
    }  
    
  return EXIT_SUCCESS;
}
