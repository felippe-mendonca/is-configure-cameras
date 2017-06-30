#ifndef __SLIDER_CONFIGURE_HPP__
#define __SLIDER_CONFIGURE_HPP__

#include <is/is.hpp>
#include <is/msgs/camera.hpp>
#include <is/msgs/common.hpp>
#include <map>
#include <nana/gui.hpp>
#include <nana/gui/widgets/checkbox.hpp>
#include <nana/gui/widgets/slider.hpp>
#include <string>

using namespace nana;
using namespace is::msg::camera;
using namespace is::msg::common;

namespace std {
std::ostream& operator<<(std::ostream& os, const std::vector<std::string>& vec) {
  for (auto item : vec) {
    os << item << " ";
  }
  return os;
}
}

const std::map<std::string, std::pair<double, double>> properties{
    {"Brightness", {1.367, 7.422}}, {"Exposure", {-7.585, 2.414}}, {"Shutter", {0.0, 100.0}},
    {"Gain", {0.0, 100.0}},         {"WB[red]", {0.0, 1023.0}},    {"WB[blue]", {0.0, 1023.0}}};

const std::map<std::string, bool> has_mode{{"Brightness", false}, {"Exposure", true}, {"Shutter", true},
                                           {"Gain", true},        {"WB[red]", true},  {"WB[blue]", true}};

const std::map<std::string, std::function<Configuration(unsigned int, unsigned int, bool)>> value_to_property{
    {"Brightness",
     [&](unsigned int value, unsigned int max, bool) {
       Configuration config;
       auto pmin = properties.at("Brightness").first;
       auto pmax = properties.at("Brightness").second;
       auto ratio = static_cast<double>(value) / static_cast<double>(max);
       config.brightness = static_cast<float>((pmax - pmin) * ratio + pmin);
       return config;
     }},
    {"Exposure",
     [&](unsigned int value, unsigned int max, bool mode) {
       Configuration config;
       Exposure exposure;
       auto pmin = properties.at("Exposure").first;
       auto pmax = properties.at("Exposure").second;
       auto ratio = static_cast<double>(value) / static_cast<double>(max);
       if (!mode) {
         exposure.value = static_cast<float>((pmax - pmin) * ratio + pmin);
       }
       exposure.auto_mode = mode;
       config.exposure = exposure;
       return config;
     }},
    {"Shutter",
     [&](unsigned int value, unsigned int max, bool mode) {
       Configuration config;
       Shutter shutter;
       auto pmin = properties.at("Shutter").first;
       auto pmax = properties.at("Shutter").second;
       auto ratio = static_cast<double>(value) / static_cast<double>(max);
       if (!mode) {
         shutter.percent = static_cast<float>((pmax - pmin) * ratio + pmin);
       }
       shutter.auto_mode = mode;
       config.shutter = shutter;
       return config;
     }},
    {"Gain",
     [&](unsigned int value, unsigned int max, bool mode) {
       Configuration config;
       Gain gain;
       auto pmin = properties.at("Gain").first;
       auto pmax = properties.at("Gain").second;
       auto ratio = static_cast<double>(value) / static_cast<double>(max);
       if (!mode) {
         gain.percent = static_cast<float>((pmax - pmin) * ratio + pmin);
       }
       gain.auto_mode = mode;
       config.gain = gain;
       return config;
     }},
    {"WB[red]",
     [&](unsigned int value, unsigned int max, bool mode) {
       Configuration config;
       WhiteBalance white_balance;
       auto pmin = properties.at("WB[red]").first;
       auto pmax = properties.at("WB[red]").second;
       auto ratio = static_cast<double>(value) / static_cast<double>(max);
       if (!mode) {
         white_balance.red = static_cast<unsigned int>((pmax - pmin) * ratio + pmin);
       }
       white_balance.auto_mode = mode;
       config.white_balance = white_balance;
       return config;
     }},
    {"WB[blue]",
     [&](unsigned int value, unsigned int max, bool mode) {
       Configuration config;
       WhiteBalance white_balance;
       auto pmin = properties.at("WB[blue]").first;
       auto pmax = properties.at("WB[blue]").second;
       auto ratio = static_cast<double>(value) / static_cast<double>(max);
       if (!mode) {
         white_balance.blue = static_cast<unsigned int>((pmax - pmin) * ratio + pmin);
       }
       white_balance.auto_mode = mode;
       config.white_balance = white_balance;
       return config;
     }},
};

const std::map<std::string, std::function<unsigned int(Configuration, unsigned int)>> property_to_value{
    {"Brightness",
     [&](Configuration c, unsigned int max) {
       auto pmin = properties.at("Brightness").first;
       auto pmax = properties.at("Brightness").second;
       auto value = *(c.brightness);
       return static_cast<unsigned int>(max * ((value - pmin) / (pmax - pmin)));
     }},
    {"Exposure",
     [&](Configuration c, unsigned int max) {
       auto pmin = properties.at("Exposure").first;
       auto pmax = properties.at("Exposure").second;
       auto value = (*(c.exposure)).value;
       return static_cast<unsigned int>(max * ((value - pmin) / (pmax - pmin)));
     }},
    {"Shutter",
     [&](Configuration c, unsigned int max) {
       auto pmin = properties.at("Shutter").first;
       auto pmax = properties.at("Shutter").second;
       auto shutter = *(c.shutter);
       auto value = *(shutter.percent);
       return static_cast<unsigned int>(max * ((value - pmin) / (pmax - pmin)));
     }},
    {"Gain",
     [&](Configuration c, unsigned int max) {
       auto pmin = properties.at("Gain").first;
       auto pmax = properties.at("Gain").second;
       auto gain = *(c.gain);
       auto value = *(gain.percent);
       return static_cast<unsigned int>(max * ((value - pmin) / (pmax - pmin)));
     }},
    {"WB[red]",
     [&](Configuration c, unsigned int max) {
       auto pmin = properties.at("WB[red]").first;
       auto pmax = properties.at("WB[red]").second;
       auto white_balance = *(c.white_balance);
       auto value = *(white_balance.red);
       return static_cast<unsigned int>(max * ((value - pmin) / (pmax - pmin)));
     }},
    {"WB[blue]",
     [&](Configuration c, unsigned int max) {
       auto pmin = properties.at("WB[blue]").first;
       auto pmax = properties.at("WB[blue]").second;
       auto white_balance = *(c.white_balance);
       auto value = *(white_balance.blue);
       return static_cast<unsigned int>(max * ((value - pmin) / (pmax - pmin)));
     }},
};

const std::map<std::string, std::function<bool(Configuration)>> property_mode{
    {"Brightness", [&](Configuration) { return false; }},
    {"Exposure",
     [&](Configuration c) {
       auto exposure = *(c.exposure);
       return *(exposure.auto_mode);
     }},
    {"Shutter",
     [&](Configuration c) {
       auto shutter = *(c.shutter);
       return *(shutter.auto_mode);
     }},
    {"Gain",
     [&](Configuration c) {
       auto gain = *(c.gain);
       return *(gain.auto_mode);
     }},
    {"WB[red]",
     [&](Configuration c) {
       auto wb = *(c.white_balance);
       return *(wb.auto_mode);
     }},
    {"WB[blue]",
     [&](Configuration c) {
       auto wb = *(c.white_balance);
       return *(wb.auto_mode);
     }},
};

void request_configuration(is::ServiceClient client, std::string const& camera, Configuration configuration) {
  auto id = client.request(camera + ".set_configuration", is::msgpack(configuration));
  client.receive_for(1s, id, is::policy::discard_others);
}

void update_values(is::ServiceClient client, std::string const& camera,
                   std::map<std::string, std::shared_ptr<slider>>& sliders,
                   std::map<std::string, std::shared_ptr<checkbox>>& cboxes, bool just_auto = false) {
  auto id = client.request(camera + ".get_configuration", is::msgpack(0));
  auto config_msg = client.receive_for(1s, id, is::policy::discard_others);

  if (config_msg == nullptr)
    return;

  auto configuration = is::msgpack<Configuration>(config_msg);
  for (auto& s : sliders) {
    auto property = s.first;
    auto slider = s.second;
    auto mode = property_mode.at(property)(configuration);
    if (just_auto && !mode)
      continue;
    slider->value(property_to_value.at(property)(configuration, 1000));
    if (has_mode.at(property))
      cboxes.at(property)->check(mode);
  }
}

#endif  // __SLIDER_CONFIGURE_HPP__
