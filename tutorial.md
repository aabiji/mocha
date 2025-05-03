In this tutorial, we'll use the [BlazePose GHUM 3d model](https://storage.googleapis.com/mediapipe-assets/Model%20Card%20BlazePose%20GHUM%203D.pdf)
to do 3d pose estimation. To run inference, we'll use the
ONNX runtime in C++. We'll feed in an image into the
model, and the model will output keypoints that we can use
to model and animate a 3d skeleton.

As an input, the model takes in a 256x256 RGB, float32 image
with values normalized to a range of 0 and 1.

The model outputs a list of 3d keypoints, segmentation masks and 3d world coordinates? (TODO: what are these: ld_3d, output_poseflag, output_segmentation, output_heatmap, world_3d).

In this tutorial we'll only use the 3d keypoints. The model outputs
39 different keypoints, in this format (x, y, z, visibility, precence)

![keypoints diagram](https://camo.githubusercontent.com/d3afebfc801ee1a094c28604c7a0eb25f8b9c9925f75b0fff4c8c8b4871c0d28/68747470733a2f2f6d65646961706970652e6465762f696d616765732f6d6f62696c652f706f73655f747261636b696e675f66756c6c5f626f64795f6c616e646d61726b732e706e67)