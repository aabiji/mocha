#pragma once

#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>

// Smoothly clamp to a range of 0 and 1
static constexpr float sigmoid(float x)
{
    return 1.0 / (1.0 + std::exp(-x));
}

const float detectionThreshold = 0.5;

const std::vector<std::pair<int, int>> keypointConnections = {
    {28, 32}, {32, 30}, {28, 30}, {27, 29}, {29, 31}, {27, 31},
    {26, 28}, {25, 27}, {24, 26}, {23, 25}, {12, 24}, {11, 23},
    {14, 12}, {16, 14}, {18, 16}, {20, 18}, {20, 16}, {22, 16},
    {11, 13}, {13, 15}, {15, 17}, {21, 15}, {19, 17}, {19, 15},
    {10,  9}, { 8,  6}, { 6,  5}, { 5,  4}, { 4,  0}, { 0,  1},
    { 1,  2}, { 2,  3}, { 3,  7}, {12, 11}, {24, 23}
};

struct Keypoint
{
    float x, y, z; // normalized to a range of -1.0 to 1.0

    float visibility;
    float prescence;

    bool detected()
    {
        float confidence = sigmoid(std::min(visibility, prescence));
        return confidence >= detectionThreshold;
    }
};

struct Pose
{
    bool detected;
    // Even though the model actually outputs 39 keypoints,
    // we'll only consider 33 of them since the last 6 are
    // always undetected
    Keypoint keypoints[33];
};

class BlazePose
{
public:
    BlazePose(std::string path) : session(env, path.c_str(), Ort::SessionOptions(nullptr))
    {
        Ort::MemoryInfo memoryInfo =
            Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

        // The model expects a 256x256 RGB image with values in a range of 0.0 to 1.0
        inputShape = {1, 256, 256, 3};
        inputData = std::vector<float>(256 * 256 * 3);
        inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo, inputData.data(), inputData.size(),
            inputShape.data(), inputShape.size());

        // 3D Keypoints
        outputShapes.push_back({1, 195});
        outputData.push_back(std::vector<float>(195));

        // Pose flag
        outputShapes.push_back({1, 1});
        outputData.push_back(std::vector<float>(1));

        for (size_t i = 0; i < outputData.size(); i++) {
            outputTensors.push_back(
                Ort::Value::CreateTensor<float>(
                    memoryInfo,
                    outputData[i].data(), outputData[i].size(),
                    outputShapes[i].data(), outputShapes[i].size()));
        }
    }

    // Run inference on the model and return the result of the pose estimation
    Pose runInference(float* pixelBuffer, int bufferSize)
    {
        // update the input tensor
        std::copy(pixelBuffer, pixelBuffer + bufferSize, inputData.data());

        // run inference
        Ort::RunOptions runOptions;
        session.Run(
            runOptions, inputNames, &inputTensor, 1,
            outputNames, outputTensors.data(), outputTensors.size());

        float* poseFlag = outputTensors[1].GetTensorMutableData<float>();
        float* data = outputTensors[0].GetTensorMutableData<float>();

        // parse the output tensors
        Pose pose;
        pose.detected = *poseFlag >= detectionThreshold;
        if (pose.detected) {
            int size = sizeof(pose.keypoints) / sizeof(Keypoint);
            for (int i = 0; i < size; i++) {
                Keypoint kp;
                kp.x = data[i * 5] / 256.0;
                kp.y = data[i * 5 + 1] / 256.0;
                kp.z = data[i * 5 + 2] / 256.0;
                kp.visibility = data[i * 5 + 3] / 256.0;
                kp.prescence = data[i * 5 + 4] / 256.0;
                pose.keypoints[i] = kp;
            }
        }

        return pose;
    }
private:
    Ort::Env env;
    Ort::Session session;

    std::vector<int64_t> inputShape;
    std::vector<float> inputData;
    Ort::Value inputTensor{nullptr};

    std::vector<std::vector<int64_t>> outputShapes;
    std::vector<std::vector<float>> outputData;
    std::vector<Ort::Value> outputTensors;

    const char* inputNames[1] = {"input_1"};
    const char* outputNames[2] = {"ld_3d", "output_poseflag"};
};