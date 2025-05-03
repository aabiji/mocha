#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "blazepose.h"

int main()
{
    BlazePose model("../assets/blazepose_lite.onnx");

    cv::Mat image, scaled, frame;
    std::string path = "../assets/images/360_F_474956815_jK8cl34vpfledXJmH99VIEjkQ9sKEAya.jpg";
    image = cv::imread(path);
    cv::resize(image, scaled, cv::Size(256, 256));
    scaled.convertTo(frame, CV_32F, 1.0 / 255, 0);

    model.inputFrame((float*)frame.data, frame.total() * frame.channels());
    Pose pose = model.runInference();

    cv::Scalar color = cv::Scalar(0, 255, 0);
    float ratioX = image.cols / 256.0;
    float ratioY = image.rows / 256.0;

    if (pose.detected) {
        for (auto pair : keypointConnections) {
            Keypoint a = pose.keypoints[pair.first];
            Keypoint b = pose.keypoints[pair.second];
            if (!a.detected() || !b.detected())
                continue;

            cv::Point pointA =
                cv::Point(a.x * 256.0 * ratioX, a.y * 256.0 * ratioY);
            cv::Point pointB =
                cv::Point(b.x * 256.0 * ratioX, b.y * 256.0 * ratioY);

            cv::circle(image, pointA, 1, color);
            cv::line(image, pointA, pointB, color);
        }
    }

    cv::imwrite("../output.jpg", image);
}