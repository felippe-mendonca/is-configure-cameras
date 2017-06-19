#include <boost/program_options.hpp>
#include <iostream>
#include <is/is.hpp>
#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <vector>

namespace po = boost::program_options;
using namespace is::msg::camera;
using namespace is::msg::common;

int main(int argc, char* argv[]) {
  std::string uri;
  std::vector<std::string> cameras;
  float brightness_f;
  float exposure_f;
  float shutter_f;
  float gain_f;
  std::vector<unsigned int> wb;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("cameras,c", po::value<std::vector<std::string>>(&cameras)->multitoken(), "cameras");

  options("brightness,b", po::value<float>(&brightness_f), "brightness [1.367~7.422] (1.367)");
  options("exposure,e", po::value<float>(&exposure_f), "exposure [-7.585~2.414] (0.858)");
  options("shutter,s", po::value<float>(&shutter_f), "shutter [0~100%]");
  options("gain,g", po::value<float>(&gain_f), "gain [0~100%]");
  options("white-balance,wr", po::value<std::vector<unsigned int>>(&wb)->multitoken(), "white balance <red> <blue> [0~1023]");
  
  options("auto-exposure", "enables auto exposure mode");
  options("auto-shutter", "enables auto shutter mode");
  options("auto-gain", "enables auto gain mode");
  options("auto-wb", "enables auto white balance mode");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("cameras")) {
    std::cout << description << std::endl;
    return 1;
  }
  
  Configuration configuration;

  if (vm.count("brightness")) {
    configuration.brightness = brightness_f;
    is::log::info("Brightness: {}", brightness_f);
  }

  if (vm.count("exposure") || vm.count("auto-exposure")) {
    Exposure exposure;
    if (vm.count("exposure")) exposure.value = exposure_f;
    exposure.auto_mode = vm.count("auto-exposure");
    configuration.exposure = exposure;
    is::log::info("Exposure: {}", exposure.auto_mode.get() ? "auto" : std::to_string(exposure.value));
  }

  if (vm.count("shutter") || vm.count("auto-shutter")) {
    Shutter shutter;
    if (vm.count("shutter")) shutter.percent = shutter_f;
    shutter.auto_mode = vm.count("auto-shutter");
    configuration.shutter = shutter;
    is::log::info("Shutter: {}", shutter.auto_mode.get() ? "auto" : (std::to_string(shutter.percent.get()) + "\%"));
  }

  if (vm.count("gain") || vm.count("auto-gain")) {
    Gain gain;
    if (vm.count("gain")) gain.percent = gain_f;
    gain.auto_mode = vm.count("auto-gain");
    configuration.gain = gain;
    is::log::info("Gain: {}", gain.auto_mode.get() ? "auto" : (std::to_string(gain.percent.get()) + "\%"));
  }

  if (vm.count("white-balance") || vm.count("auto-wb")) {
    WhiteBalance white_balance;
    if (wb.size() > 1 ) {
      white_balance.red = wb[0];
      white_balance.blue = wb[1];
    }
    white_balance.auto_mode = vm.count("auto-wb");
    configuration.white_balance = white_balance;
    is::log::info("WhiteBalance: {}", white_balance.auto_mode.get() ? "auto" : (std::to_string(white_balance.red.get()) + "/" + std::to_string(white_balance.blue.get())));
  }

  auto is = is::connect(uri);
  auto client = is::make_client(is);

  for (auto& camera : cameras) {
    client.request(camera + ".configure", is::msgpack(configuration));
  }
  client.receive_for(1s);
  return 0;
}