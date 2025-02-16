#include <cstdint>
#include <gtest/gtest.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/shared_mem_area.h>

using dcp::ShmControlSegmentBase;
using dcp::ScopedShmControlSegmentLock;
using dcp::ShmBufferPool;
using dcp::RingBuffer;
using dcp::SharedMemBuffer;
using dcp::ShmException;
using dcp::byte;


/**********************************************************************
 * Tests basic properties of a single SharedMemBuffer
 *********************************************************************/


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

/**********************************************************************
 * Tests basic properties of a ring buffer, such as parameter checking
 * in constructor, reaction to popping from / pushing to empty / full
 * ring buffer, and checking that maximum capacity is observed.
 *********************************************************************/

TEST (ShmTest, RingBufferTest_Basic) {

  // Ringbuffer constructor properly checks presence of name, length of name
  // and that name is properly stored
  RingBuffer<20>* _p;
  EXPECT_THROW (_p = new RingBuffer<20> (nullptr, 10);, ShmException);
  EXPECT_THROW (_p = new RingBuffer<20> ("0123456789012345678901234567890123456789012345678901234567890123", 10);, ShmException);
  _p = new RingBuffer<20> ("012345678901234567890123456789012345678901234567890123456789012", 10);
  EXPECT_STREQ(_p->get_name(), "012345678901234567890123456789012345678901234567890123456789012");

  // Nothing can be removed (or peeked) from an empty ring buffer
  EXPECT_THROW(_p->pop(), ShmException);
  EXPECT_THROW(_p->peek(), ShmException);

  delete _p;
  
  // Check proper states of empty and full ring buffers, and that nothing can
  // be added from an empty ringbuffer
  RingBuffer<20> rb20 ("rb20", 10);
  SharedMemBuffer empty_buff;
  EXPECT_TRUE (rb20.isEmpty());
  EXPECT_FALSE (rb20.isFull());
  EXPECT_EQ (rb20.stored_elements(), 0);
  for (int i=0; i<10; i++) rb20.push (empty_buff);
  EXPECT_FALSE (rb20.isEmpty());
  EXPECT_TRUE (rb20.isFull());
  EXPECT_EQ (rb20.stored_elements(), 10);
  EXPECT_THROW (rb20.push(empty_buff), ShmException);
  empty_buff = rb20.pop();
  rb20.push(empty_buff);

}

/**********************************************************************
 * Tests that pushing and popping from ring buffer can be done without
 * losing any data / buffers. This test runs without considering
 * locking the buffer.
 *********************************************************************/

TEST (ShmTest, RingBufferTest_SimpleCircular) {
  RingBuffer<20> rbFree ("rbFree", 19);
  RingBuffer<20> rbQueue ("rbQueue", 19);

  for (uint64_t i=0;i<19;i++)
    {
      SharedMemBuffer buff (sizeof(int), i, i*sizeof(int));
      rbFree.push(buff);
    }
  EXPECT_THROW (rbFree.push(SharedMemBuffer()), ShmException);

  int write_counter = 0;
  int read_counter = 0;
  int intvect [20];
  byte* buffer_seg_ptr = (byte*) &(intvect[0]);
  for (int i=0; i<19; i++)
    {
      SharedMemBuffer buff = rbFree.pop();
      int* iptr = (int*) (buffer_seg_ptr + buff.data_offs());
      *iptr = write_counter++;
      buff.set_used_length(sizeof(int));
      rbQueue.push(buff);
    }

  for (int i=0; i<5000; i++)
    {
      {
	SharedMemBuffer buff = rbQueue.pop();
	int* iptr = (int*) (buffer_seg_ptr + buff.data_offs());
	EXPECT_EQ (*iptr, read_counter++);
	buff.clear();
	rbFree.push (buff);
      }
      {
	SharedMemBuffer buff = rbFree.pop();
	int* iptr = (int*) (buffer_seg_ptr + buff.data_offs());
	*iptr = write_counter++;
	buff.set_used_length(sizeof(int));
	rbQueue.push(buff);
      }
    }
}

/**********************************************************************
 * Preparation for tests where a ring buffer is placed in a shared
 * memory area and operated from there, including consideration of
 * locking.
 *********************************************************************/

const char shm_area_name [] = "testing-shm-area-name";
const int number_values = 10000;

typedef struct TestControlSegment : public ShmControlSegmentBase {
  RingBuffer<20> rbFree;
  RingBuffer<20> rbQueue;

  TestControlSegment ()
    : rbFree ("rbFree", 19),
      rbQueue ("rbQueue", 19)
  {
    for (int i=0; i<19; i++)
      {
	SharedMemBuffer buff (sizeof(int), 19, i*sizeof(int));
	rbFree.push(buff);
      }
  };
} TestControlSegment;


/**********************************************************************
 * Tests parallel reading and writing from a ring buffer in shared
 * memory, including synchronization / locking using the wait_pop and
 * wait_push methods of RingBuffer.
 *********************************************************************/

void producer_thread (TestControlSegment& CS, byte* buffer_seg_ptr)
{
  int write_counter = 0;
  
  for (int i=0; i<number_values; i++)
    {
      SharedMemBuffer buff = CS.rbFree.wait_pop (CS);
      int* iptr = (int*) (buffer_seg_ptr + buff.data_offs());
      *iptr = write_counter++;
      buff.set_used_length(sizeof(int));
      CS.rbQueue.wait_push (CS, buff);
    }
}


TEST (ShmTest, RingBufferTest_ConcurrentCircular) {

  auto shmAreaPtr = std::make_shared<ShmBufferPool> (shm_area_name, true, sizeof(TestControlSegment), sizeof(int), 19);
  auto controlSegPtr = new (shmAreaPtr->getControlSegmentPtr()) TestControlSegment ();
  byte* buffer_seg_ptr = shmAreaPtr->getBufferSegmentPtr();


  auto shmAreaPtrProducer    = std::make_shared<ShmBufferPool> (shm_area_name, false, sizeof(int), 0, 0);
  byte* buffer_seg_ptr_prod  = shmAreaPtrProducer->getBufferSegmentPtr();
  std::thread thread_prod (producer_thread, std::ref(*controlSegPtr), buffer_seg_ptr_prod);

  int read_counter = 0;
  TestControlSegment& CS = *controlSegPtr;
  for (int i=0; i<number_values; i++)
    {
      SharedMemBuffer buff = CS.rbQueue.wait_pop (CS);
      int* iptr = (int*) (buffer_seg_ptr + buff.data_offs());
      EXPECT_EQ(*iptr, read_counter++);      
      CS.rbFree.wait_push(CS, buff);
    }
  
  thread_prod.join();
  
}


/**********************************************************************
 * Tests parallel reading and writing from a ring buffer in shared
 * memory, including synchronization / locking using the
 * wait_pop_process_push method of RingBuffer.
 *********************************************************************/

void producer_thread_two (TestControlSegment& CS, byte* buffer_seg_ptr)
{
  int write_counter = 0;

  auto process = [&write_counter] (SharedMemBuffer& buff, byte* data_ptr)
                    {
		      int* iptr = (int*) data_ptr;
		      *iptr = write_counter++;
		      buff.set_used_length(sizeof(int));
		    };
  
  for (int i=0; i<number_values; i++)
      CS.rbFree.wait_pop_process_push (CS, buffer_seg_ptr, CS.rbQueue, process);
}


TEST (ShmTest, RingBufferTest_ConcurrentCircularTwo) {

  auto shmAreaPtr = std::make_shared<ShmBufferPool> (shm_area_name, true, sizeof(TestControlSegment), sizeof(int), 19);
  auto controlSegPtr = new (shmAreaPtr->getControlSegmentPtr()) TestControlSegment ();
  byte* buffer_seg_ptr = shmAreaPtr->getBufferSegmentPtr();

  auto shmAreaPtrProducer    = std::make_shared<ShmBufferPool> (shm_area_name, false, sizeof(int), 0, 0);
  byte* buffer_seg_ptr_prod  = shmAreaPtrProducer->getBufferSegmentPtr();
  
  std::thread thread_prod_two (producer_thread_two, std::ref(*controlSegPtr), buffer_seg_ptr_prod);

  int read_counter = 0;
  TestControlSegment& CS = *controlSegPtr;

  auto process = [&read_counter] (SharedMemBuffer& buff, byte* data_ptr)
                    {
		      int* iptr = (int*) data_ptr;
		      EXPECT_EQ(*iptr, read_counter++);
		      buff.clear();
		    };
  
  for (int i=0; i<number_values; i++)
      CS.rbQueue.wait_pop_process_push (CS, buffer_seg_ptr, CS.rbFree, process);
  
  thread_prod_two.join();
  
}

