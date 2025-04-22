#include <memory>
#include <vector>

#include <opencv2/opencv.hpp>

#include <tensorflow/lite/c/c_api_types.h>
#include <tensorflow/lite/core/interpreter_builder.h>
#include <tensorflow/lite/core/kernels/register.h>
#include <tensorflow/lite/core/model_builder.h>
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/optional_debug_tools.h>

int main()
{
    cv::Mat original, scaled, image;
    original = cv::imread("../assets/dancer.jpg");
    assert(original.empty() == false);

    cv::Size target = cv::Size(192, 192);
    float ratioX = float(original.size[0]) / float(target.width);
    float ratioY = float(original.size[1]) / float(target.height);

    cv::resize(original, scaled, target);
    cv::cvtColor(scaled, image, cv::COLOR_BGR2RGB);

    auto model = tflite::FlatBufferModel::BuildFromFile("../assets/movenet_singlepose.tflite");
    assert(model != nullptr);

    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder builder(*model, resolver);
    std::unique_ptr<tflite::Interpreter> interpreter;
    builder(&interpreter);
    assert(interpreter != nullptr);

    assert(interpreter->AllocateTensors() == kTfLiteOk);
    memcpy(
        interpreter->typed_input_tensor<unsigned char>(0),
        image.data,
        image.total() * image.elemSize()
    );
    assert(interpreter->Invoke() == kTfLiteOk);

    float* output = interpreter->typed_output_tensor<float>(0);

    const int numKeypoints = 17;
    const float confidenceThreshold = 0.3;
    std::vector<cv::Point2f> keypoints;
    for (int i = 0; i < numKeypoints; i++) {
        float y = output[i * 3];
        float x = output[i * 3 + 1];
        float confidence = output[i * 3 + 2];
        if (confidence > confidenceThreshold)
            keypoints.push_back(cv::Point2f(x, y));
    }

    cv::Scalar color = {0, 255, 0};
    for (size_t i = 0; i < keypoints.size(); i++) {
        auto& point = keypoints[i];
        cv::Point coordinate = {
            int(point.x * target.width * ratioX),
            int(point.y * target.height * ratioY)
        };
        cv::circle(original, coordinate, 3, color);

        if (i > 0) {
            auto& prev = keypoints[i - 1];
            cv::Point prevCoordinate = {
                int(prev.x * target.width * ratioX),
                int(prev.y * target.height * ratioY)
            };
            cv::line(original, prevCoordinate, coordinate, color);
        }
    }

    cv::imwrite("../output.jpg", original);

    /*
    std::string name = "webcam";
    int w = 700, h = 500;
    const float fps = 1000.0 / 60.0;

    cv::VideoCapture camera(0);
    assert(camera.isOpened());
    camera.set(cv::CAP_PROP_FRAME_WIDTH, w);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, h);

    cv::namedWindow(name, cv::WINDOW_NORMAL);
    cv::resizeWindow(name, w, h);

    cv::Mat frame;
    while (true) {
        camera >> frame;
        cv::imshow(name, frame);
        if (cv::waitKey(fps) == 27)
            break; // press escape to quit
    }
    */
}
