#include "blur.hpp"

int main()
{
    auto blur = std::make_shared<VideoCapture>();

    blur -> start();
    blur -> stop();

    return 0;
}


int VideoCapture::start()
{
    init_decode();

    setup_sws();

    init_encoder();

    packet = av_packet_alloc();

    frame = av_frame_alloc();

    while (av_read_frame(format_ctx, packet) == 0)
    {
        avcodec_send_packet(codec_ctx, packet);
        avcodec_receive_frame(codec_ctx, frame);

        if (frame->pkt_size > 0)
        {
            sws_scale(_sws_ctx,
                      frame->data,
                      frame->linesize,
                      0,
                      frame->height,
                      av_frame_gray->data,
                      av_frame_gray->linesize);
        
            cv::Mat cv_frame(av_frame_gray->height, av_frame_gray->width, CV_8UC3, av_frame_gray->data[0]);

            av_image_fill_arrays(frame->data, av_frame_gray->linesize, cv_frame.data, AV_PIX_FMT_BGR24, av_frame_gray->width, av_frame_gray->height, 1);

            sws_scale(sws_ctx_rgb,
                      frame->data,
                      frame->linesize,
                      0,
                      frame->height,
                      av_frame_rgb->data,
                      av_frame_rgb->linesize);

            cv::imshow("Window", cv_frame);
        }
        
        av_packet_unref(packet);

        av_frame_rgb->pts = frameCounter++;

        std::cout << "send: " << avcodec_send_frame(encode_ctx, av_frame_rgb) << std::endl;

        std::cout << "receive: " << avcodec_receive_packet(encode_ctx, packet) << std::endl;

        write_frame();

        if (cv::waitKey(1) >= 0)
        {
            break;
        }
    }

    av_write_trailer(ofctx);

    return 0;
}


int VideoCapture::init_decode()
{
    avformat_open_input(&format_ctx, "rtsp://admin:Admin1234@10.24.72.84:554/ch01.264?dev=1", nullptr, nullptr);

    avformat_find_stream_info(format_ctx, nullptr);
    
    av_dump_format(format_ctx, 0, "", 0);

    for (int i = 0;format_ctx->nb_streams > i; i++)
    {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            vid_stream = i;
            break;
        }
    }

    codec = avcodec_find_decoder(format_ctx->streams[vid_stream]->codecpar->codec_id);

    codec_ctx = avcodec_alloc_context3(codec);

    avcodec_parameters_to_context(codec_ctx, format_ctx->streams[vid_stream]->codecpar);

    avcodec_open2(codec_ctx, codec, nullptr);

    return 0;
}

int VideoCapture::init_encoder()
{
    int ret = 0;
    const char *filename = "./test.mp4";

    avformat_alloc_output_context2(&ofctx, NULL, NULL, filename);
    if (!ofctx) 
    {
        std::cout << "Couldnt open file in format." << std::endl;
        avformat_alloc_output_context2(&ofctx, NULL, "mpeg", filename);
    }
    if (!ofctx)
    {
        std::cout << "Couldnt open file in MPEG format." << std::endl;
        return 0;
    }

    oformat = ofctx->oformat;

    if (oformat->video_codec == AV_CODEC_ID_NONE) 
    {
        std::cout << "Video codec dont supported." << std::endl;
    }

    encode = avcodec_find_encoder(oformat->video_codec);
    if (!encode)
    {
        std::cout << "Could not find encoder for " << avcodec_get_name(oformat->video_codec) << std::endl;
        return 0;
    }

    video_stream = avformat_new_stream(ofctx, NULL);
    if (!video_stream) 
    {
        std::cout << "Could not allocate stream" << std::endl;
        return 0;
    }
    video_stream->id = ofctx->nb_streams - 1;
    // video_stream->avg_frame_rate = AVRational{ 1, 25};

    encode_ctx = avcodec_alloc_context3(encode);
    if (!encode_ctx) 
    {
        std::cout << "Could not alloc an encoding context" << std::endl;
        return 0;
    }

    encode_ctx->codec_id = codec_ctx ->codec_id;
    encode_ctx->bit_rate = codec_ctx->bit_rate;
    encode_ctx->width = codec_ctx->width;
    encode_ctx->height = codec_ctx->height;
    encode_ctx->gop_size = codec_ctx->gop_size;
    encode_ctx->pix_fmt = codec_ctx->pix_fmt;

    if (encode_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) 
    {   
        encode_ctx->max_b_frames = 2;
    }
    if (encode_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO) 
    {
        encode_ctx->mb_decision = 2;
    }

    video_stream->time_base = AVRational{ 1, 25};
    encode_ctx->time_base = video_stream->time_base;

    if (oformat->flags & AVFMT_GLOBALHEADER) encode_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // Открываем кодек.
    ret = avcodec_open2(encode_ctx, encode, NULL);
    if (ret < 0) 
    {
        std::cout << "Could not open video codec: " << av_err2str(ret) << std::endl;
        return 0;
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(video_stream->codecpar, encode_ctx);
    if (ret < 0) 
    {
        std::cout << "Could not copy the stream parameters" << std::endl;
        return 0;
    }

    av_dump_format(ofctx, 0, filename, 1);

    /* open the output file, if needed */
    if (!(oformat->flags & AVFMT_NOFILE)) 
    {
        ret = avio_open(&ofctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) 
        {
            std::cout << "Could not open " << filename << " : " << av_err2str(ret) << std::endl;
            return 0;
        }
    }

    AVDictionary *opt = NULL;

    ret = avformat_write_header(ofctx, &opt);
    if (ret < 0) 
    {
        std::cout << "Error occurred when opening output file: " << av_err2str(ret) << std::endl;
        return 0;
    }
    
    return 0;
}

int VideoCapture::write_frame()
{
    av_packet_rescale_ts(packet, encode_ctx->time_base, video_stream->time_base);
    packet->stream_index = video_stream->index;

    return av_interleaved_write_frame(ofctx, packet);
}

void VideoCapture::setup_sws()
{
    int _height = codec_ctx->height;
    int _width = codec_ctx->width;

    // codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    _sws_ctx = sws_getContext(_width,
                              _height,
                              codec_ctx->pix_fmt,
                              _width,
                              _height,
                              AV_PIX_FMT_BGR24,
                              SWS_BILINEAR,
                              NULL,
                              NULL,
                              NULL);

    sws_ctx_rgb = sws_getContext(_width,
                                 _height,
                                 AV_PIX_FMT_BGR24,
                                 _width,
                                 _height,
                                 codec_ctx->pix_fmt,
                                 SWS_BILINEAR,
                                 NULL,
                                 NULL,
                                 NULL);

    av_frame_gray = av_frame_alloc();
    av_frame_gray->format = AV_PIX_FMT_BGR24;
    av_frame_gray->width = _width;
    av_frame_gray->height = _height;
    av_image_alloc(av_frame_gray->data, av_frame_gray->linesize, av_frame_gray->width, av_frame_gray->height, AV_PIX_FMT_BGR24, 1);

    av_frame_rgb = av_frame_alloc();
    av_frame_rgb->format = codec_ctx->pix_fmt;
    av_frame_rgb->width = _width;
    av_frame_rgb->height = _height;
    av_image_alloc(av_frame_rgb->data, av_frame_rgb->linesize, av_frame_rgb->width, av_frame_rgb->height, codec_ctx->pix_fmt, 1);
}

int VideoCapture::stop()
{
    return 0;
}