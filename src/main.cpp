#include <cstring>
#include <memory>
#include <vector>

#include <opencv2/opencv.hpp>

#include <tensorflow/lite/c/c_api_types.h>
#include <tensorflow/lite/core/interpreter_builder.h>
#include <tensorflow/lite/core/kernels/register.h>
#include <tensorflow/lite/core/model_builder.h>
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/optional_debug_tools.h>

// NOTE: I really do think we should just use the singlepose model

constexpr int nearestMultiple(int x, int factor)
{
    return std::floor((x + factor) / factor) * factor;
}

struct Detection
{
    // The model detects 17 keypoints
    // The x and y are normalized to a range of 0.0 and 1.0
    // The order of the 17 keypoint joints are:
    // [
    //      nose, left eye, right eye, left ear, right ear,
    //      left shoulder, right shoulder, left elbow, right elbow,
    //      left wrist, right wrist, left hip, right hip, left knee,
    //      right knee, left ankle, right ankle
    // ]
    struct { float y, x, score; } keypoints[17];

    // The bounding box of the area that contains the person
    // The coordinates are normalized to a range of 0.0 and 1.0
    float ymin, xmin, ymax, xmax;

    // The confidence score of the detection
    float score;
};

int main()
{
    cv::Mat original, transformed;
    original = cv::imread("../assets/kpop.webp");
    assert(original.empty() == false);
    assert(original.channels() >= 3);

    // Resize and pad the top of the image such that the
    // width and height of the image are multiples of 32
    // For optimal model performance, the longer side
    // has a length of 512
    float w = original.size[0], h = original.size[1];

    cv::Size scaledSize = cv::Size(512, 512);
    cv::Size paddedSize = cv::Size(512, 512);

    // Hmmm...this seems dodgy...
    if (w > h) { // Horizontal scaling
        scaledSize.height = h * (w / scaledSize.width);
        paddedSize.height = nearestMultiple(scaledSize.height, 32);
    } else {     // Vertical scaling
        scaledSize.width = w * (h / scaledSize.height);
        paddedSize.width = nearestMultiple(scaledSize.width, 32);
    }

    cv::resize(original, transformed, scaledSize);
    cv::copyMakeBorder(
        transformed, transformed, paddedSize.width - scaledSize.width,
        0, 0, paddedSize.height - scaledSize.height, cv::BORDER_CONSTANT,
        {0, 0, 0}
    );

    // Load the model interpreter
    auto model =
        tflite::FlatBufferModel::BuildFromFile("../assets/movenet_multipose.tflite");
    assert(model != nullptr);

    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder builder(*model, resolver);
    std::unique_ptr<tflite::Interpreter> interpreter;
    builder(&interpreter);
    assert(interpreter != nullptr);

    // Load in our image into the input tensor. Since the input
    // tensor's dynamic, we need to set its dimensions
    auto dimensions = {1, paddedSize.height, paddedSize.width, 3};
    assert(interpreter->ResizeInputTensorStrict(0, dimensions) == kTfLiteOk);
    assert(interpreter->AllocateTensors() == kTfLiteOk);
    memcpy(
        interpreter->typed_input_tensor<unsigned char>(0),
        transformed.data,
        transformed.total() * transformed.elemSize()
    );
    assert(interpreter->Invoke() == kTfLiteOk);

    // Parse the model output
    Detection detections[6]; // The model can detect up to 6 people
    float* output = interpreter->typed_output_tensor<float>(0);
    assert(interpreter->output_tensor(0)->bytes == sizeof(detections));
    memcpy(detections, output, sizeof(detections));

    float confidenceThreshold = 0.3;
    for (Detection detection : detections) {
        if (detection.score < confidenceThreshold)
            continue; // bogus detection

        int xmin = detection.xmin * paddedSize.width;
        int ymin = detection.ymin * paddedSize.height;
        int xmax = detection.xmax * paddedSize.width;
        int ymax = detection.ymax * paddedSize.height;
        cv::rectangle(transformed, { xmin, ymin }, { xmax, ymax }, {0, 255, 0});
    }
    cv::imwrite("../output.jpg", transformed);

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
