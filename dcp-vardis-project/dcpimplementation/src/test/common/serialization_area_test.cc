#include <cstdint>
#include <gtest/gtest.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/area.h>

using dcp::byte;
using dcp::bytevect;
using dcp::Area;
using dcp::AreaException;
using dcp::AssemblyAreaException;
using dcp::DisassemblyAreaException;
using dcp::ByteVectorAssemblyArea;
using dcp::ByteVectorDisassemblyArea;
using dcp::MemoryChunkAssemblyArea;
using dcp::MemoryChunkDisassemblyArea;


/**********************************************************************
 * Tests basic consistency and integrity properties of a base class
 * Area
 *********************************************************************/


TEST (AreaTest, BasicAreaTest) {

  Area area ("test", 37);
  EXPECT_EQ (area.used(), 0);
  EXPECT_EQ (area.available(), 37);
  EXPECT_EQ (area.initial(), 37);
  area.incr();
  EXPECT_EQ (area.used(), 1);
  EXPECT_EQ (area.available(), 36);
  EXPECT_EQ (area.initial(), 37);
  area.incr(5);
  EXPECT_EQ (area.used(), 6);
  EXPECT_EQ (area.available(), 31);
  EXPECT_EQ (area.initial(), 37);

  char testdata [] = "hello";
  EXPECT_THROW (area.assert_block(0, nullptr), AreaException);
  EXPECT_THROW (area.assert_block(6, nullptr), AreaException);
  EXPECT_NO_THROW (area.assert_block(6, (byte*) testdata));
  EXPECT_NO_THROW (area.assert_block(31, (byte*) testdata));
  EXPECT_THROW (area.assert_block(32, (byte*) testdata), AreaException);

  area.reset();
  EXPECT_EQ (area.used(), 0);
  EXPECT_EQ (area.available(), 37);
  EXPECT_EQ (area.initial(), 37);

  EXPECT_THROW (area.resize(0), AreaException);
  area.incr(20);
  EXPECT_THROW (area.resize(15), AreaException);
  area.resize(22);
  EXPECT_EQ (area.used(), 20);
  EXPECT_EQ (area.available(), 2);
  EXPECT_EQ (area.initial(), 22);
}


/**********************************************************************
 * Tests basic consistency properties of MemoryChunk assembly and
 * disassembly areas, including of their base classes AssemblyArea and
 * DisassemblyArea. Also tests "reversibility".
 *********************************************************************/


TEST (AreaTest, MemoryChunkTest) {
  // Tests checking error checking in constructor
  byte tmpdata [20];
  EXPECT_THROW (MemoryChunkAssemblyArea tmp0 ("test", 0), AssemblyAreaException);
  EXPECT_THROW (MemoryChunkAssemblyArea tmp1 ("test", 0, nullptr), AssemblyAreaException);
  EXPECT_THROW (MemoryChunkAssemblyArea tmp2 ("test", 10, nullptr), AssemblyAreaException);
  EXPECT_NO_THROW (MemoryChunkAssemblyArea tmp3 ("test", 20, tmpdata));

  byte buffer [100];
  MemoryChunkAssemblyArea area0 ("area0", 20, buffer);
  uint64_t u64_1 = 0x497E;
  uint64_t u64_2 = 0xFF00;
  uint32_t u32   = 0x6666;
  byte     b     = 0xFF;
  EXPECT_NO_THROW (area0.serialize_uint64_n(u64_1));
  EXPECT_NO_THROW (area0.serialize_uint64_n(u64_2));
  EXPECT_NO_THROW (area0.serialize_uint32_n(u32));
  EXPECT_THROW (area0.serialize_byte (b), AssemblyAreaException);
  
  MemoryChunkDisassemblyArea darea0 ("darea0", 20, buffer);
  uint64_t du64_1; darea0.deserialize_uint64_n (du64_1);
  EXPECT_EQ (du64_1, u64_1);
  uint64_t du64_2; darea0.deserialize_uint64_n (du64_2);
  EXPECT_EQ (du64_2, u64_2);
  uint32_t du32; darea0.deserialize_uint32_n (du32);
  EXPECT_EQ (du32, u32);
  EXPECT_THROW (darea0.deserialize_byte (), DisassemblyAreaException);


  MemoryChunkAssemblyArea     area1  ("area1", 21, buffer);
  MemoryChunkDisassemblyArea  darea1 ("darea1", 21, buffer);
  char testdata [] = "01234567890123456789";
  area1.serialize_byte_block (std::strlen(testdata)+1, (byte*) testdata);
  EXPECT_THROW (area1.serialize_byte (b), AssemblyAreaException);
  char testdataret [50];
  darea1.deserialize_byte_block (std::strlen(testdata)+1, (byte*) testdataret);
  EXPECT_THROW (darea1.deserialize_byte (), DisassemblyAreaException);
  EXPECT_THROW (darea1.peek_byte(), DisassemblyAreaException);
  EXPECT_STREQ (testdata, testdataret);
  EXPECT_EQ (area1.used(), 21);
  EXPECT_EQ (area1.available(), 0);
  EXPECT_EQ (darea1.used(), 21);
  EXPECT_EQ (darea1.available(), 0);


  MemoryChunkAssemblyArea area2 ("area2", 1);
  EXPECT_EQ (area2.available(), 1);
  EXPECT_EQ (area2.used(), 0);
  EXPECT_THROW (area2.serialize_uint16_n(0x497E), AssemblyAreaException);
  area2.serialize_byte (0xFF);
  EXPECT_EQ (area2.available(), 0);
  EXPECT_EQ (area2.used(), 1);
  MemoryChunkDisassemblyArea darea2 ("darea2", 1, area2.getBufferPtr());
  EXPECT_EQ (darea2.used(), 0);
  EXPECT_EQ (darea2.available(), 1);
  EXPECT_EQ (darea2.peek_byte(), 0xFF);
  EXPECT_EQ (darea2.used(), 0);
  EXPECT_EQ (darea2.available(), 1);
  EXPECT_EQ (darea2.deserialize_byte(), 0xFF);
  EXPECT_EQ (darea2.used(), 1);
  EXPECT_EQ (darea2.available(), 0);
}


/**********************************************************************
 * Tests basic consistency properties of ByteVector assembly and
 * disassembly areas, including of their base classes AssemblyArea and
 * DisassemblyArea. Also tests "reversibility".
 *********************************************************************/


TEST (AreaTest, ByteVectorTest) {
  // Tests checking error checking in constructor
  bytevect bvdummy (20);;
  EXPECT_THROW (ByteVectorAssemblyArea tmp0 ("test", 0), AssemblyAreaException);
  EXPECT_THROW (ByteVectorAssemblyArea tmp1 ("test", 0, bvdummy), AssemblyAreaException);
  EXPECT_NO_THROW (ByteVectorAssemblyArea tmp3 ("test", 20, bvdummy));

  bytevect bv0 (100);
  ByteVectorAssemblyArea area0 ("area0", 20, bv0);
  uint64_t u64_1 = 0x497E;
  uint64_t u64_2 = 0xFF00;
  uint32_t u32   = 0x6666;
  byte     b     = 0xFF;
  EXPECT_NO_THROW (area0.serialize_uint64_n(u64_1));
  EXPECT_NO_THROW (area0.serialize_uint64_n(u64_2));
  EXPECT_NO_THROW (area0.serialize_uint32_n(u32));
  EXPECT_THROW (area0.serialize_byte (b), AssemblyAreaException);

  bv0.resize(area0.used());
  ByteVectorDisassemblyArea darea0 ("darea0", bv0);
  EXPECT_EQ (bv0.size(), 20);
  uint64_t du64_1; darea0.deserialize_uint64_n (du64_1);
  EXPECT_EQ (du64_1, u64_1);
  uint64_t du64_2; darea0.deserialize_uint64_n (du64_2);
  EXPECT_EQ (du64_2, u64_2);
  uint32_t du32; darea0.deserialize_uint32_n (du32);
  EXPECT_EQ (du32, u32);
  EXPECT_THROW (darea0.deserialize_byte (), DisassemblyAreaException);

  bv0.resize(21);
  ByteVectorAssemblyArea     area1  ("area1", 21, bv0);
  ByteVectorDisassemblyArea  darea1 ("darea1", bv0);
  char testdata [] = "01234567890123456789";
  area1.serialize_byte_block (std::strlen(testdata)+1, (byte*) testdata);
  EXPECT_THROW (area1.serialize_byte (b), AssemblyAreaException);
  char testdataret [50];
  darea1.deserialize_byte_block (std::strlen(testdata)+1, (byte*) testdataret);
  EXPECT_THROW (darea1.deserialize_byte (), DisassemblyAreaException);
  EXPECT_THROW (darea1.peek_byte(), DisassemblyAreaException);
  EXPECT_STREQ (testdata, testdataret);
  EXPECT_EQ (area1.used(), 21);
  EXPECT_EQ (area1.available(), 0);
  EXPECT_EQ (darea1.used(), 21);
  EXPECT_EQ (darea1.available(), 0);

  ByteVectorAssemblyArea area2 ("area2", 1);
  EXPECT_EQ (area2.available(), 1);
  EXPECT_EQ (area2.used(), 0);
  EXPECT_THROW (area2.serialize_uint16_n(0x497E), AssemblyAreaException);
  area2.serialize_byte (0xFF);
  EXPECT_EQ (area2.available(), 0);
  EXPECT_EQ (area2.used(), 1);
  ByteVectorDisassemblyArea darea2 ("darea2", *area2.getVectorPtr());
  EXPECT_EQ (darea2.used(), 0);
  EXPECT_EQ (darea2.available(), 1);
  EXPECT_EQ (darea2.peek_byte(), 0xFF);
  EXPECT_EQ (darea2.used(), 0);
  EXPECT_EQ (darea2.available(), 1);
  EXPECT_EQ (darea2.deserialize_byte(), 0xFF);
  EXPECT_EQ (darea2.used(), 1);
  EXPECT_EQ (darea2.available(), 0);
}
