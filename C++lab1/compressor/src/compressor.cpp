#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "libcompressor/libcompressor.hpp"

/**
 * libcompressor 的 CLI 封装。
 * 将压缩后的数据以十六进制打印到标准输出，错误通过 spdlog 记录到标准错误。
 * CLI-обёртка над libcompressor.
 * Печатает сжатые данные в шестнадцатеричном виде в STDOUT, ошибки логируются через spdlog в STDERR.
 */
int main(int argc, char** argv) {
  spdlog::set_level(spdlog::level::err);

  if (argc < 3) {
    spdlog::error("Usage: compressor <zlib|bzip> <string>");
    return EXIT_FAILURE;
  }

  libcompressor_CompressionAlgorithm algo;
  std::string a = argv[1];
  if (a == "zlib")
    algo = libcompressor_Zlib;
  else if (a == "bzip")
    algo = libcompressor_Bzip;
  else {
    spdlog::error("Unknown algorithm: {}", a);
    return EXIT_FAILURE;
  }

  libcompressor_Buffer in{argv[2], (int)std::strlen(argv[2])};
  auto out = libcompressor_compress(algo, in);
  if (!out.data || out.size <= 0) {
    spdlog::error("Compression failed");
    return EXIT_FAILURE;
  }

  for (int i = 0; i < out.size; ++i) std::printf("%.2hhx", static_cast<unsigned char>(out.data[i]));

  std::printf("\n");
  std::free(out.data);
  return EXIT_SUCCESS;
}
