#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/model.h>

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

struct Keypoint
{
    float y, x;  // coordinate, in the range of 0 to 1
    float score; // confidence score
    bool detected() { return score > 0.3; }
};

class MoveNet
{
public:
    // Load the model from the path
    void init(const char* modelPath);

    // Run inference on the MoveNet model with an rgb image as input
    std::vector<Keypoint> runInference(unsigned char* image, glm::vec2 imageSize);

    // Are we ready to process the next frame?
    bool ready() { return readyForNextFrame; }

    // Get the position in the range of the original
    // image dimensions from the normalized xy coordinate
    glm::vec2 getPosition(float x, float y, float originalWidth, float originalHeight);
private:
    bool readyForNextFrame;
    std::unique_ptr<tflite::Interpreter> interpreter;
    std::unique_ptr<tflite::FlatBufferModel> model;
};
