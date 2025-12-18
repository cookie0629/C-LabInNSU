#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>

#include "libcompressor/libcompressor.hpp"

static libcompressor_Buffer make_buf(const char* s) {
  return s ? libcompressor_Buffer{const_cast<char*>(s), (int)std::strlen(s)} : libcompressor_Buffer{nullptr, 0};
}

TEST(LibCompressor, ZlibBasic) {
  auto in = make_buf("abc");
  auto out = libcompressor_compress(libcompressor_Zlib, in);
  ASSERT_NE(out.data, nullptr);
  ASSERT_GT(out.size, 0);
  std::free(out.data);
}

TEST(LibCompressor, BzipBasic) {
  auto in = make_buf("abc");
  auto out = libcompressor_compress(libcompressor_Bzip, in);
  ASSERT_NE(out.data, nullptr);
  ASSERT_GT(out.size, 0);
  std::free(out.data);
}

TEST(LibCompressor, EmptyInput) {
  libcompressor_Buffer in{nullptr, 0};
  auto out1 = libcompressor_compress(libcompressor_Zlib, in);
  EXPECT_EQ(out1.data, nullptr);
  EXPECT_EQ(out1.size, 0);
  auto out2 = libcompressor_compress(libcompressor_Bzip, in);
  EXPECT_EQ(out2.data, nullptr);
  EXPECT_EQ(out2.size, 0);
}
