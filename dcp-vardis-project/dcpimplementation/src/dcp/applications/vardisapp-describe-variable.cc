#include <iostream>
#include <dcp/common/debug_helpers.h>
#include <dcp/common/services_status.h>
#include <dcp/vardis/vardisclient_configuration.h>
#include <dcp/vardis/vardisclient_lib.h>

using std::cout;
using std::endl;
using namespace dcp;
using namespace dcp::vardis;

void print_help_and_exit (std::string execname)
{
  cout << execname << " <sockname> <varId>" << endl
       << endl
       << "Describes a Vardis variable." << endl
       << endl
       << "Parameters:" << endl
       << "    sockname:     filename of Vardis command socket (UNIX Domain socket)" << endl
       << "    varId:        variable identifier, unique value between 0 and " << (int) dcp::vardis::VarIdT::max_val() << endl;
  exit (EXIT_SUCCESS);
}

int main (int argc, char* argv [])
{
  // ----------------------------------
  // Check parameters
  
  if (argc != 3)
    print_help_and_exit (std::string (argv[0]));

  std::string sockname (argv[1]);
  int         varIdTmp   (std::stoi(std::string(argv[2])));

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
  cl_conf.shm_conf.shmAreaName           = "irrelevant";

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
