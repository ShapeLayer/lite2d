#include <iostream>
#include <fstream>

#include "csm_allocate.h"
#include "csm_serialize.h"
#include "fio.h"

struct main_args {
  std::string input_file;
  bool output_required = false;
  std::string output_file;
} typedef main_args_t;

void print_usage()
{
  std::cout << "Usage: moc32json -o (json_output) <input_file.moc3>" << std::endl;
}

main_args_t parse_args(int argc, char *argv[])
{
  main_args_t args;
  std::string now;
  for (auto i = 1; i < argc; ++i) {
    now = argv[i];

    if (now == "-o" && i + 1 < argc) {
      args.output_required = true;
      args.output_file = argv[i + 1];
      i++;
    } else {
      args.input_file = now;
    }
  }
  return args;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    print_usage();
    return 1;
  }

  main_args_t args = parse_args(argc, argv);

  const std::string moc3file_path = args.input_file;
  csm_allocator_controller *csm_controller = new csm_allocator_controller();
  csm_serialize_controller *serializer = new csm_serialize_controller();


  const auto moc3_bytes = read_f(&moc3file_path);
  if (moc3_bytes.empty()) {
    std::cerr << "Failed to read file: " << moc3file_path << std::endl;
    return 1;
  }


  if (csm_controller->startup() != 0) {
    delete csm_controller;
    return 1;
  }

  if (csm_controller->create_moc(moc3_bytes.data(), static_cast<csm::csmSizeInt>(moc3_bytes.size())) != 0) {
    csm_controller->cleanup();
    delete csm_controller;
    return 1;
  }

  
  if (serializer->fetch_value(std::move(csm_controller->model)) != 0) {
    delete serializer;
    csm_controller->cleanup();
    delete csm_controller;
    return 1;
  }

  if (args.output_required) {
    std::ofstream ofs(args.output_file);
    if (!ofs) {
      std::cerr << "Failed to open output file: " << args.output_file << std::endl;
      delete serializer;
      csm_controller->cleanup();
      delete csm_controller;
      return 1;
    }
    ofs << serializer->output.dump(2) << std::endl;
  } else {
    std::cout << serializer->output.dump(2) << std::endl;
  }

  delete serializer;
  csm_controller->cleanup();
  delete csm_controller;

  return 0;
}
