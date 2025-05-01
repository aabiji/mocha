#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
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

// TODO:
// - merge Silencer
// - maybe add some sort of progress message when generating keypoints.dat
// - read keypoints.dat file and visualize in the engine

#include "movenet.h"

class Silencer
{
public:
    void disableStdout()
    {
        savedStdout = dup(STDOUT_FILENO);
        freopen("/dev/null", "w", stdout);
    }

    void enableStdout()
    {
        dup2(savedStdout, STDOUT_FILENO);
        close(savedStdout);
    }
private:
    int savedStdout;
};

int main(int argc, const char** argv)
{
    assert(argc >= 2 && "Must provide a path to a video file as path");
    const char* path = argv[1];

    // open the video decoder
    AVFormatContext* formatContext = nullptr;
    int res = avformat_open_input(&formatContext, path, nullptr, nullptr);
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
    int targetSize = 192;
    AVFrame* scaledFrame = av_frame_alloc();
    scaledFrame->format = AV_PIX_FMT_RGB24;
    scaledFrame->width = targetSize;
    scaledFrame->height = targetSize;

    std::vector<unsigned char> pixelBuffer(targetSize * targetSize * 3);

    // the output file that'll hold the keypoints for each frame of the video
    std::ofstream outputFile("../keypoints.dat", std::ios::binary);
    assert(outputFile.good() && "Couldn't open the output file");

    MoveNet model;
    model.init("../assets/movenet_singlepose.tflite");

    Silencer silencer;

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
            silencer.disableStdout();
            auto keypoints = model.runInference(pixelBuffer.data());
            silencer.enableStdout();
            outputFile.write(
                reinterpret_cast<const char*>(keypoints.data()),
                keypoints.size() * sizeof(Keypoint)
            );

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
