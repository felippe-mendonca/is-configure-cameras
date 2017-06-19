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
  Resolution resolution;
  SamplingRate sample_rate;
  double fps;
  std::string image_type;

  po::options_description description("Allowed options");
  auto&& options = description.add_options();
  options("help,", "show available options");
  options("uri,u", po::value<std::string>(&uri)->default_value("amqp://localhost"), "broker uri");
  options("cameras,c", po::value<std::vector<std::string>>(&cameras)->multitoken(), "cameras");
  options("height,h", po::value<unsigned int>(&resolution.height)->default_value(728), "image height");
  options("width,w", po::value<unsigned int>(&resolution.width)->default_value(1288), "image width");
  options("fps,f", po::value<double>(&fps)->default_value(5.0), "frames per second");
  options("type,t", po::value<std::string>(&image_type)->default_value("rgb"), "image type");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, description), vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("cameras")) {
    std::cout << description << std::endl;
    return 1;
  }

  auto is = is::connect(uri);
  auto client = is::make_client(is);

  sample_rate.rate = fps;
  for (auto& camera : cameras) {
    client.request(camera + ".set_sample_rate", is::msgpack(sample_rate));
    client.request(camera + ".set_resolution", is::msgpack(resolution));
    client.request(camera + ".set_image_type", is::msgpack(ImageType{image_type}));
  }
  client.receive_for(1s);

  std::vector<std::string> topics;
  for (auto& camera : cameras) {
    topics.push_back(camera + ".frame");
  }
  auto tag = is.subscribe(topics);

  SyncRequest sr;
  sr.entities = cameras;
  sr.sampling_rate = sample_rate;
  is::logger()->info("Sync request");
  client.request("is.sync", is::msgpack(sr));

  is::logger()->info("Starting capture");

  while (1) {
    auto images_msg = is.consume_sync(tag, topics, static_cast<int64_t>(1000.0 / fps));

    std::vector<cv::Mat> up_frames, down_frames;
    int n_frame = 0;
    for (auto& msg : images_msg) {
      auto image = is::msgpack<CompressedImage>(msg);
      cv::Mat current_frame = cv::imdecode(image.data, CV_LOAD_IMAGE_COLOR);
      cv::resize(current_frame, current_frame, cv::Size(current_frame.cols / 2, current_frame.rows / 2));
      if (n_frame < 2) {
        up_frames.push_back(current_frame);
      } else {
        down_frames.push_back(current_frame);
      }
      n_frame++;
    }

    cv::Mat output_image;
    cv::Mat up_row, down_row;
    std::vector<cv::Mat> rows_frames;
    cv::hconcat(up_frames, up_row);
    rows_frames.push_back(up_row);
    cv::hconcat(down_frames, down_row);
    rows_frames.push_back(down_row);
    cv::vconcat(rows_frames, output_image);

    cv::imshow("Intelligent Space", output_image);
    cv::waitKey(1);
  }

  is::logger()->info("Exiting");
  return 0;
}