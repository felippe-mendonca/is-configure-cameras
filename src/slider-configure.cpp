#include "slider-configure.hpp"
#include <algorithm>
#include <atomic>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <is/is.hpp>
#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>
#include <list>
#include <map>
#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/checkbox.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/slider.hpp>
#include <string>
#include <thread>
#include <vector>
#include "yaml-configure.hpp"

using namespace std;
using namespace std::chrono;
using namespace nana;
using namespace is::msg::camera;
using namespace is::msg::common;
namespace po = boost::program_options;

const unsigned int slider_width = 200;
const unsigned int slider_height = 30;
const unsigned int X0 = 50;
const unsigned int Y0 = 50;
const unsigned int slider_vspacing = 10;
const unsigned int slider_hspacing = 10;

int main(int argc, char* argv[]) {
  std::string uri;
  std::vector<std::string> cameras;
  const std::vector<std::string> default_cameras{"ptgrey.0", "ptgrey.1", "ptgrey.2", "ptgrey.3"};
  std::string yaml_file;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<string>(&uri)->default_value("amqp://edge.is:30000"), "broker uri");
  options("cameras,c", po::value<vector<string>>(&cameras)->multitoken()->default_value(default_cameras), "cameras");
  options("yaml-file,y", po::value<std::string>(&yaml_file)->default_value("configuration.yaml"), "configuration file");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << description << std::endl;
    return 1;
  }

  // Get initial parameters
  auto is = is::connect(uri);
  auto client = is::make_client(is);

  std::vector<std::string> ids;
  for (auto& camera : cameras) {
    ids.push_back(client.request(camera + ".get_configuration", is::msgpack(0)));
  }

  std::unordered_map<std::string, is::Envelope::ptr_t> configuration_msgs;
  for (int i = 0; i < 5; ++i) {
    is::log::info("Requesting cameras configuration... {}/5", i + 1);
    configuration_msgs = client.receive_until(high_resolution_clock::now() + 2s, ids, is::policy::discard_others);
    if (configuration_msgs.size() == cameras.size())
      break;
    if (i == 4) {
      is::log::warn("Failed. Exiting...");
      exit(0);
    } else {
      is::log::warn("Failed.");
    }
  }

  std::map<std::string, Configuration> configurations;

  auto config_iterator_begin = boost::make_zip_iterator(boost::make_tuple(configuration_msgs.begin(), cameras.begin()));
  auto config_iterator_end = boost::make_zip_iterator(boost::make_tuple(configuration_msgs.end(), cameras.end()));
  std::for_each(config_iterator_begin, config_iterator_end, [&](auto config) {
    auto configuration = is::msgpack<Configuration>(config.get<0>().second);
    auto camera = config.get<1>();
    configurations.emplace(camera, configuration);
  });

  auto n_properties = properties.size();
  form fm(rectangle(0, 0, 2 * slider_width, 1.6 * (slider_height + slider_vspacing) * n_properties));
  fm.caption("is::CameraParameters");

  unsigned int x = X0;
  unsigned int y = Y0;
  unsigned int inc_x = 0;
  std::shared_ptr<label> camera_label = std::make_shared<label>(fm, rectangle(x, y, slider_width, slider_height));
  camera_label->caption("Camera");
  y += (slider_height + slider_vspacing);
  std::vector<std::shared_ptr<label>> properties_labels;
  for (auto p : properties) {
    std::shared_ptr<label> lb = std::make_shared<label>(fm, rectangle(x, y, slider_width, slider_height));
    lb->caption(p.first);
    inc_x = lb->measure(0).width > inc_x ? lb->measure(0).width : inc_x;
    properties_labels.push_back(lb);
    y += (slider_height + slider_vspacing);
  }
  x += (inc_x + slider_hspacing);

  std::map<std::string, std::shared_ptr<slider>> sliders;
  std::map<std::string, std::shared_ptr<checkbox>> cboxes;
  std::atomic<std::size_t> camera;
  std::atomic<bool> update_all{false};
  camera.store(0);

  y = Y0;
  combox cameras_list(fm, rectangle(x, y, slider_width, slider_height));
  for (auto& camera : cameras) {
    cameras_list.push_back(camera);
  }
  cameras_list.option(0);

  cameras_list.events().selected([&](auto) {
    camera.store(cameras_list.option());
    update_all.store(true);
    is::log::info("Selected camera {}", cameras.at(cameras_list.option()));
  });

  y += (slider_height + slider_vspacing);
  for (auto& p : properties) {
    auto property = p.first;
    rectangle rec(x, y, slider_width, slider_height);
    std::shared_ptr<slider> sl = std::make_shared<slider>(fm, rec);
    sl->maximum(1000);
    sl->events().mouse_up([&, sl, property]() {
      if (has_mode.at(property) && cboxes.at(property)->checked()) {
        cboxes.at(property)->check(false);
      }
      is::log::info("[{}|{}|manual|{}]", cameras.at(camera.load()), property, sl->value());
      request_configuration(client, cameras.at(camera.load()),
                            value_to_property.at(property)(sl->value(), 1000, false));
    });
    sl->vernier([&](unsigned int maximum, unsigned int cursor_value) {
      auto percentage = static_cast<double>(cursor_value) / static_cast<double>(maximum);
      return std::to_string((p.second.second - p.second.first) * percentage + p.second.first);
    });
    sliders.emplace(property, sl);

    rec.x += slider_width + slider_hspacing;
    if (has_mode.at(property)) {
      std::shared_ptr<checkbox> cb = std::make_shared<checkbox>(fm, rec);
      cb->events().mouse_up([&, cb, property]() {
        auto mode = cb->checked();
        if (property == "WB[red]") {
          cboxes.at("WB[blue]")->check(mode);
          if (!mode) {
            auto blue_value = sliders.at("WB[blue]")->value();
            request_configuration(client, cameras.at(camera.load()),
                                  value_to_property.at("WB[blue]")(blue_value, 1000, mode));
          }
        }
        if (property == "WB[blue]") {
          cboxes.at("WB[red]")->check(mode);
          if (!mode) {
            auto red_value = sliders.at("WB[red]")->value();
            request_configuration(client, cameras.at(camera.load()),
                                  value_to_property.at("WB[red]")(red_value, 1000, mode));
          }
        }
        is::log::info("[{}|{}|{}]", cameras.at(camera.load()), property, mode ? "auto" : "manual");
        auto value = sliders.at(property)->value();
        request_configuration(client, cameras.at(camera.load()), value_to_property.at(property)(value, 1000, mode));
      });
      cboxes.emplace(property, cb);
    }
    y += (slider_height + slider_vspacing);
  }

  update_values(client, cameras.at(camera.load()), sliders, cboxes);

  button save_bt(fm, rectangle(X0, y, slider_width / 2, slider_height));
  save_bt.caption("Save");
  save_bt.events().mouse_up([&]() {
    std::vector<std::string> ids;
    for (auto& camera : cameras) {
      ids.push_back(client.request(camera + ".get_configuration", is::msgpack(0)));
    }
    auto configuration_msgs = client.receive_until(high_resolution_clock::now() + 2s, ids, is::policy::discard_others);
    if (configuration_msgs.size() != cameras.size()) {
      is::log::warn("Failed on requesting cameras parameters. Try again.");
      return;
    }
    auto config_iterator_begin =
        boost::make_zip_iterator(boost::make_tuple(configuration_msgs.begin(), cameras.begin()));
    auto config_iterator_end = boost::make_zip_iterator(boost::make_tuple(configuration_msgs.end(), cameras.end()));
    std::map<std::string, Configuration> configurations;
    std::for_each(config_iterator_begin, config_iterator_end, [&](auto config) {
      auto configuration = is::msgpack<Configuration>(boost::get<0>(config).second);
      auto camera = boost::get<1>(config);
      configurations.emplace(camera, configuration);
    });

    is::log::info("Saving parameters on configuration.yaml");
    is::camera::configuration::from_configurations(configurations, yaml_file);
  });

  fm.show();

  std::atomic_bool running{true};
  std::thread refresh_values([&]() {
    auto is = is::connect(uri);
    auto client = is::make_client(is);
    while (running) {
      auto start = std::chrono::high_resolution_clock::now();
      update_values(client, cameras.at(camera.load()), sliders, cboxes, !update_all.load());
      update_all.store(false);
      std::this_thread::sleep_until(start + 1s);
    }
  });

  fm.events().destroy([&running]() { running = false; });
  exec();
  refresh_values.join();
}