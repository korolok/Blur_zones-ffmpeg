#include <iostream>
#include <opencv4/opencv2/opencv.hpp>
#include <unistd.h>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
}


class VideoCapture {
    private:
    AVFormatContext *format_ctx = nullptr;
    AVCodecContext *codec_ctx = nullptr;

    AVCodec *codec = nullptr;

    AVFrame *frame = nullptr;
    AVPacket *packet = nullptr;

    AVOutputFormat *oformat = nullptr;
    AVFormatContext *ofctx = nullptr;
    
    AVStream *video_stream = nullptr;

    AVCodec *encode = nullptr;
    AVCodecContext *encode_ctx = nullptr;

    AVFrame *av_frame_gray = nullptr;
    AVFrame *av_frame_rgb = nullptr;

    SwsContext *_sws_ctx = nullptr;
    SwsContext *sws_ctx_rgb = nullptr;

    cv::Mat cv_frame;

    int vid_stream = -1;

    int frameCounter;

    int init_decode();
    
    int init_encoder();

    int write_frame();

    void setup_sws();

    public:
    int start();
    int stop();
};