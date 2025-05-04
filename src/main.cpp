#include <assert.h>
#include <stdlib.h>
#include <fstream>

extern "C" {
#include <libavcodec/codec_par.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include <opencv2/opencv.hpp>

#include "blazepose.h"

void processVideo(BlazePose& model, const char* videoPath, std::string outputPath)
{
    // open the video decoder
    AVFormatContext* formatContext = nullptr;
    int res = avformat_open_input(&formatContext, videoPath, nullptr, nullptr);
    assert(res >= 0 && "Couldn't read the provided file");

    const AVCodec* codec;
    int videoStreamIndex =
        av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    assert(videoStreamIndex >= 0 && "Couldn't find a video stream");

    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(
        codecContext,
        formatContext->streams[videoStreamIndex]->codecpar
    );
    res = avcodec_open2(codecContext, codec, nullptr);
    assert(res >= 0 && "Couldn't open the video decoder");

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    SwsContext* scalingContext = nullptr;
    bool setupScalingContext = false;

    // the output frame and pixels
    int targetSize = 256;
    AVFrame* scaledFrame = av_frame_alloc();
    scaledFrame->format = AV_PIX_FMT_RGBAF32;
    scaledFrame->width = targetSize;
    scaledFrame->height = targetSize;

    std::vector<float> pixelBuffer(targetSize * targetSize * 3);

    // the output file that'll hold the keypoints for each frame of the video
    std::ofstream outputFile(outputPath, std::ios::binary);
    assert(outputFile.good() && "Couldn't open the output file");

    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index != videoStreamIndex) continue;

        int res = avcodec_send_packet(codecContext, packet);
        assert(res >= 0 && "Error sending video packet for decoding");

        while (res >= 0) {
            res = avcodec_receive_frame(codecContext, frame);
            if (res == AVERROR(EAGAIN) || res == AVERROR_EOF)
                break;
            assert(res >= 0 && "Error while decoding frame");

            // We can only be sure of the input width, height and
            // pixel format when we've read at least 1 frame
            if (!setupScalingContext) {
                scalingContext = sws_getContext(
                    codecContext->width, codecContext->height, codecContext->pix_fmt,
                    scaledFrame->width, scaledFrame->height,
                    (AVPixelFormat)scaledFrame->format,
                    SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
                );
                assert(scalingContext != nullptr && "Couldn't create the scaling context");
                setupScalingContext = true;
            }

            res = sws_scale_frame(scalingContext, scaledFrame, frame);
            assert(res >= 0 && "Couldn't resize the frame");

            // copy the scaled frame's pixels
            int stride = scaledFrame->linesize[0];
            for (int y = 0; y < targetSize; y++) {
                memcpy(
                    &pixelBuffer[y * targetSize * 3],
                    scaledFrame->data[0] + y * stride,
                    targetSize * 3
                );
            }

            // run inference with the frame and write the outputs to the output file
            Pose pose = model.runInference(pixelBuffer.data(), pixelBuffer.size());
            outputFile.write(reinterpret_cast<char*>(&pose), sizeof(pose));

            av_frame_unref(frame);
        }

        av_packet_unref(packet);
    }

    outputFile.close();
    av_frame_free(&frame);
    av_frame_free(&scaledFrame);
    avcodec_free_context(&codecContext);
    avformat_free_context(formatContext);
    sws_freeContext(scalingContext);
}

void drawSkeleton(
    cv::Mat& image,
    Keypoint* keypoints,
    float xScale, float yScale
) {
    cv::Scalar color = {0, 255, 0};

    for (int i = 0; i < 33; i++) {
        Keypoint& kp = keypoints[i];
        if (!kp.detected()) continue;
        cv::Point xy = cv::Point(kp.x * 256.0 * xScale, kp.y * 256.0 * yScale);
        cv::circle(image, xy, 3, color);
    }

    for (auto p : keypointConnections) {
        cv::Point a = cv::Point(
            keypoints[p.first].x * 256.0 * xScale,
            keypoints[p.first].y * 256.0 * yScale
        );
        cv::Point b = cv::Point(
            keypoints[p.second].x * 256.0 * xScale,
            keypoints[p.second].y * 256.0 * yScale
        );
        if (keypoints[p.first].detected() &&
            keypoints[p.second].detected())
            cv::line(image, a, b, color, 2, cv::LINE_AA);
    }
}

void processWebcam(BlazePose& model)
{
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

        cv::resize(frame, scaled, cv::Size(256, 256));
        Pose pose = model.runInference((float*)scaled.data, scaled.total() * scaled.channels());

        // Horizontal and vertical scaling factors
        float xScale = float(frame.size[1]) / 256.0;
        float yScale = float(frame.size[0]) / 256.0;
        drawSkeleton(frame, pose.keypoints, xScale, yScale);

        cv::imshow(name, frame);
        meter.stop();

        std::cout << meter.getFPS() << "\n";
        if (cv::waitKey(meter.getFPS()) == 27)
            break; // press escape to quit
    }
}

void outputHelp()
{

    std::cout << "Usage:\n";
    std::cout << "\t--video <PATH>\tRun pose estimation on each frame of the video and";
    std::cout << " write the keypoints to a file\n";
    std::cout << "\t--webcam\tDemo pose estimation with webcam footage\n";
}

int main(int argc, const char** argv)
{
    if (argc < 2) {
        outputHelp();
        return -1;
    }

    BlazePose model("../assets/blazepose_lite.onnx");
    std::string flag = argv[1];

    if (flag == "--video")
        processVideo(model, argv[2], "../keypoints.data");
    else if (flag == "--webcam")
        processWebcam(model);
    else {
        outputHelp();
        return -1;
    }
}