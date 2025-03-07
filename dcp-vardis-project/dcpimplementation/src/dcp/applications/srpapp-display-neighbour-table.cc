#include <chrono>
#include <csignal>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <thread>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/program_options.hpp>
#include <dcp/srp/srp_transmissible_types.h>
#include <dcp/srp/srpclient_configuration.h>
#include <dcp/srp/srpclient_lib.h>

// For some odd reason ncurses should be included last, otherwise we
// get tons of compiler errors
#include <ncurses.h>   


using std::cout;
using std::endl;
using std::cerr;
using dcp::srp::defaultSRPStoreShmName;

using namespace dcp;

void print_version ()
{
  cout << dcp::dcpHighlevelDescription
       << " -- version " << dcp::dcpVersionNumber
       << endl;
}


bool       exitFlag = false;

void signalHandler (int signum)
{
  cout << "Caught signal code " << signum << " (" << strsignal(signum) << "). Exiting." << endl;
  exitFlag = true;
}


void show_header (int counter)
{
  move (0, 0);
  printw ("Neighbour table (%d)", counter);
  move (1, 0);
  printw ("----------------------------------------------------");

  attron (A_BOLD);
  move (3, 0);
  printw ("Neighbour ID");
  move (3, 20);
  printw ("Position (x/y/z)");
  move (3, 50);
  printw ("Seqno");
  move (3, 60);
  printw ("Age (ms)");
  
  attroff (A_BOLD);
}

void show_node_line (const int line,
		     const NodeIdentifierT  nodeId,
		     const SafetyDataT sd,
		     const uint32_t seqno,
		     const uint16_t age)
{
  move (line, 0);
  printw ("%s", nodeId.to_str().c_str());
  move (line, 20);
  printw ("%.2f / %.2f / %.2f", sd.position_x, sd.position_y, sd.position_z);
  move (line, 50);
  printw ("%d", seqno);
  move (line, 60);
  printw ("%d", age);
}
		     

void show_footer (const int line)
{
  move (line, 0);
  printw ("----------------------------------------------------");
}


int main (int argc, char* argv [])
{
  std::string shmname_store  = defaultSRPStoreShmName;
  int     periodTmp = 0;

  
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h",         "produce help message and exit")
    ("version,v",      "show version information and exit")
    ("shmstore,s",     po::value<std::string>(&shmname_store)->default_value(defaultSRPStoreShmName), "Unique name of shared memory area for SRP store")
    ("period",         po::value<int>(&periodTmp), "Query period (in ms)")
    ;

  po::positional_options_description desc_pos;
  desc_pos.add ("period", 1);

  try {
    po::variables_map vm;
    po::store (po::command_line_parser(argc, argv).options(desc).positional(desc_pos).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
      {
	cout << std::string (argv[0]) << " [-s <shmstore>] <genperiodMS> <average-x> <average-y> <average-z> <stddev>" << endl;
	cout << desc << endl;
	return EXIT_SUCCESS;
      }

    if (vm.count("version"))
      {
	print_version();
	return EXIT_SUCCESS;
      }
    
    if ((periodTmp <= 0) || (periodTmp > std::numeric_limits<uint16_t>::max()))
      {
	cout << "Generation period outside allowed range. Aborting." << endl;
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
  SRPClientConfiguration cl_conf;
  cl_conf.shm_conf_store.shmAreaName    = shmname_store;

  try {
    SRPClientRuntime cl_rt (cl_conf);    
    
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
	if ((w >= 85) and (h >= 20))
	  {
	    counter++;
	    show_header(counter);
	      
	    std::list<srp::ExtendedSafetyDataT> esd_list;
	    cl_rt.get_all_neighbours_esd(esd_list);

	    int line = 5;

	    if (line < h-1)
	      {
		for (const auto& esd : esd_list)
		  {
		    TimeStampT current_time   = TimeStampT::get_current_system_time();
		    TimeStampT received_time  = esd.timeStamp;
		    auto age = current_time.milliseconds_passed_since(received_time);
		    
		    show_node_line (line, esd.nodeId, esd.safetyData, esd.seqno, age);
		    line++;
		  }
	      }
	    show_footer (h-1);
	  }
	refresh ();
      }    

    endwin ();
    return EXIT_SUCCESS;
  }
  catch (std::exception& e)
    {
      cout << "Caught an exception, got " << e.what() << ", exiting." << endl;
      return EXIT_FAILURE;
    }
}
