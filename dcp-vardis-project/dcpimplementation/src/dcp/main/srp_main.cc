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
#include <dcp/common/sharedmem_configuration.h>
#include <dcp/common/debug_helpers.h>
#include <dcp/bp/bp_queueing_mode.h>
#include <dcp/bp/bpclient_lib.h>
#include <dcp/srp/srp_configuration.h>
#include <dcp/srp/srp_logging.h>
#include <dcp/srp/srp_receiver.h>
#include <dcp/srp/srp_runtime_data.h>
#include <dcp/srp/srp_scrubber.h>
#include <dcp/srp/srp_transmissible_types.h>
#include <dcp/srp/srp_transmitter.h>

using dcp::BP_STATUS_OK;
using dcp::bp_status_to_string;
using dcp::BPClientRuntime;
using dcp::DcpStatus;
using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::size_t;
using dcp::bp::BPStaticClientInfo;

using namespace std::chrono_literals;

namespace po = boost::program_options;

using namespace dcp::srp;

std::string get_protocol_name ()
{
  char buf [500] = "State Reporting Protocol ";

  return std::string(std::string(buf) + dcp::dcpVersionNumber);
}



void print_version ()
{
  cout << dcp::dcpHighlevelDescription
       << " -- " << get_protocol_name()
       << endl;
}

SRPRuntimeData* srp_rt_ptr = nullptr;

void signalHandler (int signum)
{
  BOOST_LOG_SEV(log_main, trivial::info) << "Caught signal code " << signum << " (" << strsignal(signum) << ")";
  BOOST_LOG_SEV(log_main, trivial::info) << "Setting exit flag.";
  if (srp_rt_ptr)
    srp_rt_ptr->srp_exitFlag = true;
}


int run_srp_demon (const std::string cfg_filename)
{
  // read configuration and start logging
  SRPConfiguration srpconfig;
  srpconfig.read_from_config_file (cfg_filename);
  initialize_logging (srpconfig);
  BOOST_LOG_SEV(log_main, trivial::info) << "Demon mode with config file " << cfg_filename;
  BOOST_LOG_SEV(log_main, trivial::info) << "Configuration: " << srpconfig;

  BPStaticClientInfo client_info;
  client_info.protocolId      =  dcp::BP_PROTID_SRP;
  std::strncpy (client_info.protocolName, get_protocol_name().c_str(), dcp::bp::maximumProtocolNameLength);
  client_info.maxPayloadSize         =  sizeof(ExtendedSafetyDataT);
  client_info.queueingMode           =  dcp::bp::BP_QMODE_ONCE;
  client_info.maxEntries             =  0;
  client_info.allowMultiplePayloads  =  false;

  
  try {
    srp_rt_ptr = new SRPRuntimeData (client_info, srpconfig);

    BOOST_LOG_SEV(log_main, trivial::info) << "BP registration successful, ownNodeIdentifier = " << srp_rt_ptr->get_own_node_identifier();

    
    // install signal handlers
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGABRT, signalHandler);

    BOOST_LOG_SEV(log_main, trivial::info) << "Starting threads.";    
    std::thread thread_rx (receiver_thread, std::ref(*srp_rt_ptr));
    std::thread thread_tx (transmitter_thread, std::ref(*srp_rt_ptr));
    std::thread thread_scrub (scrubber_thread, std::ref(*srp_rt_ptr));
    
    // and wait for their end
    BOOST_LOG_SEV (log_main, trivial::info) << "Running ...";
    thread_rx.join ();
    thread_tx.join ();
    thread_scrub.join ();

    delete srp_rt_ptr;
    srp_rt_ptr = nullptr;

    // and exit
    BOOST_LOG_SEV(log_main, trivial::info) << "Exiting.";

    return EXIT_SUCCESS;
    
  }
  catch (std::exception& e)
    {
      BOOST_LOG_SEV(log_main, trivial::fatal) << "Caught an exception, got " << e.what() << ", exiting.";

      if (srp_rt_ptr)
	delete srp_rt_ptr;

      return EXIT_FAILURE;
    }
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
	cout << "Running SRP demon ..." << endl;		
	return run_srp_demon (cfg_filename);
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
