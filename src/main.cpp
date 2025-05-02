#include <iostream>
#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>

#include <opencv2/opencv.hpp>

/*
the model outputs 39 keypoints (x, y, z, visibility, prescence):

nose, left_eye_inner, left_eye, left_eye_outer, right_eye_inner,
right_eye, right_eye_outer, left_ear, right_ear, mouth_left,
mouth_right, left_shoulder, right_shoulder, left_elbow, right_elbow,
left_wrist, right_wrist, left_pinky, right_pinky, left_index,
right_index, left_thumb, right_thumb, left_hip, right_hip,
left_knee, right_knee, left_ankle, right_ankle, left_heel,
right_heel, left_foot_index, right_foot_index, bodyCenter,
forehead, leftThumb, leftHand, rightThumb, rightHand

so the first output tensor is 1x195 (39 * 5)
maybe it's a good idea to make the keypoint confidence
sigmoid(min(prescence, visibility)) to
lowest score clamped to a range of 0 to 1
*/

int main()
{
    Ort::Env env;
    Ort::SessionOptions options(nullptr);
    Ort::Session session(env, "../assets/blazepose_lite.onnx", options);

    Ort::AllocatorWithDefaultOptions allocator;

    std::vector<std::string> inputNames;
    for (size_t i = 0; i < session.GetInputCount(); i++) {
        std::string tensorName = session.GetInputNameAllocated(i, allocator).get();
        inputNames.push_back(tensorName);

        auto typeInfo = session.GetInputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        std::vector<int64_t> shape = tensorInfo.GetShape();

        std::cout << tensorName << ": ";
        for (size_t j = 0; j < shape.size() - 1; j++) {
            std::cout << shape[j] << "x";
        }
        std::cout << shape[shape.size() - 1] << "\n";
    }

    std::vector<std::string> outputNames;
    for (size_t i = 0; i < session.GetOutputCount(); i++) {
        std::string tensorName = session.GetOutputNameAllocated(i, allocator).get();
        outputNames.push_back(tensorName);

        auto typeInfo = session.GetOutputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        std::vector<int64_t> shape = tensorInfo.GetShape();

        std::cout << tensorName << ": ";
        for (size_t j = 0; j < shape.size() - 1; j++) {
            std::cout << shape[j] << "x";
        }
        std::cout << shape[shape.size() - 1] << "\n";
    }

    cv::Mat image, normalized, scaled;
    image = cv::imread("../assets/images/NYCDP_Aran_0236.jpg");
    image.convertTo(normalized, CV_32F, 1.0 / 255, 0);
    cv::resize(normalized, scaled, cv::Size(256, 256));

    // TODO: now feed the image into an input tensor and parse the model output
    // auto outputs = session.Run(runOptions, inputNames, inputTensors, 1, outputNames, outputNames.size());
    // figuring out how to do this was none trivial. Maybe I should blog about this when
    // i figure out a demo
}