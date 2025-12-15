#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "include/context.h"
#include "include/model.h"

// 极简的 T-MAC 风格 pipeline，示意如何在 mllm/端侧框架中调度 MindSpore Lite。
class TMacPipeline {
 public:
  explicit TMacPipeline(std::shared_ptr<mindspore::lite::Model> model,
                        std::shared_ptr<mindspore::Context> ctx)
      : model_(std::move(model)), context_(std::move(ctx)) {}

  bool Run(const std::vector<float> &input, std::vector<float> &output) {
    if (!model_) {
      std::cerr << "[T-MAC] model is null\n";
      return false;
    }
    auto inputs = model_->GetInputs();
    if (inputs.empty()) {
      std::cerr << "[T-MAC] model has no inputs\n";
      return false;
    }
    const size_t bytes = input.size() * sizeof(float);
    if (bytes > inputs[0]->DataSize()) {
      std::cerr << "[T-MAC] input too large for tensor\n";
      return false;
    }
    std::memcpy(inputs[0]->MutableData(), input.data(), bytes);

    if (model_->Predict() != mindspore::kSuccess) {
      std::cerr << "[T-MAC] Predict failed\n";
      return false;
    }

    auto outputs = model_->GetOutputs();
    if (outputs.empty()) {
      std::cerr << "[T-MAC] model has no outputs\n";
      return false;
    }
    auto out_tensor = outputs[0];
    const float *out_data = reinterpret_cast<float *>(out_tensor->MutableData());
    const size_t out_len = out_tensor->ElementNum();
    output.assign(out_data, out_data + out_len);
    return true;
  }

 private:
  std::shared_ptr<mindspore::lite::Model> model_;
  std::shared_ptr<mindspore::Context> context_;
};

static bool LoadBinary(const std::string &path, std::vector<float> &buf) {
  std::ifstream fin(path, std::ios::binary);
  if (!fin) return false;
  fin.seekg(0, std::ios::end);
  const size_t sz = fin.tellg();
  fin.seekg(0, std::ios::beg);
  if (sz % sizeof(float) != 0) return false;
  buf.resize(sz / sizeof(float));
  fin.read(reinterpret_cast<char *>(buf.data()), sz);
  return fin.good();
}

int main(int argc, char **argv) {
  std::string model_path = "./models/demo_int8.ms";
  std::string input_path = "./data/input.bin";
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--model" && i + 1 < argc) model_path = argv[++i];
    else if (arg == "--input" && i + 1 < argc) input_path = argv[++i];
  }

  std::vector<float> input;
  if (!LoadBinary(input_path, input)) {
    std::cerr << "[demo] load input failed: " << input_path << "\n";
    return 1;
  }

  auto context = std::make_shared<mindspore::Context>();
  context->SetThreadNum(4);

  auto model = std::make_shared<mindspore::lite::Model>();
  auto ret = model->BuildFromFile(model_path.c_str(), mindspore::kMindIR_Lite, context);
  if (ret != mindspore::kSuccess) {
    std::cerr << "[demo] BuildFromFile failed: " << model_path << "\n";
    return 2;
  }

  TMacPipeline pipeline(model, context);
  std::vector<float> output;
  if (!pipeline.Run(input, output)) {
    std::cerr << "[demo] pipeline run failed\n";
    return 3;
  }

  std::cout << "[demo] output size: " << output.size() << "\n";
  for (size_t i = 0; i < std::min<size_t>(8, output.size()); ++i) {
    std::cout << "  out[" << i << "]=" << output[i] << "\n";
  }
  return 0;
}

