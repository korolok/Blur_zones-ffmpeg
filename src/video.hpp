#define VIDEOCAPTURE_API __declspec(dllexport)

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <math.h>
#include <string.h>
#include <algorithm>
#include <string> 

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/avfft.h>

#include <libavdevice/avdevice.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <libavformat/avformat.h>
#include <libavformat/avio.h>

    // libav resample

#include <libavutil/opt.h>
#include <libavutil/common.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/file.h>


    // hwaccel
#include "libavcodec/vdpau.h"
#include "libavutil/hwcontext.h"
#include "libavutil/hwcontext_vdpau.h"

    // lib swresample

#include <libswscale/swscale.h>

    class VideoCapture {
    public:

        VideoCapture() {
            oformat = NULL;
            ofctx = NULL;
            videoStream = NULL;
            // videoFrame = NULL;
            swsCtx = NULL;
            frameCounter = 0;

            // Initialize libavcodec
            av_register_all();
        }

        ~VideoCapture() {
            Free();
        }

        void Init(int width, int height, int fpsrate, int bitrate);

        void AddFrame(AVFrame *frame);

        void Finish();

    private:

        bool flag = false;

        AVOutputFormat *oformat;
        AVFormatContext *ofctx;

        AVStream *videoStream;
        // AVFrame *videoFrame;

        AVCodec *codec;
        AVCodecContext *cctx;

        SwsContext *swsCtx;

        int frameCounter;

        int fps;

        void Free();

        void Remux();
    };
}