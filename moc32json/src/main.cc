#include <iostream>
#include <fstream>

#include "csm_allocate.h"
#include "csm_serialize.h"
#include "fio.h"

struct main_args {
  std::string input_file;
  bool output_required = false;
  std::string output_file;
  std::string model3_file;
  std::string cdi3_file;
} typedef main_args_t;

void print_usage()
{
  std::cout << "Usage: moc32json -o (json_output) [--model3 model3.json] [--cdi3 cdi3.json] <input_file.moc3>" << std::endl;
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
    } else if (now == "--model3" && i + 1 < argc) {
      args.model3_file = argv[i + 1];
      i++;
    } else if (now == "--cdi3" && i + 1 < argc) {
      args.cdi3_file = argv[i + 1];
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

  if (!args.model3_file.empty()) {
    const auto model3_bytes = read_f(&args.model3_file);
    if (model3_bytes.empty()) {
      std::cerr << "Failed to read model3 file: " << args.model3_file << std::endl;
      delete serializer;
      csm_controller->cleanup();
      delete csm_controller;
      return 1;
    }
    if (serializer->add_model3(model3_bytes) != 0) {
      std::cerr << "Failed to parse model3 file: " << args.model3_file << std::endl;
      delete serializer;
      csm_controller->cleanup();
      delete csm_controller;
      return 1;
    }
  }

  if (!args.cdi3_file.empty()) {
    const auto cdi3_bytes = read_f(&args.cdi3_file);
    if (cdi3_bytes.empty()) {
      std::cerr << "Failed to read cdi3 file: " << args.cdi3_file << std::endl;
      delete serializer;
      csm_controller->cleanup();
      delete csm_controller;
      return 1;
    }
    if (serializer->add_cdi3(cdi3_bytes) != 0) {
      std::cerr << "Failed to parse cdi3 file: " << args.cdi3_file << std::endl;
      delete serializer;
      csm_controller->cleanup();
      delete csm_controller;
      return 1;
    }
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
