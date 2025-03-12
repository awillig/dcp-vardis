#include <cstdlib>
#include <iostream>
#include <exception>
#include <csignal>
#include <signal.h>
#include <thread>
#include <list>
#include <unistd.h>
#include <boost/program_options.hpp>
#include <dcp/common/exceptions.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/services_status.h>
#include <dcp/bp/bpclient_configuration.h>
#include <dcp/bp/bp_configuration.h>
#include <dcp/bp/bp_runtime_data.h>
#include <dcp/bp/bp_management_command.h>
#include <dcp/bp/bp_management_payload.h>
#include <dcp/bp/bp_transmitter.h>
#include <dcp/bp/bp_receiver.h>
#include <dcp/bp/bp_logging.h>
#include <dcp/bp/bpclient_lib.h>


using dcp::BPClientConfiguration;
using dcp::BPClientRuntime;
using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::size_t;
using dcp::DcpException;

namespace po = boost::program_options;

using namespace dcp::bp;


void print_version ()
{
  cout << dcp::dcpHighlevelDescription
       << " -- Beaconing Protocol (BP) -- Version " << dcp::dcpVersionNumber
       << endl;
}


BPRuntimeData* pRuntime = nullptr;

void signalHandler (int signum)
{
  BOOST_LOG_SEV(log_main, trivial::info) << "Caught signal code " << signum << " (" << strsignal(signum) << ")";
  BOOST_LOG_SEV(log_main, trivial::info) << "Setting exit flag.";
  if (pRuntime)
    pRuntime->bp_exitFlag = true;
}


void run_bp_demon (const std::string cfg_filename)
{
  // read configuration and start logging
  BPConfiguration bpconfig;
  bpconfig.read_from_config_file (cfg_filename);
  initialize_logging (bpconfig.logging_conf);
  BOOST_LOG_SEV(log_main, trivial::info) << "Demon mode with config file " << cfg_filename; 
  BOOST_LOG_SEV(log_main, trivial::info) << "Configuration: " << bpconfig;
  BOOST_LOG_SEV(log_main, trivial::info) << "uid = " << getuid() << ", euid = " << geteuid();
  
  // create runtime data
  pRuntime = new BPRuntimeData (bpconfig);
  BOOST_LOG_SEV(log_main, trivial::info) << "Own node identifier (MAC address): " << pRuntime->ownNodeIdentifier;
  
  // install signal handlers
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGINT, signalHandler);
  std::signal(SIGABRT, signalHandler);
  
  // start threads
  BOOST_LOG_SEV(log_main, trivial::info) << "Starting threads.";
  std::thread thread_mgmt_command (management_thread_command, std::ref(*pRuntime));
  std::thread thread_mgmt_payload (management_thread_payload, std::ref(*pRuntime));
  std::thread thread_tx (transmitter_thread, std::ref(*pRuntime));
  std::thread thread_rx (receiver_thread, std::ref(*pRuntime));

  
  // and wait for their end
  BOOST_LOG_SEV(log_main, trivial::info) << "Running ...";
  thread_mgmt_command.join();
  thread_mgmt_payload.join();
  thread_tx.join();
  thread_rx.join();  // possibly not wait on this one so we can escape from next_packet()? (libpcap does not support timeouts)
  
  // and exit
  BOOST_LOG_SEV(log_main, trivial::info) << "Exiting.";
}

enum MgmtCommand { Shutdown, Activate, Deactivate, Stats };

void run_bp_management_command (MgmtCommand cmd, const std::string cfg_filename)
{
  BPClientConfiguration bpconfig;
  bpconfig.read_from_config_file (cfg_filename, true);
  
  BPClientRuntime cl_rt (0, "ephemeral", 100, bpconfig);
  
  DcpStatus sd_status;
  switch (cmd)
    {
    case Shutdown:     sd_status = cl_rt.shutdown_bp (); break;
    case Activate:     sd_status = cl_rt.activate_bp (); break;
    case Deactivate:   sd_status = cl_rt.deactivate_bp (); break;
    case Stats:
      {
	double avg_inter_beacon_time;
	double avg_beacon_size;
	unsigned int number_received_payloads;
	sd_status = cl_rt.get_runtime_statistics (avg_inter_beacon_time, avg_beacon_size, number_received_payloads);
	if (sd_status == BP_STATUS_OK)
	  {
	    cout << "Average inter-beacon time (ms):      " << avg_inter_beacon_time << endl;
	    cout << "Average beacon size (B):             " << avg_beacon_size << endl;
	    cout << "Number received payloads:            " << number_received_payloads << endl;
	    
	    if (avg_inter_beacon_time > 0)
	      {
		cout << "Average data reception rate (B/s):   " << avg_beacon_size / (avg_inter_beacon_time / 1000) << endl;
	      }
	  }
	break;
      }
    default:           cout << "Unknown type of management command: " << cmd << endl; return;
    }
  cout << "BP return status = " << bp_status_to_string (sd_status) << endl;
}



void run_query_client_protocols (const std::string cfg_filename)
{
  BPClientConfiguration bpconfig;
  bpconfig.read_from_config_file (cfg_filename, true);

  BPClientRuntime cl_rt (0, "ephemeral", 100, bpconfig);

  std::list<BPRegisteredProtocolDataDescription> descr_list;
  DcpStatus qcp_status = cl_rt.list_registered_protocols (descr_list);

  if (qcp_status == BP_STATUS_OK)
    {
      if (descr_list.size() > 0)
	{
	  cout << "Query client protocols: " << descr_list.size() << " protocols currently registered:" << endl;
	  for (auto it = descr_list.begin(); it != descr_list.end(); ++it)
	    {
	      cout << "Protocol " << it->protocolName << " (protocolId = " << it->protocolId << "):" << endl
		   << "    maxPayloadSize                 = " << it->maxPayloadSize << endl
		   << "    queueingMode                   = " << bp_queueing_mode_to_string(it->queueingMode) << endl
		   << "    timeStampRegistration          = " << it->timeStampRegistration << endl
		   << "    maxEntries                     = " << it->maxEntries << endl
		   << "    allowMultiplePayloads          = " << it->allowMultiplePayloads << endl
		   << "    cntOutgoingPayloads            = " << it->cntOutgoingPayloads << endl
		   << "    cntReceivedPayloads            = " << it->cntReceivedPayloads << endl
		   << "    cntDroppedOutgoingPayloads     = " << it->cntDroppedOutgoingPayloads << endl
		   << "    cntDroppedIncomingPayloads     = " << it->cntDroppedIncomingPayloads << endl
		;
	    }
	}
      else
	{
	  cout << "Query client protocols: No client protocols registered." << endl;
	}
    }
  else
    {
      cout << "Query client protocols: return status = " << bp_status_to_string (qcp_status) << endl;
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
    ("querycp,q",   po::value<std::string>(&cfg_filename), "queries current client protocols registered with BP using given config file")
    ("run,r",       po::value<std::string>(&cfg_filename), "run BP protocol with given config file")
    ("shutdown,s",  po::value<std::string>(&cfg_filename), "send shutdown command to running demon using given config file")
    ("activate,a",   po::value<std::string>(&cfg_filename), "send activate command to running demon using given config file")
    ("deactivate,d", po::value<std::string>(&cfg_filename), "send deactivate command to running demon using given config file")
    ("runtimestats,t",  po::value<std::string>(&cfg_filename), "show BP runtime statistics and exit")
    ;
  
  try {
    
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))     { cout << desc << endl; return EXIT_SUCCESS; }
    if (vm.count("version"))  {	print_version(); return EXIT_SUCCESS; }
    if (vm.count("cfghelp"))
      {
	BPConfiguration cfg;
	auto cfgdesc = cfg.construct_options_description();
	cout << cfgdesc << endl;
	return EXIT_SUCCESS;
      }
    
    if (vm.count("run")) { cout << "Running BP demon ..." << endl; run_bp_demon (cfg_filename);	return EXIT_SUCCESS; }
    if (vm.count("shutdown"))     { run_bp_management_command (Shutdown, cfg_filename); return EXIT_SUCCESS; }
    if (vm.count("activate"))     { run_bp_management_command (Activate, cfg_filename); return EXIT_SUCCESS; }
    if (vm.count("deactivate"))   { run_bp_management_command (Deactivate, cfg_filename);	return EXIT_SUCCESS; }
    if (vm.count("querycp"))      { run_query_client_protocols (cfg_filename); return EXIT_SUCCESS; }

    if (vm.count("runtimestats")) { run_bp_management_command (Stats, cfg_filename); return EXIT_SUCCESS; }
    
    cerr << "No valid option given." << endl;
    cerr << desc << endl;
    return EXIT_FAILURE;

  }
  catch (DcpException& e)
    {
      cout << "DCP ERROR - " << e.what() << endl;
      cout << "Exiting." << endl;
      return EXIT_FAILURE;
    }
  catch (exception& e) {
    cerr << e.what() << endl;
    cerr << desc << endl;
    return EXIT_FAILURE;
  }
  

  
  return EXIT_SUCCESS;
}

