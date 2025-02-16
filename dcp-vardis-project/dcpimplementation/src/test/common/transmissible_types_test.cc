#include <cstdint>
#include <gtest/gtest.h>
#include <dcp/common/area.h>
#include <dcp/common/transmissible_type.h>

namespace dcp {

  // ------------------------------------------------------------
  
  TEST(TTTest, TransmissibleIntegralTest_Basic) {
    TransmissibleIntegral<uint8_t> u1 (10);
    TransmissibleIntegral<uint8_t> u2 (10);
    TransmissibleIntegral<uint8_t> u3 (11);
    TransmissibleIntegral<uint8_t> u4 (0);
    TransmissibleIntegral<uint8_t> u5 (20);
    EXPECT_EQ (u1, 10);
    EXPECT_NE (u1, 11);
    EXPECT_EQ (u1, u2);
    EXPECT_NE (u1, u3);
    EXPECT_EQ (u1 - u2, 0);
    EXPECT_EQ (u1 - u2, u4);
    EXPECT_EQ (u1 + u2, u5);
    EXPECT_EQ (u1 + u2, 20);
    EXPECT_GT (u3, u2);
    EXPECT_GT (u3, 9);
    EXPECT_LT (u2, u3);
    EXPECT_LT (u2, 11);
    
    TransmissibleIntegral<uint8_t> u6 (10);
    EXPECT_EQ (u6++, u3);
    EXPECT_EQ (u6--, u2);


    TransmissibleIntegral<uint8_t>   us8;
    TransmissibleIntegral<uint16_t>  us16;
    TransmissibleIntegral<uint32_t>  us32;
    TransmissibleIntegral<uint64_t>  us64;
    EXPECT_EQ(us8.fixed_size(), 1);
    EXPECT_EQ(us16.fixed_size(), 2);
    EXPECT_EQ(us32.fixed_size(), 4);
    EXPECT_EQ(us64.fixed_size(), 8);
  }

  // ------------------------------------------------------------

  TEST(TTTest, TransmissibleIntegralTest_Serialization) {
    const size_t bufsize = 1024;
    char buffer [bufsize];
    
    TransmissibleIntegral<uint8_t> u1 (0x7E);
    MemoryChunkAssemblyArea ass8 ("test-ass", bufsize, (byte*) buffer);
    u1.serialize (ass8);
    TransmissibleIntegral<uint8_t> u2;
    MemoryChunkDisassemblyArea dass8 ("test-dass", bufsize, (byte*) buffer);
    u2.deserialize (dass8);
    EXPECT_EQ (u1, u2);

    TransmissibleIntegral<uint16_t> u3 (0x497E);
    MemoryChunkAssemblyArea ass16 ("test-ass", bufsize, (byte*) buffer);
    u3.serialize (ass16);
    TransmissibleIntegral<uint16_t> u4;
    MemoryChunkDisassemblyArea dass16 ("test-dass", bufsize, (byte*) buffer);
    u4.deserialize (dass16);
    EXPECT_EQ (u3, u4);

    TransmissibleIntegral<uint32_t> u5 (0x497E497E);
    MemoryChunkAssemblyArea ass32 ("test-ass", bufsize, (byte*) buffer);
    u5.serialize (ass32);
    TransmissibleIntegral<uint32_t> u6;
    MemoryChunkDisassemblyArea dass32 ("test-dass", bufsize, (byte*) buffer);
    u6.deserialize (dass32);
    EXPECT_EQ (u5, u6);

    TransmissibleIntegral<uint64_t> u7 (0x497E123409870666);
    MemoryChunkAssemblyArea ass64 ("test-ass", bufsize, (byte*) buffer);
    u7.serialize (ass64);
    TransmissibleIntegral<uint64_t> u8;
    MemoryChunkDisassemblyArea dass64 ("test-dass", bufsize, (byte*) buffer);
    u8.deserialize (dass64);
    EXPECT_EQ (u7, u8);
    
  }

  // ------------------------------------------------------------
}
