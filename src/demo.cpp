#include <memory>
#include <opencv2/core/utility.hpp>
#include <vector>

#include <opencv2/opencv.hpp>

#include <tensorflow/lite/c/c_api_types.h>
#include <tensorflow/lite/core/interpreter_builder.h>
#include <tensorflow/lite/core/kernels/register.h>
#include <tensorflow/lite/core/model_builder.h>
#include <tensorflow/lite/optional_debug_tools.h>

struct Keypoint
{
    // the x and y are normalized to a range of 0 and 1
    float y, x, score;
    bool detected() { return score > 0.3; };
};

// the indexes of the keypoints that should be connected
// the order of the 17 keypoint joints are:
// [ nose, left eye, right eye, left ear, right ear,
//   left shoulder, right shoulder, left elbow, right elbow,
//   left wrist, right wrist, left hip, right hip, left knee,
//   right knee, left ankle, right ankle ]
const std::vector<std::pair<int, int>> keypointConnections = {
    {0, 1}, {0, 2}, {1, 3}, {2, 4}, {5, 6}, {5, 7}, {7, 9},
    {6, 8}, {8, 10}, {5, 11}, {6, 12}, {11, 12}, {11, 13},
    {13, 15}, {12, 14}, {14, 16}
};

// movenet requires the input image to be 192x192
const cv::Size targetSize = cv::Size(192, 192);

std::vector<Keypoint> runInference(
    std::unique_ptr<tflite::Interpreter>& interpreter,
    cv::Mat frame
) {
    memcpy(
        interpreter->typed_input_tensor<unsigned char>(0),
        frame.data,
        frame.total() * frame.elemSize()
    );
    assert(interpreter->Invoke() == kTfLiteOk);
    float* output = interpreter->typed_output_tensor<float>(0);

    std::vector<Keypoint> points;
    for (int i = 0; i < 17; i++) {
        points.push_back({output[i * 3], output[i * 3 + 1], output[i * 3 + 2]});
    }
    return points;
}

void drawSkeleton(
    cv::Mat& image,
    std::vector<Keypoint>& keypoints,
    float xScale, float yScale
) {
    cv::Scalar color = {0, 255, 0};

    for (Keypoint p : keypoints) {
        if (!p.detected()) continue;
        cv::Point xy = cv::Point(
            p.x * targetSize.width  * xScale,
            p.y * targetSize.height * yScale
        );
        cv::circle(image, xy, 3, color);
    }

    for (auto p : keypointConnections) {
        cv::Point a = cv::Point(
            keypoints[p.first].x * targetSize.width  * xScale,
            keypoints[p.first].y * targetSize.height * yScale
        );
        cv::Point b = cv::Point(
            keypoints[p.second].x * targetSize.width  * xScale,
            keypoints[p.second].y * targetSize.height * yScale
        );
        if (keypoints[p.first].detected() &&
            keypoints[p.second].detected())
            cv::line(image, a, b, color, 2, cv::LINE_AA);
    }
}

int main()
{
    // Load the model interpreter
    auto model =
        tflite::FlatBufferModel::BuildFromFile("../assets/movenet_singlepose.tflite");
    assert(model != nullptr);

    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder builder(*model, resolver);
    std::unique_ptr<tflite::Interpreter> interpreter;
    builder(&interpreter);
    assert(interpreter != nullptr);
    assert(interpreter->AllocateTensors() == kTfLiteOk);

    std::string name = "webcam";
    int w = 700, h = 500;

    cv::VideoCapture camera(0);
    assert(camera.isOpened());
    camera.set(cv::CAP_PROP_FRAME_WIDTH, w);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, h);

    cv::namedWindow(name, cv::WINDOW_NORMAL);
    cv::resizeWindow(name, w, h);

    cv::Mat frame, scaled;
    cv::TickMeter meter;

    while (true) {
        meter.start();
        camera >> frame;

        cv::resize(frame, scaled, targetSize);
        auto keypoints = runInference(interpreter, scaled);

        // Horizontal and vertical scaling factors
        float xScale = float(frame.size[1]) / float(targetSize.width);
        float yScale = float(frame.size[0]) / float(targetSize.height);
        drawSkeleton(frame, keypoints, xScale, yScale);

        cv::imshow(name, frame);
        meter.stop();

        if (cv::waitKey(meter.getFPS()) == 27)
            break; // press escape to quit
    }
}
