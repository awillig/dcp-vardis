#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <list>
#include <thread>
#include <dcp/applications/vardisapp-test-variabletype.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/other_helpers.h>
#include <dcp/vardis/vardis_service_primitives.h>
#include <dcp/vardis/vardis_transmissible_types.h>
#include <dcp/vardis/vardisclient_configuration.h>
#include <dcp/vardis/vardisclient_lib.h>

// For some odd reason ncurses should be included last, otherwise we
// get tons of compiler errors
#include <ncurses.h>   


using dcp::vardis::defaultVardisCommandSocketFileName;
using dcp::vardis::defaultVardisStoreShmName;
using std::cerr;
using std::cout;
using std::endl;
using dcp::vardis::VarSeqnoT;

using namespace dcp;


const std::string defaultVardisClientShmName = "shm-vardisapp-test-consumer";


void print_version ()
{
  cout << dcp::dcpHighlevelDescription
       << " -- version " << dcp::dcpVersionNumber
       << endl;
}

bool       exitFlag = false;

void signalHandler (int)
{
  exitFlag = true;
}


void show_header (int counter)
{
  move (0, 0);
  printw ("Vardis variables (%d) -- Press Ctrl-C to exit", counter);
  move (1, 0);
  printw ("-------------------------------------------------------------------");

  attron (A_BOLD);
  move (3, 0);
  printw ("VarId");
  move (3, 8);
  printw ("Descr");
  move (3, 25);
  printw ("Producer");
  move (3, 45);
  printw ("Seqno");
  move (3, 55);
  printw ("Value");
  move (3, 69);
  printw ("Age(ms)");
  move (3, 77);
  printw ("DEL");
  
  attroff (A_BOLD);
}

void show_var_line (const int line,
		    const VarIdT varId,
		    const char* descr,
		    const NodeIdentifierT prodId,
		    const uint32_t seqno,
		    const double value,
		    const uint32_t age,
		    const bool isDeleted)
{
  move (line, 0);
  printw ("%d", (int) varId.val);
  move (line, 8);
  printw ("%.15s", descr);
  move (line, 25);
  printw ("%s", prodId.to_str().c_str());
  move (line, 45);
  printw ("%d", seqno);
  move (line, 55);
  printw ("%.3f", value);
  move (line, 69);
  printw ("%d", isDeleted ? 0 : age);
  move (line, 77);
  printw ("%s", isDeleted ? "true " : "false");
}


void show_footer (const int line)
{
  move (line, 0);
  printw ("-------------------------------------------------------------------");
}

void output_cmdline_guidance (char* argv[])
{
  cout << std::string (argv[0]) << " [-s <sockname>] [-mc <shmcli>] [-mg <shmgdb>] <queryperiodMS>" << endl;
}

int main (int argc, char* argv [])
{
  std::string cmdsock_name  = defaultVardisCommandSocketFileName;
  std::string shmname_cli   = defaultVardisClientShmName;
  std::string shmname_glob  = defaultVardisStoreShmName;
  int     periodTmp = 0;

  
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h",         "produce help message and exit")
    ("version,v",      "show version information and exit")
    ("sockname,s",     po::value<std::string>(&cmdsock_name)->default_value(defaultVardisCommandSocketFileName), "filename of VarDis command socket (UNIX Domain Socket)")
    ("shmcli,c",       po::value<std::string>(&shmname_cli)->default_value(defaultVardisClientShmName), "Name of shared memory area for interfacing with Vardis")
    ("shmgdb,g",       po::value<std::string>(&shmname_glob)->default_value(defaultVardisStoreShmName), "Unique name of shared memory area for accessing VarDis variables (global database)")
    ("period",         po::value<int>(&periodTmp), "Generation period (in ms)")
    ;

  po::positional_options_description desc_pos;
  desc_pos.add ("period", 1);

  try {
    po::variables_map vm;
    po::store (po::command_line_parser(argc, argv).options(desc).positional(desc_pos).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
      {
	output_cmdline_guidance (argv);
	cout << desc << endl;
	return EXIT_SUCCESS;
      }

    if (vm.count("version"))
      {
	print_version();
	return EXIT_SUCCESS;
      }

    if (vm.count("period") == 0)
      {
	cout << "Insufficient arguments." << endl;
	output_cmdline_guidance (argv);
	cout << desc << endl;
	return EXIT_FAILURE;
      }
    
    if ((periodTmp <= 0) || (periodTmp > std::numeric_limits<uint16_t>::max()))
      {
	cout << "Query period outside allowed range. Aborting." << endl;
	return EXIT_FAILURE;
      }
    
  }
  catch(std::exception& e) {
    cerr << e.what() << endl;
    cerr << desc << endl;
    return EXIT_FAILURE;
  }
  
  // ----------------------------------
  
  uint16_t  periodMS = (uint16_t) periodTmp;

  // ----------------------------------
  // Install signal handlers
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGINT, signalHandler);
  std::signal(SIGABRT, signalHandler);
  

  // ----------------------------------
  // Register with Vardis
  VardisClientConfiguration cl_conf;
  cl_conf.cmdsock_conf.commandSocketFile = cmdsock_name;
  cl_conf.shm_conf_client.shmAreaName    = shmname_cli;
  cl_conf.shm_conf_global.shmAreaName    = shmname_glob;

  try {
    VardisClientRuntime cl_rt (cl_conf, true, true);

    // ============================================
    // Main loop
    // ============================================
    
    cout << "Entering update loop. Stop with <Ctrl-C>." << endl;
    
    int counter = 0;

    initscr ();
    
    while (not exitFlag)
      {
	std::this_thread::sleep_for (std::chrono::milliseconds (periodMS));

	int h, w;
	getmaxyx (stdscr, h, w);

	if ((w >= 80) and (h >= 12))
	  {
	    counter++;
	    clear ();
	    show_header (counter);
		
	    std::list<DescribeDatabaseVariableDescription> db_list;
	
	    DcpStatus dd_status = cl_rt.describe_database (db_list);
	    if (dd_status != VARDIS_STATUS_OK)
	      {
		endwin ();
		cout << "Obtaining database description failed with status " << vardis_status_to_string (dd_status) << ", exiting." << endl;
		return EXIT_FAILURE;
	      }

	    int line = 5;
	    
	    for (const auto& descr : db_list)
	      {
		if (line < h-2)
		  {
		    VarIdT      respVarId;
		    VarLenT     respVarLen;
		    TimeStampT  respTimeStamp;
		    const size_t read_buffer_size = 1000;
		    byte     read_buffer [read_buffer_size];
		    DcpStatus read_status = cl_rt.rtdb_read (descr.varId, respVarId, respVarLen, respTimeStamp, read_buffer_size, read_buffer);
		    
		    if ((read_status != VARDIS_STATUS_OK) and (read_status != VARDIS_STATUS_VARIABLE_IS_DELETED))
		      {
			endwin ();
			cout << "Reading varId " << descr.varId << " failed with status " << vardis_status_to_string (read_status) << endl;
			return EXIT_FAILURE;
		      }
		    
		    if ((read_status == VARDIS_STATUS_OK) and (respVarId != descr.varId))
		      {
			endwin ();
			cout << "Submitted read request for varId " << descr.varId << " but got response for varId " << respVarId << ", exiting." << endl;
			return EXIT_FAILURE;
		      }
		    
		    if ((read_status == VARDIS_STATUS_OK) and (respVarLen != sizeof(VardisTestVariable)))
		      {
			endwin ();
			cout << "Submitted read request for varId " << descr.varId << ", got respVarLen = " << respVarLen << " but expected length " << sizeof(VardisTestVariable) << ", exiting." << endl;
			return EXIT_FAILURE;
		      }
		    
		    VardisTestVariable* tv_ptr = (VardisTestVariable*) read_buffer;
		    
		    TimeStampT start_time  = tv_ptr->tstamp;
		    TimeStampT rcvd_time   = respTimeStamp;
		    auto age = rcvd_time.milliseconds_passed_since (start_time);
		    
		    show_var_line (line, descr.varId,
				   descr.description,
				   descr.prodId,
				   tv_ptr->seqno,
				   tv_ptr->value,
				   age,
				   (read_status == VARDIS_STATUS_VARIABLE_IS_DELETED) ? true : false);
		    line++;		    
		  }
	      }
	    show_footer (h-1);
	  }
	refresh();
      }
    endwin ();
    return EXIT_SUCCESS;
    
  }
  catch (DcpException& e)
    {
      print_exiting_dcp_exception (e);
      return EXIT_FAILURE;
    }
  catch (std::exception& e)
    {
      endwin();
      cout << "Caught an exception, got " << e.what() << ", exiting." << endl;
      return EXIT_FAILURE;
    }  
}
