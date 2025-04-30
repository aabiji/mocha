Could use [MotionBert](https://github.com/Walter0807/MotionBERT)
to do 3d pose estimation. The one problem is whether it's real time or not.

If it's not performant enough to be in real time I think we might
really just have to stick with 2d keypoints. But then how would we
map that to 3d bone transformations????

Usng MotionBERT:
- Extract 2d keypoints using Alphapose
    - Use the FastPoint [model](https://github.com/MVIG-SJTU/AlphaPose/blob/master/docs/MODEL_ZOO.md#halpe-dataset-26-keypoints)
- Convert into the required keypoint format
- Feed into MotionBERT

We should ditch tensorflow and use the C++ frontend for
pytorch in order to run inference on the models seamlessly:
https://pytorch.org/tutorials/advanced/cpp_frontend.html

Will we need to port the DSTFormer model over to C++??