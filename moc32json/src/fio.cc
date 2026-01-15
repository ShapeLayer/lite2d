#include "fio.h"

#include <string>
#include <vector>
#include <fstream>

std::vector<csm::csmByte> read_f(const std::string *path)
{
  std::ifstream stream(*path, std::ios::binary);
  if (!stream) {
    return {};
  }
  stream.seekg(0, std::ios::end);
  const auto size = static_cast<size_t>(stream.tellg());
  stream.seekg(0, std::ios::beg);
  std::vector<csm::csmByte> buffer(size);
  stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size));
  return buffer;
}
