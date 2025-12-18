#include "libcompressor/libcompressor.hpp"

#include <bzlib.h>
#include <zlib.h>

#include <cstdlib>
#include <cstring>

namespace {
libcompressor_Buffer err() { return {nullptr, 0}; }
}  // namespace

/**
 * 使用所选算法压缩输入缓冲区。
 * Сжать входной буфер, используя выбранный алгоритм.
 */
libcompressor_Buffer libcompressor_compress(libcompressor_CompressionAlgorithm algo, libcompressor_Buffer input) {
  if (!input.data || input.size <= 0) return err();

  const int out_cap = input.size + 1024;
  char* out = static_cast<char*>(std::malloc(static_cast<size_t>(out_cap)));
  if (!out) return err();

  int out_size = 0;

  if (algo == libcompressor_Zlib) {
    uLongf dest = static_cast<uLongf>(out_cap);
    int rc = compress2(reinterpret_cast<Bytef*>(out), &dest, reinterpret_cast<const Bytef*>(input.data),
                       static_cast<uLong>(input.size), Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK) {
      std::free(out);
      return err();
    }
    out_size = static_cast<int>(dest);
  } else if (algo == libcompressor_Bzip) {
    unsigned int dest = static_cast<unsigned int>(out_cap);
    int rc = BZ2_bzBuffToBuffCompress(out, &dest, const_cast<char*>(input.data), static_cast<unsigned int>(input.size),
                                      1, 0, 0);
    if (rc != BZ_OK) {
      std::free(out);
      return err();
    }
    out_size = static_cast<int>(dest);
  } else {
    std::free(out);
    return err();
  }

  return {out, out_size};
}
