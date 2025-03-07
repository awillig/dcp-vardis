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

boost::mt19937 generator;

void signalHandler (int signum)
{
  cout << "Caught signal code " << signum << " (" << strsignal(signum) << "). Exiting." << endl;
  exitFlag = true;
}


SafetyDataT generate_new_sd (boost::normal_distribution<> dx,
			     boost::normal_distribution<> dy,
			     boost::normal_distribution<> dz)
{
  SafetyDataT sd;
  sd.position_x = dx (generator);
  sd.position_y = dy (generator);
  sd.position_z = dz (generator);
  
  return sd;
}


int main (int argc, char* argv [])
{
  std::string shmname_store  = defaultSRPStoreShmName;
  int     periodTmp = 0;
  double  average_x = 0;
  double  average_y = 0;
  double  average_z = 0;
  double  stddev = 0;

  
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h",         "produce help message and exit")
    ("version,v",      "show version information and exit")
    ("shmstore,s",     po::value<std::string>(&shmname_store)->default_value(defaultSRPStoreShmName), "Unique name of shared memory area for SRP store")
    ("period",         po::value<int>(&periodTmp), "Generation period (in ms)")
    ("averagex",       po::value<double>(&average_x), "Average of generated Gaussian for x-coordinates")
    ("averagey",       po::value<double>(&average_y), "Average of generated Gaussian for y-coordinates")
    ("averagez",       po::value<double>(&average_z), "Average of generated Gaussian for z-coordinates")
    ("stddev",         po::value<double>(&stddev), "Standard deviation of generated Gaussian")
    ;

  po::positional_options_description desc_pos;
  desc_pos.add ("period", 1);
  desc_pos.add ("averagex", 1);
  desc_pos.add ("averagey", 1);
  desc_pos.add ("averagez", 1);
  desc_pos.add ("stddev", 1);

  try {
    po::variables_map vm;
    //po::store(po::parse_command_line(argc, argv, desc), vm);
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
    
    if (stddev < 0)
      {
	cout << "Stddev outside allowed range. Aborting." << endl;
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
  boost::normal_distribution<> dx (average_x, stddev);
  boost::normal_distribution<> dy (average_y, stddev);
  boost::normal_distribution<> dz (average_z, stddev);

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
    
    while (not exitFlag)
      {
	std::this_thread::sleep_for (std::chrono::milliseconds (periodMS));

	srp::SafetyDataT new_sd = generate_new_sd (dx, dy, dz);
	
	DcpStatus update_status = cl_rt.set_own_safety_data(new_sd);
	if (update_status != SRP_STATUS_OK)
	  {
	    cout << "Update of own safety data failed with status " << srp_status_to_string (update_status) << ", exiting." << endl;
	    exitFlag = true;
	  }
      }    
    
    return EXIT_SUCCESS;
  }
  catch (std::exception& e)
    {
      cout << "Caught an exception, got " << e.what() << ", exiting." << endl;
      return EXIT_FAILURE;
    }
}
