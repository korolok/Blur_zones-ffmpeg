#include <iostream>
#include <opencv4/opencv2/opencv.hpp>
#include <unistd.h>
#include <fstream>
#include "json.hpp"

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
    AVFrame *temp_frame = nullptr;

    SwsContext *sws_ctx = nullptr;
    SwsContext *sws_ctx_rgb = nullptr;

    cv::Mat cv_frame;

    std::string video_path;
    std::string cam_url;
    std::string grid;
    int *grid_pos;
    int grid_y;
    int grid_x;
    int kernel;

    int vid_stream = -1;
    int frameCounter;

    int init_decode();
    
    int init_encoder();

    int write_frame();

    void setup_sws();

    void blurPieces();
    int* gen_grid();
    std::vector<std::string> split(const std::string s, char delim);

    void parse_config(int argc, char **argv);

    public:
    void start(int argc, char** argv);
    void stop();
};