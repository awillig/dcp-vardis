#include <boost/log/trivial.hpp>
#include <cstdlib>
#include <iostream>
#include <exception>
#include <csignal>
#include <signal.h>
#include <sys/types.h>
#include <thread>
#include <list>
#include <boost/program_options.hpp>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/other_helpers.h>
#include <dcp/common/services_status.h>
#include <dcp/bp/bpclient_lib.h>
#include <dcp/vardis/vardis_configuration.h>
#include <dcp/vardis/vardis_logging.h>
#include <dcp/vardis/vardis_management_command.h>
#include <dcp/vardis/vardis_management_rtdb.h>
#include <dcp/vardis/vardis_receiver.h>
#include <dcp/vardis/vardis_runtime_data.h>
#include <dcp/vardis/vardis_transmitter.h>
#include <dcp/vardis/vardisclient_lib.h>


using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::size_t;
using dcp::DcpException;
using dcp::VardisClientRuntime;
using dcp::VardisClientConfiguration;
using dcp::vardis_status_to_string;
using dcp::bp::BPStaticClientInfo;


namespace po = boost::program_options;

using namespace dcp::vardis;


std::string get_protocol_name ()
{
  char buf [500] = "Variable Dissemination Protocol ";

  return std::string(std::string(buf) + dcp::dcpVersionNumber);
}



void print_version ()
{
  cout << dcp::dcpHighlevelDescription
       << " -- " << get_protocol_name()
       << endl;
}

VardisRuntimeData* vd_rt_ptr = nullptr;

void signalHandler (int signum)
{
  BOOST_LOG_SEV(log_main, trivial::info) << "Caught signal code " << signum << " (" << strsignal(signum) << ")";
  BOOST_LOG_SEV(log_main, trivial::info) << "Setting exit flag.";
  if (vd_rt_ptr)
    vd_rt_ptr->vardis_exitFlag = true;
}


int run_vardis_demon (const std::string cfg_filename)
{
  // read configuration and start logging
  VardisConfiguration vdconfig;
  vdconfig.read_from_config_file (cfg_filename);
  initialize_logging (vdconfig);
  BOOST_LOG_SEV(log_main, trivial::info) << "Demon mode with config file " << cfg_filename; 
  BOOST_LOG_SEV(log_main, trivial::info) << "Configuration: " << vdconfig;

  BPStaticClientInfo client_info;
  client_info.protocolId      =  dcp::BP_PROTID_VARDIS;
  std::strncpy (client_info.protocolName, get_protocol_name().c_str(), dcp::bp::maximumProtocolNameLength);
  client_info.maxPayloadSize         =  vdconfig.vardis_conf.maxPayloadSize;
  client_info.queueingMode           =  dcp::bp::BP_QMODE_QUEUE_DROPHEAD;
  client_info.maxEntries             =  vdconfig.vardis_conf.queueMaxEntries;
  client_info.allowMultiplePayloads  =  false;

  
  try {
    vd_rt_ptr = new VardisRuntimeData (client_info, vdconfig);

    BOOST_LOG_SEV(log_main, trivial::info) << "BP registration successful, ownNodeIdentifier = " << vd_rt_ptr->get_own_node_identifier();
    
    // install signal handlers
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGABRT, signalHandler);
    
    // start threads
    BOOST_LOG_SEV(log_main, trivial::info) << "Starting threads.";
    std::thread thread_rx (receiver_thread, std::ref(*vd_rt_ptr));
    std::thread thread_tx (transmitter_thread, std::ref(*vd_rt_ptr));
    std::thread thread_mgmt_command (management_thread_command, std::ref(*vd_rt_ptr));
    std::thread thread_mgmt_rtdb (management_thread_rtdb, std::ref(*vd_rt_ptr));
    
    // and wait for their end
    BOOST_LOG_SEV (log_main, trivial::info) << "Running ...";
    thread_rx.join ();
    thread_tx.join ();
    thread_mgmt_command.join ();
    thread_mgmt_rtdb.join ();

    delete vd_rt_ptr;
    vd_rt_ptr = nullptr;

    // and exit
    BOOST_LOG_SEV(log_main, trivial::info) << "Exiting.";

    return EXIT_SUCCESS;
    
  }
  catch (std::exception& e)
    {
      BOOST_LOG_SEV(log_main, trivial::fatal) << "Caught an exception, got " << e.what() << ". Exiting.";

      if (vd_rt_ptr)
	delete vd_rt_ptr;

      return EXIT_FAILURE;
    }
}

enum MgmtCommand { Shutdown, Activate, Deactivate, GetStatistics };

int run_vardis_management_command (MgmtCommand cmd, const std::string cfg_filename)
{
  // read configuration and start logging
  VardisClientConfiguration vdconfig;
  vdconfig.read_from_config_file (cfg_filename, true);
  VardisClientRuntime cl_rt (vdconfig);
  
  DcpStatus result;
  VardisProtocolStatistics protocol_stats;
  switch (cmd)
    {
    case Shutdown:        result = cl_rt.shutdown_vardis(); break;
    case Activate:        result = cl_rt.activate_vardis(); break;
    case Deactivate:      result = cl_rt.deactivate_vardis(); break;
    case GetStatistics:
      {
	result = cl_rt.retrieve_statistics(protocol_stats);
	if (result == dcp::VARDIS_STATUS_OK)
	  {
	    cout << "Vardis demon protocol runtime statistics:\n"
	    << "    RTDB services: create: " << protocol_stats.count_handle_rtdb_create
	    << ", delete: " << protocol_stats.count_handle_rtdb_delete
	    << ", update: " << protocol_stats.count_handle_rtdb_update
	    << ", read: " << protocol_stats.count_handle_rtdb_read
	    << "\n"
	    << "    Processed instructions: create: " << protocol_stats.count_process_var_create
	    << ", delete: " << protocol_stats.count_process_var_delete
	    << ", update: " << protocol_stats.count_process_var_update
	    << ", summary: " << protocol_stats.count_process_var_summary
	    << ", reqcreate: " << protocol_stats.count_process_var_reqcreate
	    << ", requpdate: " << protocol_stats.count_process_var_requpdate
	    << endl;
	  }
	break;
      }
    default:
      cout << "Unknown type of management command: " << cmd << endl;
      return EXIT_FAILURE;
    }

  if (result != dcp::VARDIS_STATUS_OK)
    {
      cout << "Management command failed with status code " << vardis_status_to_string (result) << endl;
      return EXIT_FAILURE;
    }
  
  return EXIT_SUCCESS;
}


int main (int argc, char* argv[])
{

  std::string cfg_filename;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help",         "produce help message and exit")
    ("version",      "show version information and exit")
    ("cfghelp",      "produce help message for config file format and exit")
    ("run,r",        po::value<std::string>(&cfg_filename), "run as a demon with given config file")
    ("shutdown,s",   po::value<std::string>(&cfg_filename), "send shutdown command to running demon using given config file")
    ("activate,a",   po::value<std::string>(&cfg_filename), "send activate command to running demon using given config file")
    ("deactivate,d", po::value<std::string>(&cfg_filename), "send deactivate command to running demon using given config file")
    ("statistics,t", po::value<std::string>(&cfg_filename), "retrieve runtime statistics from running demon using given config file")
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
	VardisConfiguration cfg;
	auto cfgdesc = cfg.construct_options_description();
	cout << cfgdesc << endl;
	return EXIT_SUCCESS;
      }
    
    if (vm.count("run"))
      {
	cout << "Running Vardis demon ..." << endl;
	return run_vardis_demon (cfg_filename);
      }

    if (vm.count("shutdown"))
      {
	return run_vardis_management_command (Shutdown, cfg_filename);
      }

    if (vm.count("activate"))
      {
	return run_vardis_management_command (Activate, cfg_filename);
      }

    if (vm.count("deactivate"))
      {
	return run_vardis_management_command (Deactivate, cfg_filename);
      }

    if (vm.count("statistics"))
      {
	return run_vardis_management_command (GetStatistics, cfg_filename);
      }
    
    cerr << "No valid option given." << endl;
    cerr << desc << endl;
    return EXIT_FAILURE;

  }
  catch (DcpException& e) {
    print_exiting_dcp_exception (e);
    return EXIT_FAILURE;
  }
  catch(exception& e) {
    cerr << e.what() << endl;
    cerr << desc << endl;
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}


