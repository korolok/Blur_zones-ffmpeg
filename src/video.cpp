#include "video.hpp"

#define VIDEO_TMP_FILE "tmp.h264"
#define FINAL_FILE_NAME "record.mp4"


using namespace std;

void VideoCapture::Init(int width, int height, int fpsrate, int bitrate) {

    fps = fpsrate;

    int err;

    if (!(oformat = av_guess_format(NULL, VIDEO_TMP_FILE, NULL))) {
        std::cout << ("Failed to define output format") << std::endl;
        return;
    }

    if ((err = avformat_alloc_output_context2(&ofctx, oformat, NULL, VIDEO_TMP_FILE) < 0)) {
        std::cout << ("Failed to allocate output context", err) << std::endl;
        Free();
        return;
    }

    if (!(codec = avcodec_find_encoder(oformat->video_codec))) {
        std::cout << ("Failed to find encoder") << std::endl;
        Free();
        return;
    }

    if (!(videoStream = avformat_new_stream(ofctx, codec))) {
        std::cout << ("Failed to create new stream") << std::endl;
        Free();
        return;
    }

    if (!(cctx = avcodec_alloc_context3(codec))) {
        std::cout << ("Failed to allocate codec context") << std::endl;
        Free();
        return;
    }

    videoStream->codecpar->codec_id = oformat->video_codec;
    videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    videoStream->codecpar->width = width;
    videoStream->codecpar->height = height;
    videoStream->codecpar->format = AV_PIX_FMT_YUV420P;
    videoStream->codecpar->bit_rate = bitrate * 1000;
    videoStream->time_base = { 1, fps };

    avcodec_parameters_to_context(cctx, videoStream->codecpar);
    cctx->time_base = { 1, fps };
    cctx->max_b_frames = 2;
    cctx->gop_size = 12;
    if (videoStream->codecpar->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(cctx, "preset", "ultrafast", 0);
    }
    if (ofctx->oformat->flags & AVFMT_GLOBALHEADER) {
        cctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    avcodec_parameters_from_context(videoStream->codecpar, cctx);

    if ((err = avcodec_open2(cctx, codec, NULL)) < 0) {
        std::cout << ("Failed to open codec", err);
        Free();
        return;
    }

    if (!(oformat->flags & AVFMT_NOFILE)) {
        if ((err = avio_open(&ofctx->pb, VIDEO_TMP_FILE, AVIO_FLAG_WRITE)) < 0) {
            std::cout << ("Failed to open file", err);
            Free();
            return;
        }
    }

    if ((err = avformat_write_header(ofctx, NULL)) < 0) {
        std::cout << ("Failed to write header", err);
        Free();
        return;
    }

    av_dump_format(ofctx, 0, VIDEO_TMP_FILE, 1);
}

void VideoCapture::AddFrame(AVFrame *videoFrame) {
    int err;
    // if (!videoFrame) {

        // videoFrame = av_frame_alloc();
        // videoFrame->format = AV_PIX_FMT_BGR24;
        // videoFrame->width = cctx->width;
        // videoFrame->height = cctx->height;
            // if (flag == false)
            // {
            // av_frame_get_buffer(videoFrame, 32);
            // flag = true;
            // } 
        // if ((err = av_frame_get_buffer(videoFrame, 32)) < 0) {
            // std::cout << ("Failed to allocate picture", err);
            // return;
        // }
    // }

    videoFrame->pts = frameCounter++;

    if ((err = avcodec_send_frame(cctx, videoFrame)) < 0) {
        std::cout << ("Failed to send frame", err);
        return;
    }

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    if (avcodec_receive_packet(cctx, &pkt) == 0) {
        pkt.flags |= AV_PKT_FLAG_KEY;
        av_interleaved_write_frame(ofctx, &pkt);
        av_packet_unref(&pkt);
    }
}

void VideoCapture::Finish() {
    //DELAYED FRAMES
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    for (;;) {
        avcodec_send_frame(cctx, NULL);
        if (avcodec_receive_packet(cctx, &pkt) == 0) {
            av_interleaved_write_frame(ofctx, &pkt);
            av_packet_unref(&pkt);
        }
        else {
            break;
        }
    }

    av_write_trailer(ofctx);
    if (!(oformat->flags & AVFMT_NOFILE)) {
        int err = avio_close(ofctx->pb);
        if (err < 0) {
            std::cout << ("Failed to close file", err);
        }
    }

    Free();

    Remux();
}

void VideoCapture::Free() {
    if (cctx) {
        avcodec_free_context(&cctx);
    }
    if (ofctx) {
        avformat_free_context(ofctx);
    }
    if (swsCtx) {
        sws_freeContext(swsCtx);
    }
}

void VideoCapture::Remux() {
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    int err;

    if ((err = avformat_open_input(&ifmt_ctx, VIDEO_TMP_FILE, 0, 0)) < 0) {
        std::cout << ("Failed to open input file for remuxing", err);
        exit(1);
    }
    if ((err = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        std::cout << ("Failed to retrieve input stream information", err);
        exit(1);
    }
    if ((err = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, FINAL_FILE_NAME))) {
        std::cout << ("Failed to allocate output context", err);
        exit(1);
    }

    AVStream *inVideoStream = ifmt_ctx->streams[0];
    AVStream *outVideoStream = avformat_new_stream(ofmt_ctx, NULL);
    if (!outVideoStream) {
        std::cout << ("Failed to allocate output video stream", 0);
        exit(1);
    }
    outVideoStream->time_base = { 1, fps };
    avcodec_parameters_copy(outVideoStream->codecpar, inVideoStream->codecpar);
    outVideoStream->codecpar->codec_tag = 0;

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if ((err = avio_open(&ofmt_ctx->pb, FINAL_FILE_NAME, AVIO_FLAG_WRITE)) < 0) {
            std::cout << ("Failed to open output file", err);
            exit(1);
        }
    }

    if ((err = avformat_write_header(ofmt_ctx, 0)) < 0) {
        std::cout << ("Failed to write header to output file", err);
        exit(1);
    }

    AVPacket videoPkt;
    int ts = 0;
    while (true) {
        if ((err = av_read_frame(ifmt_ctx, &videoPkt)) < 0) {
            break;
        }
        videoPkt.stream_index = outVideoStream->index;
        videoPkt.pts = ts;
        videoPkt.dts = ts;
        videoPkt.duration = av_rescale_q(videoPkt.duration, inVideoStream->time_base, outVideoStream->time_base);
        ts += videoPkt.duration;
        videoPkt.pos = -1;

        if ((err = av_interleaved_write_frame(ofmt_ctx, &videoPkt)) < 0) {
            std::cout << ("Failed to mux packet", err);
            av_packet_unref(&videoPkt);
            break;
        }
        av_packet_unref(&videoPkt);
    }

    av_write_trailer(ofmt_ctx);

end:
    if (ifmt_ctx) {
        avformat_close_input(&ifmt_ctx);
    }
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&ofmt_ctx->pb);
    }
    if (ofmt_ctx) {
        avformat_free_context(ofmt_ctx);
    }
}