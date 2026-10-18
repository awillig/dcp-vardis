#include <cstdint>
#include <exception>
#include <gtest/gtest.h>
#include <dcp/common/memblock.h>

using namespace dcp;



TEST (CommonMiscTest, MemBlock) {

  byte testarray[] = {1,2,3,4,5,6,7,8,9,10,0};

  MemBlock mb0;
  EXPECT_EQ (mb0.data, nullptr);
  EXPECT_EQ (mb0.length, 0);
  EXPECT_TRUE (mb0.do_delete);
  
  MemBlock mb1 (mb0);
  EXPECT_EQ (mb1.data, nullptr);
  EXPECT_EQ (mb1.length, 0);
  EXPECT_EQ (mb1.do_delete, mb0.do_delete);

  
  MemBlock mb2 (11, testarray);
  EXPECT_NE (mb2.data, nullptr);
  EXPECT_EQ (mb2.length, 11);
  EXPECT_TRUE (mb2.do_delete);
  EXPECT_EQ (std::memcmp(testarray, mb2.data, 11), 0);
  EXPECT_NE (testarray, mb2.data);

  byte* mb2data     = mb2.data;
  size_t mb2length  = mb2.length;
  bool mb2dodelete  = mb2.do_delete;
  MemBlock mb3 = std::move(mb2);
  EXPECT_EQ (mb2.data, nullptr);
  EXPECT_EQ (mb2.length, 0);
  EXPECT_EQ (mb2.do_delete, true);
  EXPECT_EQ (mb3.data, mb2data);
  EXPECT_EQ (mb3.length, mb2length);
  EXPECT_EQ (mb3.do_delete, mb2dodelete);


  MemBlock mb4 (11, testarray);

  EXPECT_EQ (mb3, mb4);
}
