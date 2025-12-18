#pragma once
#include <cstddef>

enum libcompressor_CompressionAlgorithm { libcompressor_Zlib, libcompressor_Bzip };

struct libcompressor_Buffer {
  char* data;
  int size;
};

/**
 * 使用所选算法压缩输入缓冲区。
 * Сжать входной буфер выбранным алгоритмом.
 * 根据 `algo` 压缩 `input`，返回新分配的压缩后缓冲区。
 * 调用者需要使用 `std::free` 释放返回的 `data`。
 */
libcompressor_Buffer libcompressor_compress(libcompressor_CompressionAlgorithm algo, libcompressor_Buffer input);
