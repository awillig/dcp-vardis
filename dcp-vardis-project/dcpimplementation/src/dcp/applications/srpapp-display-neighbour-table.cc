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
#include <dcp/common/exceptions.h>
#include <dcp/common/other_helpers.h>
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
using dcp::DcpException;

using namespace dcp;

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


void show_header (int counter, int numelems)
{
  move (0, 0);
  printw ("Neighbour table (%d, %d elements) -- Press Ctrl-C to exit", counter, numelems);
  move (1, 0);
  printw ("-------------------------------------------------------------------");

  attron (A_BOLD);
  move (3, 0);
  printw ("Neighbour ID");
  move (3, 20);
  printw ("Position (x/y/z)");
  move (3, 50);
  printw ("Seqno");
  move (3, 60);
  printw ("AgeTx(ms)");
  move (3, 71);
  printw ("AgeRx(ms)");
  move (3, 82);
  printw ("AvgGapSize");
  
  attroff (A_BOLD);
}

void show_node_line (const int line,
		     const NodeIdentifierT  nodeId,
		     const SafetyDataT sd,
		     const uint32_t seqno,
		     const uint32_t age_tx,
		     const uint32_t age_rx,
		     const double avg_gapsize)
{
  move (line, 0);
  printw ("%s", nodeId.to_str().c_str());
  move (line, 20);
  printw ("%.2f / %.2f / %.2f", sd.position_x, sd.position_y, sd.position_z);
  move (line, 50);
  printw ("%d", seqno);
  move (line, 60);
  printw ("%d", age_tx);
  move (line, 71);
  printw ("%d", age_rx);
  move (line, 82);
  printw ("%.2f", avg_gapsize);
}
		     

void show_footer (const int line)
{
  move (line, 0);
  printw ("-------------------------------------------------------------------");
}


void output_cmdline_guidance (char* argv[])
{
  cout << std::string (argv[0]) << " [-s <shmstore>] <queryperiodMS>" << endl;
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
	if ((w >= 80) and (h >= 20))
	  {
	    counter++;
	      
	    std::list<srp::NodeInformation> ni_list;
	    cl_rt.get_all_neighbours_node_information (ni_list);

	    clear ();
	    show_header(counter, (int) ni_list.size());
	    
	    int line = 5;

	    for (const auto& ni : ni_list)
	      {
		if (line < h-2)
		  {
		    TimeStampT current_time      = TimeStampT::get_current_system_time();
		    TimeStampT transmitted_time  = ni.esd.timeStamp;
		    TimeStampT received_time     = ni.last_reception_time;
		    auto age_tx = current_time.milliseconds_passed_since(transmitted_time);
		    auto age_rx = current_time.milliseconds_passed_since(received_time);
		    
		    show_node_line (line, ni.esd.nodeId, ni.esd.safetyData, ni.esd.seqno, age_tx, age_rx, ni.avg_seqno_gap_size_estimate);
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
