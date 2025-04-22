#include <opencv2/opencv.hpp>

#include <tensorflow/lite/core/interpreter_builder.h>
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model_builder.h>
#include <tensorflow/lite/optional_debug_tools.h>

int main()
{
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
}
