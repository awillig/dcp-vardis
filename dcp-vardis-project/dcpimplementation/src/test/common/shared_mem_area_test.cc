#include <cstdint>
#include <iostream>
#include <gtest/gtest.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/fixedmem_ring_buffer.h>
#include <dcp/common/sharedmem_finite_queue.h>
#include <dcp/common/sharedmem_structure_base.h>

using dcp::byte;
using dcp::FixedMemRingBuffer;
using dcp::PopHandler;
using dcp::PushHandler;
using dcp::RingBufferException;
using dcp::ShmException;
using dcp::ShmFiniteQueue;
using dcp::ShmStructureBase;

using std::cout;
using std::endl;
using std::flush;

typedef FixedMemRingBuffer<int, 20> RBType;

/**********************************************************************
 * Tests basic properties of a single SharedMemBuffer
 *********************************************************************/


#ifdef __UNDEFINED__
TEST (ShmTest, SharedMemBufferTest) {

  SharedMemBuffer empty_buff;
  EXPECT_TRUE (empty_buff.isEmpty());
  EXPECT_EQ (empty_buff.used_length (), 0);
  EXPECT_EQ (empty_buff.max_length (), 0);
  EXPECT_EQ (empty_buff.data_offs (), 0);
  EXPECT_EQ (empty_buff.index (), 0);

  const size_t bufsize = 64;
  const char testtext [] = "Hello";
  char buffer [bufsize];
  SharedMemBuffer data_buff1 (bufsize, 4, 0);;
  EXPECT_TRUE (data_buff1.isEmpty());
  EXPECT_EQ (data_buff1.used_length(), 0);
  EXPECT_EQ (data_buff1.max_length(), bufsize);
  EXPECT_EQ (data_buff1.data_offs(), 0);
  EXPECT_EQ (data_buff1.index(), 4);
  
  EXPECT_THROW (data_buff1.write_to (nullptr, nullptr, 0), ShmException);
  EXPECT_THROW (data_buff1.write_to ((byte*) buffer, nullptr, 0), ShmException);
  EXPECT_THROW (data_buff1.write_to ((byte*) buffer, (byte*) testtext, 0), ShmException);
  data_buff1.write_to ((byte*) buffer, (byte*) testtext, std::strlen(testtext)+1);
  EXPECT_EQ (data_buff1.used_length(), 6);
  EXPECT_EQ (data_buff1.max_length(), bufsize);
  EXPECT_EQ (data_buff1.data_offs(), 0);
  EXPECT_STREQ (testtext, buffer);
  
}
#endif

/**********************************************************************
 * Tests basic properties of a ring buffer, such as parameter checking
 * in constructor, reaction to popping from / pushing to empty / full
 * ring buffer, and checking that maximum capacity is observed.
 *********************************************************************/

TEST (ShmTest, FixedMemRingBufferTest_Basic) {
  
  // Ringbuffer constructor properly checks presence of name, length of name
  // and that name is properly stored
  RBType* _p;
  EXPECT_THROW (_p = new RBType (nullptr, 10);, RingBufferException);
  EXPECT_THROW (_p = new RBType ("0123456789012345678901234567890123456789012345678901234567890123", 10);, RingBufferException);
  _p = new RBType ("012345678901234567890123456789012345678901234567890123456789012", 10);
  EXPECT_STREQ(_p->get_name(), "012345678901234567890123456789012345678901234567890123456789012");

  // Nothing can be removed (or peeked) from an empty ring buffer
  EXPECT_THROW(_p->pop(), RingBufferException);
  EXPECT_THROW(_p->peek(), RingBufferException);

  delete _p;
  
  // Check proper states of empty and full ring buffers, and that nothing can
  // be added from an empty ringbuffer
  FixedMemRingBuffer<int, 20> rb20 ("rb20", 10);
  int empty_val =  0;
  EXPECT_TRUE (rb20.isEmpty());
  EXPECT_FALSE (rb20.isFull());
  EXPECT_EQ (rb20.stored_elements(), 0);
  for (int i=0; i<10; i++) rb20.push (empty_val);
  EXPECT_FALSE (rb20.isEmpty());
  EXPECT_TRUE (rb20.isFull());
  EXPECT_EQ (rb20.stored_elements(), 10);
  EXPECT_THROW (rb20.push(empty_val), RingBufferException);
  empty_val = rb20.pop();
  rb20.push(empty_val);

}

/**********************************************************************
 * Tests that pushing and popping from ring buffer can be done without
 * losing any data / buffers. This test runs without considering
 * locking the buffer.
 *********************************************************************/

TEST (ShmTest, FixedMemRingBufferTest_SimpleCircular) {

  RBType rbFree ("rbFree", 19);
  RBType rbQueue ("rbQueue", 19);

  for (uint64_t i=0;i<19;i++)
    {
      rbFree.push(i);
    }
  EXPECT_THROW (rbFree.push(100), RingBufferException);

  int write_counter = 0;
  int read_counter = 0;
  for (int i=0; i<19; i++)
    {
      rbFree.pop();
      rbQueue.push(write_counter++);
      EXPECT_EQ (rbFree.stored_elements() + rbQueue.stored_elements(), 19);
    }

  for (int i=0; i<5000; i++)
    {
      {
	int read_value = rbQueue.pop();
	EXPECT_EQ (read_value, read_counter++);
	rbFree.push (read_value);
      }
      {
	rbFree.pop();
	rbQueue.push(write_counter++);
      }
      EXPECT_EQ (rbFree.stored_elements() + rbQueue.stored_elements(), 19);
    }
}

/**********************************************************************
 * Preparation for tests where a ring buffer is placed in a shared
 * memory area and operated from there, including consideration of
 * locking.
 *********************************************************************/

const char shm_area_name [100] = "testing-shm-area-name";

template <uint64_t numBuffers>
class TestControlSegment {
  typedef ShmFiniteQueue<numBuffers, sizeof(int)> ShmRBType;
  const int number_values = 300000;
  
  ShmRBType rbQueue;
  
public:
  
  TestControlSegment ()
    : rbQueue ("rbQueue", numBuffers)
  { 
  };
  
  void producer_thread ()
  {
    int write_counter = 0;
    
    for (int i=0; i<number_values; i++)
      {
	bool timed_out;
	PushHandler handler = [&] (byte* memaddr, size_t)
	{
	  *((int*)memaddr) = write_counter++;
	  return sizeof(int);
	};
	rbQueue.push_wait (handler, timed_out, 1000);
	EXPECT_FALSE (timed_out);
      }
  };
  
  
  void consumer_thread_wait ()
  {
      int read_counter = 0;

      for (int i=0; i<number_values; i++)
	{
	  int  read_val;
	  bool timed_out;
	  bool further_entries;
	  PopHandler handler = [&] (byte* memaddr, size_t len)
	  {
	    EXPECT_EQ (len, sizeof(int));
	    read_val = * ((int*) memaddr);
	  };
	  
	  rbQueue.pop_wait (handler, timed_out, further_entries, 1000);
	  EXPECT_FALSE(timed_out);
	  EXPECT_EQ(read_val, read_counter++);
	}  
  };


  void consumer_thread_nowait ()
  {
      int read_counter = 0;

      for (int i=0; i<number_values; i++)
	{
	  int  read_val;
	  bool timed_out       = false;
	  bool further_entries = false;
	  bool got_data        = false;
	  PopHandler handler = [&] (byte* memaddr, size_t len)
	  {
	    got_data = true;
	    EXPECT_EQ (len, sizeof(int));
	    read_val = * ((int*) memaddr);
	  };

	  do {
	    rbQueue.pop_nowait (handler, timed_out, further_entries, 10);
	  } while (not got_data);
	  EXPECT_FALSE(timed_out);
	  EXPECT_TRUE(got_data);
	  EXPECT_FALSE(further_entries);
	  EXPECT_EQ(read_val, read_counter++);
	}
      
  };
  
};


/**********************************************************************
 * Tests parallel reading and writing from a ring buffer in shared
 * memory, including synchronization / locking using the wait_pop and
 * wait_push methods of ShmRingBuffer.
 *********************************************************************/



TEST (ShmTest, ShmRingBufferTest_ConcurrentCircular20) {

  auto shmAreaPtr            = std::make_shared<ShmStructureBase> (shm_area_name, sizeof(TestControlSegment<20>), true);
  auto shmAreaPtrProducer    = std::make_shared<ShmStructureBase> (shm_area_name, 0, false);
  new (shmAreaPtr->get_memory_address()) TestControlSegment<20>;
  TestControlSegment<20>* pCSCons = ((TestControlSegment<20>*) shmAreaPtr->get_memory_address());
  TestControlSegment<20>* pCSProd = ((TestControlSegment<20>*) shmAreaPtrProducer->get_memory_address());

  std::thread thread_prod ([&] () {pCSProd->producer_thread(); });
  std::thread thread_cons ([&] () {pCSCons->consumer_thread_wait ();});
  
  thread_prod.join();
  thread_cons.join();  
}



TEST (ShmTest, ShmRingBufferTest_ConcurrentCircular1Wait) {

  auto shmAreaPtr            = std::make_shared<ShmStructureBase> (shm_area_name, sizeof(TestControlSegment<1>), true);
  auto shmAreaPtrProducer    = std::make_shared<ShmStructureBase> (shm_area_name, 0, false);
  new (shmAreaPtr->get_memory_address()) TestControlSegment<1>;
  TestControlSegment<1>* pCSCons = ((TestControlSegment<1>*) shmAreaPtr->get_memory_address());
  TestControlSegment<1>* pCSProd = ((TestControlSegment<1>*) shmAreaPtrProducer->get_memory_address());

  std::thread thread_prod ([&] () {pCSProd->producer_thread(); });
  std::thread thread_cons ([&] () {pCSCons->consumer_thread_wait ();});
    
  thread_prod.join();
  thread_cons.join();
}

TEST (ShmTest, ShmRingBufferTest_ConcurrentCircular1Nowait) {

  auto shmAreaPtr            = std::make_shared<ShmStructureBase> (shm_area_name, sizeof(TestControlSegment<1>), true);
  auto shmAreaPtrProducer    = std::make_shared<ShmStructureBase> (shm_area_name, 0, false);
  new (shmAreaPtr->get_memory_address()) TestControlSegment<1>;
  TestControlSegment<1>* pCSCons = ((TestControlSegment<1>*) shmAreaPtr->get_memory_address());
  TestControlSegment<1>* pCSProd = ((TestControlSegment<1>*) shmAreaPtrProducer->get_memory_address());

  std::thread thread_prod ([&] () {pCSProd->producer_thread(); });
  std::thread thread_cons ([&] () {pCSCons->consumer_thread_nowait ();});
    
  thread_prod.join();
  thread_cons.join();  
}


