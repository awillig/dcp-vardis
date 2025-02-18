#include <cstdlib>
#include <iostream>
#include <exception>
#include <csignal>
#include <signal.h>
#include <thread>
#include <chrono>
#include <boost/program_options.hpp>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/services_status.h>
#include <dcp/common/shared_mem_area.h>
#include <dcp/common/debug_helpers.h>
#include <dcp/bp/bp_queueing_mode.h>
#include <dcp/bp/bpclient_lib.h>
#include <dcp/srp/srp_configuration.h>
#include <dcp/srp/srp_transmissible_types.h>

using std::cout;
using std::cerr;
using std::endl;
using std::size_t;
using std::exception;
using dcp::DcpStatus;
using dcp::BP_STATUS_OK;
using dcp::BPClientRuntime;
using dcp::bp_status_to_string;

using namespace std::chrono_literals;

namespace po = boost::program_options;

using namespace dcp::srp;


std::string get_protocol_name ()
{
  char buf [500] = "State Reporting Protocol (SRP) -- Version ";

  return std::string(std::string(buf) + dcp::dcpVersionNumber);
}


void print_version ()
{
  cout << dcp::dcpHighlevelDescription
       << " -- " << get_protocol_name() << endl;
}


bool srp_exitFlag = false;

void signalHandler (int signum)
{
  cout << "Caught signal " << signum << " (" << strsignal(signum) << ")" << endl;
  srp_exitFlag = true;
}



void payload_generator (BPClientRuntime& cl_rt)
{
  byte txdata [40];
  byte currval = 0x20; 

  while (not srp_exitFlag)
    {
      std::this_thread::sleep_for (3000ms);

      for (byte i=0; i<40; i++)
	txdata[i] = (currval + i) % 256;
      currval = (currval + 1) % 256;

      // cout << "payload_generator: transmitting payload " << byte_array_to_string (txdata, 40) << endl;
      
      DcpStatus stat = cl_rt.transmit_payload (40, txdata);

      if (stat != BP_STATUS_OK)
	cout << "payload_generator: returned status " << bp_status_to_string (stat) << endl;
    }
}


int run_srp_main (std::string cfg_filename)
{
  // read configuration
  SRPConfiguration srpconfig;
  srpconfig.read_from_config_file (cfg_filename);
  cout << "Configuration: " << srpconfig << endl;
  
  // install signal handlers
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGINT, signalHandler);
  std::signal(SIGABRT, signalHandler);
  
  try {
    BPClientRuntime cl_rt (dcp::BP_PROTID_SRP,
			   get_protocol_name(),
			   ExtendedSafetyDataT::fixed_size(),
			   dcp::bp::BP_QMODE_REPEAT,
			   0,
			   false,  // allowMultiplePayloads
			   false,  // generateTransmitPayloadConfirms
			   srpconfig);

    std::thread thread_generator (payload_generator, std::ref(cl_rt));
    
    while (not srp_exitFlag)
      {
	std::this_thread::sleep_for (50ms);
	
	BPLengthT result_length = 0;
	byte buffer [4000];
	DcpStatus rx_stat = cl_rt.receive_payload (result_length, buffer);
	if ((result_length > 0) && rx_stat == BP_STATUS_OK)
	  {
	    cout << "Rx thread: Got payload, printing first few bytes: " << byte_array_to_string (buffer, 15) << endl;
	  }
	else
	  {
	    if (rx_stat != BP_STATUS_OK)
	      cout << "Rx thread: got rx_stat = " << bp_status_to_string (rx_stat) << endl;
	  }
      }
  
    thread_generator.join ();
  }
  catch (std::exception& e)
    {
      cout << "Caught an exception, got " << e.what() << ", exiting." << endl;
      return EXIT_FAILURE;
    }  

  return EXIT_SUCCESS;
}



int main (int argc, char* argv[])
{

  std::string cfg_filename;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h",     "produce help message and exit")
    ("version,v",  "show version information and exit")
    ("cfghelp,c",  "produce help message for config file format and exit")
    ("run,r",      po::value<std::string>(&cfg_filename), "run SRP program with given configuration file")
    ;
  
  try {
    
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
      {
	cout << desc << endl;
	return EXIT_SUCCESS;
      }


    if (vm.count("version"))
      {
	print_version();
	return EXIT_SUCCESS;
      }

    if (vm.count("cfghelp"))
      {
	SRPConfiguration cfg;
	auto cfgdesc = cfg.construct_options_description();
	cout << cfgdesc << endl;
	return EXIT_SUCCESS;
      }

    if (vm.count("run"))
      {
	return run_srp_main(cfg_filename);
      }

    cerr << "No valid option given." << endl;
    cerr << desc << endl;
    return EXIT_FAILURE;
  }
  catch(std::exception& e) {
    cerr << "Caught an exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
