#include <boost/iterator/zip_iterator.hpp>
#include <boost/program_options.hpp>
#include <boost/tuple/tuple.hpp>
#include <chrono>
#include <iostream>
#include <is/is.hpp>
#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>
#include <string>
#include <vector>
#include "yaml-configure.hpp"

namespace po = boost::program_options;
using namespace is::msg::camera;
using namespace is::msg::common;

int main(int argc, char* argv[]) {
  std::string uri;
  std::vector<std::string> cameras;
  std::string yaml_file;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("cameras,c", po::value<std::vector<std::string>>(&cameras)->multitoken(), "cameras");
  options("yaml-file,y", po::value<std::string>(&yaml_file)->default_value("configuration.yaml"), "configuration file");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("cameras")) {
    std::cout << description << std::endl;
    return 1;
  }

  auto is = is::connect(uri);
  auto client = is::make_client(is);

  std::vector<std::string> ids;
  for (auto& camera : cameras) {
    ids.push_back(client.request(camera + ".get_configuration", is::msgpack(0)));
  }

  auto configuration_msgs =
      client.receive_until(std::chrono::high_resolution_clock::now() + 1s, ids, is::policy::discard_others);

  if (configuration_msgs.size() != cameras.size())
    exit(0);

  auto config_iterator_begin = boost::make_zip_iterator(boost::make_tuple(configuration_msgs.begin(), cameras.begin()));
  auto config_iterator_end = boost::make_zip_iterator(boost::make_tuple(configuration_msgs.end(), cameras.end()));
  std::map<std::string, Configuration> configurations;
  std::for_each(config_iterator_begin, config_iterator_end, [&](auto config) {
    auto configuration = is::msgpack<Configuration>(boost::get<0>(config).second);
    auto camera = boost::get<1>(config);
    configurations.emplace(camera, configuration);
  });

  is::camera::configuration::from_configurations(configurations, yaml_file);
  
  /*
    auto configurations = client.receive_until(std::chrono::high_resolution_clock::now() + 1s, ids,
    is::policy::discard_others);
    for (auto& config_msg : configurations) {
        auto configuration = is::msgpack<Configuration>(config_msg.second);
        auto resolution = *(configuration.resolution);
        is::log::warn("{}", config_msg.first);
        is::log::info("Resolution: {}x{}", resolution.width, resolution.height);
        auto sampling_rate = *(configuration.sampling_rate);
        is::log::info("SamplingRate: {} fps, {} ms", *(sampling_rate.rate), *(sampling_rate.period));
        is::log::info("ImageType: {}", (*configuration.image_type).value);
        is::log::info("Brightness: {}", *(configuration.brightness));
        auto exposure = *(configuration.exposure);
        is::log::info("Exposure: {}, {}", *(exposure.auto_mode) ? "auto" : "manual", exposure.value);
        auto shutter = *(configuration.shutter);
        is::log::info("Exposure: {}, {} \%, {} ms", *(shutter.auto_mode) ? "auto" : "manual", *(shutter.percent),
    *(shutter.ms));
        auto gain = *(configuration.gain);
        is::log::info("Gain: {}, {} \%, {} db", *(gain.auto_mode) ? "auto" : "manual", *(gain.percent), *(gain.db));
        auto white_balance = *(configuration.white_balance);
        is::log::info("WhiteBalance: {}, red: {}, blue {}", *(white_balance.auto_mode) ? "auto" : "manual",
    *(white_balance.red), *(white_balance.blue));
    }
  */

  return 0;
}