#include "blur.hpp"

int main(int argc, char **argv)
{
    auto blur = std::make_shared<VideoCapture>();

    blur -> start(argc, argv);
    blur -> stop();

    return 0;
}


void VideoCapture::start(int argc, char** argv)
{
    parse_config(argc, argv);

    init_decode();

    setup_sws();

    init_encoder();

    grid_pos = gen_grid();

    packet = av_packet_alloc();

    frame = av_frame_alloc();

    while (av_read_frame(format_ctx, packet) == 0)
    {
        avcodec_send_packet(codec_ctx, packet);
        avcodec_receive_frame(codec_ctx, frame);

        if (frame->pkt_size > 0)
        {
            sws_scale(sws_ctx,
                      frame->data,
                      frame->linesize,
                      0,
                      frame->height,
                      av_frame_gray->data,
                      av_frame_gray->linesize);
        
            cv_frame = cv::Mat(av_frame_gray->height, av_frame_gray->width, CV_8UC3, av_frame_gray->data[0]);

            blurPieces();

            av_image_fill_arrays(temp_frame->data, temp_frame->linesize, cv_frame.data, AV_PIX_FMT_BGR24, temp_frame->width, temp_frame->height, 1);

            sws_scale(sws_ctx_rgb,
                      temp_frame->data,
                      temp_frame->linesize,
                      0,
                      temp_frame->height,
                      av_frame_rgb->data,
                      av_frame_rgb->linesize);

            cv::resize(cv_frame, cv_frame, cv::Size(800, 600));
            cv::imshow("Window", cv_frame);
        }

        av_frame_rgb->pts = frameCounter++;

        std::cout << "send: " << avcodec_send_frame(encode_ctx, av_frame_rgb) << std::endl;
        std::cout << "receive: " << avcodec_receive_packet(encode_ctx, packet) << std::endl;
        std::cout << "write frame: " << write_frame() << std::endl;
        std::cout << std::endl;

        av_frame_unref(frame); 
        av_packet_unref(packet);

        if (cv::waitKey(1) >= 0)
        {
            break;
        }
    }

    av_write_trailer(ofctx);
}


int VideoCapture::init_decode()
{
    avformat_open_input(&format_ctx, cam_url.c_str(), nullptr, nullptr);

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

    avformat_alloc_output_context2(&ofctx, NULL, NULL, video_path.c_str());
    if (!ofctx) 
    {
        std::cout << "Couldnt open file in format." << std::endl;
        avformat_alloc_output_context2(&ofctx, NULL, "mpeg", video_path.c_str());
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

    ret = avcodec_open2(encode_ctx, encode, NULL);
    if (ret < 0) 
    {
        std::cout << "Could not open video codec: " << av_err2str(ret) << std::endl;
        return 0;
    }

    ret = avcodec_parameters_from_context(video_stream->codecpar, encode_ctx);
    if (ret < 0) 
    {
        std::cout << "Could not copy the stream parameters" << std::endl;
        return 0;
    }

    av_dump_format(ofctx, 0, video_path.c_str(), 1);

    if (!(oformat->flags & AVFMT_NOFILE)) 
    {
        ret = avio_open(&ofctx->pb, video_path.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) 
        {
            std::cout << "Could not open " << video_path << " : " << av_err2str(ret) << std::endl;
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

    sws_ctx = sws_getContext(_width,
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

    temp_frame = av_frame_alloc();
    temp_frame->format = AV_PIX_FMT_BGR24;
    temp_frame->width = _width;
    temp_frame->height = _height;
    av_image_alloc(temp_frame->data, temp_frame->linesize, temp_frame->width, temp_frame->height, codec_ctx->pix_fmt, 1);

    av_frame_rgb = av_frame_alloc();
    av_frame_rgb->format = codec_ctx->pix_fmt;
    av_frame_rgb->width = _width;
    av_frame_rgb->height = _height;
    av_image_alloc(av_frame_rgb->data, av_frame_rgb->linesize, av_frame_rgb->width, av_frame_rgb->height, codec_ctx->pix_fmt, 1);
}

void VideoCapture::blurPieces()
{
    int img_width = cv_frame.cols;
    int img_height = cv_frame.rows;

    int block_width;
    int block_height = img_height / grid_x;

    int x_offset = 0;
    int y_offset = 0;

    int x_coord = 0;
    int y_coord = 0;

    while (y_offset < img_height)
    {
        block_width = img_width / grid_y;
        while (x_offset < img_width)
        {
            if (img_width - block_width * 2 < x_offset)
            {
                block_width += img_width - x_offset - block_width;
            }

            if (grid_pos[y_coord * grid_y + x_coord])
            {
                cv::blur(cv_frame(cv::Rect(x_offset, y_offset, block_width, block_height)), cv_frame(cv::Rect(x_offset, y_offset, block_width, block_height)), cv::Size(kernel, kernel));
            }

            x_offset += block_width;

            if (img_width - block_width < x_offset)
            {
                x_offset += img_width - x_offset;
            }
            x_coord++;
        }

        x_coord = 0;
        x_offset = 0;
        y_offset += block_height;

        if (img_height - block_height * 2 < y_offset)
        {
            block_height += img_height - y_offset - block_height;
        }

        y_coord++;
    }
}

int* VideoCapture::gen_grid()
{
    int *grid_positions = (int *)calloc(grid_y * grid_x, __SIZEOF_INT__);
    int cnt = 1;
    int x, y;

    std::vector<std::string> cells = split(grid, ','); 
    for (auto cell: cells)
    {
        std::vector<std::string> coords = split(cell, ':');
        x = std::stoi(coords[1]);
        y = std::stoi(coords[0]);
        grid_positions[x * grid_y + y] = 1;
    }

    return grid_positions;
}

std::vector<std::string> VideoCapture::split(const std::string s, char delim) 
{
    std::vector<std::string> elems;
    std::stringstream ss;
    ss.str(s);
    std::string item;

    while (std::getline(ss, item, delim)) 
    {
        elems.push_back(item);
    }

    return elems;
}

void VideoCapture::parse_config(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "input config file path" << std::endl;
        exit(1);
    }
    std::fstream file(argv[1]);

    std::string test = std::string((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
    
    nlohmann::json conf = nlohmann::json::parse(test);

    cam_url = std::string(conf["cam_url"]);

    grid_y = std::stoi(std::string(conf["grid_y"]).c_str());
    grid_x = std::stoi(std::string(conf["grid_x"]).c_str());

    grid = std::string(conf["grid"]);

    kernel = std::stoi(std::string(conf["kernel"]).c_str());

    video_path = std::string(conf["video_path"]);
}

void VideoCapture::stop()
{
    if (codec_ctx)
    {
        avcodec_free_context(&codec_ctx);
        codec_ctx = NULL;
    }

    if (encode_ctx)
    {
        avcodec_free_context(&encode_ctx);
        encode_ctx = NULL;
    }

    if (format_ctx)
    {
        avformat_close_input(&format_ctx);
        format_ctx = NULL;
    }

    if (oformat)
    {
        avformat_close_input(&ofctx);
        ofctx = NULL;
    }

    if (frame)
    {
        av_frame_free(&frame);
        frame = NULL;
    }

    if (av_frame_gray)
    {
        av_freep(&av_frame_gray[0]);
        av_frame_free(&av_frame_gray);
        av_frame_gray = NULL;
    }

    if (av_frame_rgb)
    {
        av_freep(&av_frame_rgb[0]);
        av_frame_free(&av_frame_rgb);
        av_frame_rgb = NULL;
    }

    if (temp_frame)
    {
        av_freep(&temp_frame[0]);
        av_frame_free(&temp_frame);
        temp_frame = NULL;
    } 

    if (sws_ctx)
    {
        sws_freeContext(sws_ctx);
        sws_ctx = NULL;
    }

    if (sws_ctx_rgb)
    {
        sws_freeContext(sws_ctx_rgb);
        sws_ctx_rgb = NULL;
    }

    if (packet->size)
    {
        av_packet_unref(packet);
        packet->size = 0;
    }
}