// #ifndef VIDEO_CAPTURE2_H
// #define VIDEO_CAPTURE2_H

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "../include/glad/glad.h" 
// #include "finite_math.hpp"
// #include <stdexcept>
// extern "C" {
//     #include <libavcodec/avcodec.h>
//     #include <libswscale/swscale.h>
//     #include <libavutil/opt.h>
//     #include <libavutil/imgutils.h>
//     #include <libavformat/avformat.h>
//     #include <libavutil/opt.h>
//     #include <libavutil/mathematics.h>
//     #include <libavutil/timestamp.h>
// }


// // These exist to patch three functions for which gcc gets compiler errors
// #ifdef av_err2str
// #undef av_err2str
// #include <string>
// av_always_inline std::string av_err2string(int errnum) {
//     char str[AV_ERROR_MAX_STRING_SIZE];
//     return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
// }
// #define av_err2str(err) av_err2string(err).c_str()
// #endif


// #ifdef av_ts2str
// #undef av_ts2str
// #include <string>
// av_always_inline std::string av_ts2string(int ts) {
//     char str[AV_TS_MAX_STRING_SIZE];
//     return av_ts_make_string(str, ts);
// }
// #define av_ts2str(ts) av_ts2string(ts).c_str()
// #endif

// #ifdef av_ts2timestr
// #undef av_ts2timestr
// #include <string>
// av_always_inline std::string av_ts2timestring(int ts, AVRational *tb) {
//     char str[AV_TS_MAX_STRING_SIZE];
//     return av_ts_make_time_string(str, ts, tb);
// }
// #define av_ts2timestr(ts, tb) av_ts2timestring(ts, tb).c_str()
// #endif


// class VideoCapture2
// {
// public:

//     VideoCapture2(const char *filename, unsigned int width, unsigned int height, int framerate, unsigned int bitrate){

//         avformat_alloc_output_context2(&avFormatContext, NULL, NULL, filename);
//         if (!avFormatContext) {
//             printf("Could not deduce output format from file extension: using MPEG.\n");
//             avformat_alloc_output_context2(&avFormatContext, NULL, "mpeg", filename);
//         }
//         if (!avFormatContext)
//             exit(1);

//         avOutputFormat = avFormatContext->oformat;

//         // Video Stream

//         /* find the mpeg1video encoder */

//         /* find the encoder */
//         AVCodecID codec_id = AV_CODEC_ID_H264;
//         codec = avcodec_find_encoder(codec_id);
//         if (!codec) {
//             fprintf(stderr, "Could not find encoder for '%s'\n",
//                     avcodec_get_name(codec_id));
//             exit(1);
//         }

//         pkt = av_packet_alloc();
//         if (!pkt) {
//             fprintf(stderr, "Could not allocate AVPacket\n");
//             exit(1);
//         }

//         avStream = avformat_new_stream(avFormatContext, NULL);
//         if (!avStream) {
//             fprintf(stderr, "Could not allocate stream\n");
//             exit(1);
//         }
//         avStream->id = avFormatContext->nb_streams-1;
//         codec_ctx = avcodec_alloc_context3(codec);
//         if (!codec_ctx) {
//             fprintf(stderr, "Could not alloc an encoding context\n");
//             exit(1);
//         }

//         codec_ctx->codec_id = codec_id;
//         /* put sample parameters */
//         codec_ctx->bit_rate = bitrate;
//         /* resolution must be a multiple of two */
//         if(width % 2 != 0)
//             throw std::invalid_argument( "The width must be devisible by two" );

//         if(height % 2 != 0)
//             throw std::invalid_argument( "The height must be devisible by two" );

//         codec_ctx->width = width;
//         codec_ctx->height = height;
//         /* frames per second */
//         codec_ctx->framerate = (AVRational){framerate, 1};

//         /* timebase: This is the fundamental unit of time (in seconds) in terms
//         * of which frame timestamps are represented. For fixed-fps content,
//         * timebase should be 1/framerate and timestamp increments should be
//         * identical to 1. */
//         avStream->time_base = (AVRational){ 1, framerate };
//         codec_ctx->time_base       = avStream->time_base;

//         codec_ctx->gop_size      = 10; /* emit one intra frame every twelve frames at most */
//         codec_ctx->pix_fmt       = AV_PIX_FMT_YUV420P;

//         /* Some formats want stream headers to be separate. */
//         if (avOutputFormat->flags & AVFMT_GLOBALHEADER)
//             codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

//         /* open the codec */
//         AVDictionary *opt = NULL;
//         av_dict_copy(&opt, avDict, 0);
//         ret = avcodec_open2(codec_ctx, codec, &opt);
//         av_dict_free(&opt);
//         if (ret < 0) {
//             fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
//             exit(1);
//         }


//         frame = alloc_frame(codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height);
//         if (!frame) {
//             fprintf(stderr, "Could not allocate video frame\n");
//             exit(1);
//         }

//         /* copy the stream parameters to the muxer */
//         ret = avcodec_parameters_from_context(avStream->codecpar, codec_ctx);
//         if (ret < 0) {
//             fprintf(stderr, "Could not copy the stream parameters\n");
//             exit(1);
//         }

//         // Color fromat COnversion

//         sws = sws_getContext( codec_ctx->width
//                             , codec_ctx->height
//                             , AV_PIX_FMT_RGB32
//                             , codec_ctx->width
//                             , codec_ctx->height
//                             , AV_PIX_FMT_YUV420P
//                             , SWS_FAST_BILINEAR // Change this???
//                             , 0, 0, 0);

        
//         // Check output file
//         av_dump_format(avFormatContext, 0, filename, 1);

//         /* open the output file, if needed */
//         if (!(avOutputFormat->flags & AVFMT_NOFILE)) {
//             ret = avio_open(&avFormatContext->pb, filename, AVIO_FLAG_WRITE);
//             if (ret < 0) {
//                 fprintf(stderr, "Could not open '%s': %s\n", filename,
//                         av_err2str(ret));
//                 exit(1);
//             }
//         }
//         /* Write the stream header, if any. */
//         ret = avformat_write_header(avFormatContext, &avDict);
//         if (ret < 0) {
//             fprintf(stderr, "Error occurred when opening output file: %s\n",
//                     av_err2str(ret));
//             exit(1);
//         }

//     }



//     void addFrame(){
//         fflush(stdout);

//         /* Make sure the frame data is writable.
//            On the first round, the frame is fresh from av_frame_get_buffer()
//            and therefore we know it is writable.
//            But on the next rounds, encode() will have called
//            avcodec_send_frame(), and the codec may have kept a reference to
//            the frame in its internal structures, that makes the frame
//            unwritable.
//            av_frame_make_writable() checks that and allocates a new buffer
//            for the frame only if necessary.
//          */
//         ret = av_frame_make_writable(frame);
//         if (ret < 0){
//             fprintf(stderr, "Could not make the frame writable\n");
//             exit(1); // Wait... you should throw error instead!
//         }

//         size_t nvals = 4 * codec_ctx->width * codec_ctx->height; //GL_BGRA
//         pixels = (GLubyte *) realloc(pixels, nvals * sizeof(GLubyte)); // I don't think I need to do this every time since the size is constant
//         glReadPixels(0, 0, codec_ctx->width, codec_ctx->height, GL_BGRA, GL_UNSIGNED_BYTE, pixels);

//         // CONVERT TO YUV AND ENCODE
//         ret =  av_image_alloc(frame->data, frame->linesize, codec_ctx->width, codec_ctx->height, AV_PIX_FMT_YUV420P, 32);
//         if (ret < 0){
//             fprintf(stderr, "Could not allocate the image\n");
//             exit(1); // Wait... you should throw error instead!
//         }

//         // Compensate for OpenGL y-axis pointing upwards and ffmpeg y-axis pointing downwards        
//         uint8_t *in_data[1] = {(uint8_t *) pixels + (codec_ctx->height-1)*codec_ctx->width*4}; // address of the last line
//         int in_linesize[1] = {- codec_ctx->width * 4}; // negative stride

//         sws_scale(sws, in_data, in_linesize, 0, codec_ctx->height, frame->data, frame->linesize);

//         frame->pts = frame_order;
//         frame_order++;

//         /* encode the image */
//         write_frame(avFormatContext, codec_ctx, avStream, frame, pkt);
//     }




//     void close()
//     {
//         write_frame(avFormatContext, codec_ctx, avStream, NULL, pkt);

//         av_write_trailer(avFormatContext);

//         avcodec_free_context(&codec_ctx);
//         av_frame_free(&frame);
//         sws_freeContext(sws);
//         if (!(avFormatContext->oformat->flags & AVFMT_NOFILE))
//             /* Close the output file. */
//             avio_closep(&avFormatContext->pb);

//         avformat_free_context(avFormatContext);

//     }

// private:

//     AVOutputFormat *avOutputFormat;
//     AVFormatContext* avFormatContext = NULL;
//     AVStream* avStream;
//     AVDictionary *avDict = NULL; // "create" an empty dictionary

//     GLubyte *pixels = NULL;
//     struct SwsContext *sws;
//     const AVCodec *codec;
//     AVCodecContext *codec_ctx= NULL;

//     // Should be ref counted??? https://ffmpeg.org/doxygen/3.3/group__lavc__encdec.html
//     AVFrame *frame;
//     AVPacket *pkt;
   
//     //
//     int frame_order, ret;


//     int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
//                         AVStream *st, AVFrame *frame, AVPacket *pkt)
//     {
//         int ret;

//         // send the frame to the encoder
//         ret = avcodec_send_frame(c, frame);
//         if (ret < 0) {
//             fprintf(stderr, "Error sending a frame to the encoder: %s\n",
//                     av_err2str(ret));
//             exit(1);
//         }

//         while (ret >= 0) {
//             ret = avcodec_receive_packet(c, pkt);
//             if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
//                 break;
//             else if (ret < 0) {
//                 fprintf(stderr, "Error encoding a frame: %s\n", av_err2str(ret));
//                 exit(1);
//             }

//             /* rescale output packet timestamp values from codec to stream timebase */
//             av_packet_rescale_ts(pkt, c->time_base, st->time_base);
//             pkt->stream_index = st->index;

//             /* Write the compressed frame to the media file. */
//             log_packet(fmt_ctx, pkt);
//             ret = av_interleaved_write_frame(fmt_ctx, pkt);
//             /* pkt is now blank (av_interleaved_write_frame() takes ownership of
//             * its contents and resets pkt), so that no unreferencing is necessary.
//             * This would be different if one used av_write_frame(). */
//             if (ret < 0) {
//                 fprintf(stderr, "Error while writing output packet: %s\n", av_err2str(ret));
//                 exit(1);
//             }
//         }

//         return ret == AVERROR_EOF ? 1 : 0;
//     }

//     void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
//     {
//         AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

//         printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
//             av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
//             av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
//             av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
//             pkt->stream_index);
//     }

//     AVFrame *alloc_frame(enum AVPixelFormat pix_fmt, int width, int height)
//     {
//         AVFrame *frame;
//         int ret;

//         frame = av_frame_alloc();
//         if (!frame)
//             return NULL;

//         frame->format = pix_fmt;
//         frame->width  = width;
//         frame->height = height;

//         /* allocate the buffers for the frame data */
//         ret = av_frame_get_buffer(frame, 0);
//         if (ret < 0) {
//             fprintf(stderr, "Could not allocate frame data.\n");
//             exit(1);
//         }

//         return frame;
//     }


// };
// #endif

// #include <iostream>

// extern "C"
// {
//     #include <libavcodec/avcodec.h>
//     #include <libavutil/avassert.h>
//     #include <libavutil/channel_layout.h>
//     #include <libavutil/opt.h>
//     #include <libavutil/mathematics.h>
//     #include <libavutil/timestamp.h>
//     #include <libavformat/avformat.h>
//     #include <libswscale/swscale.h>
//     #include <libswresample/swresample.h>
//     #include <libavutil/imgutils.h>
// }

// using namespace std;

// // Для av_err2str
// #ifdef  __cplusplus
// static const std::string av_make_error_string(int errnum)
// {
//     char errbuf[AV_ERROR_MAX_STRING_SIZE];
//     av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
//     return (std::string)errbuf;
// }

// #undef av_err2str
// #define av_err2str(errnum) av_make_error_string(errnum).c_str()
// #endif // __cplusplus

// #define STREAM_DURATION   10.0
// #define STREAM_FRAME_RATE 25 /* 25 images/s */
// #define STREAM_PIX_FMT  AV_PIX_FMT_BGRA // AV_PIX_FMT_BGRA AV_PIX_FMT_RGBA AV_PIX_FMT_ARGB
// #define CODEC_PIX_FMT  AV_PIX_FMT_YUV420P //AV_PIX_FMT_YUV420P AV_PIX_FMT_NV12 AV_PIX_FMT_YUVJ420P  AV_PIX_FMT_BGR24

// #define FRAME_WIDTH 1440
// #define FRAME_HEIGHT 900

// #define SCALE_FLAGS SWS_BICUBIC

// AVCodecContext *cc = NULL;
// AVFormatContext *oc = NULL;
// AVOutputFormat *fmt = NULL;
// AVDictionary *opt = NULL;

// AVCodec *video_codec = NULL;
// AVStream *video_stream = NULL;

// AVFrame *video_frame = NULL;

// int64_t next_pts = 0;

// SwsContext *sws_ctx = 0;

// static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
// {
//     AVFrame *picture;
//     int ret;

//     picture = av_frame_alloc();
//     if (!picture)
//         return NULL;

//     picture->format = pix_fmt;
//     picture->width = width;
//     picture->height = height;

//     ret = av_frame_get_buffer(picture, 32);
//     if (ret < 0) {
//         fprintf(stderr, "Could not allocate frame data.\n");
//         exit(1);
//     }

//     return picture;
// }

// bool bInit = false;
// int nSimpleWidth = FRAME_WIDTH; 
// int nSimpleHeight = FRAME_HEIGHT;
// int nSimpleStride = 0;
// int nSimpleAlign = 0;
// int bSimpleUseAlign = 0;
// HWND hDesktopWnd;
// HDC hDesktopDC;
// HDC hCaptureDC;

// int cadrSize = 0;
// BITMAPINFO m_bmiSimple;
// int m_bSimpleBottomUpImg = 0;
// BYTE *m_bSimpleData = 0;

// HBITMAP hCaptureBitmap = 0;
// BOOL res = 0;

// LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
// LARGE_INTEGER Frequency;
// int startFrame = 0;
// int frameCounter = 0;
// int ofps = 40; 
// int globalStat = 0;

// static AVFrame *get_video_frame()
// {
//     if (av_compare_ts(next_pts, cc->time_base, STREAM_DURATION, AVRational{ 1, 1 }) >= 0)
//     {
//         return NULL;
//     }

//     if (av_frame_make_writable(video_frame) < 0)
//         exit(1);

//     if (!bInit)
//     {
//         /**
//     * Настройка захвата кадра.
//     **/
//     }

//     if (bInit)
//     {
//         BOOL res = BitBlt(hCaptureDC, 0, 0, nSimpleWidth, nSimpleHeight, hDesktopDC, 0, 0, SRCCOPY | CAPTUREBLT);
//         if (cc->pix_fmt != STREAM_PIX_FMT)
//         {
//             sws_scale(sws_ctx, &m_bSimpleData, &nSimpleStride, 0, cc->height, video_frame->data, video_frame->linesize);

//         }
//         else
//         {
//             memcpy_s(video_frame->data[0], cadrSize, m_bSimpleData, cadrSize);
//         }
//     }

//     if (!startFrame)
//     {
//         startFrame = 1;
//         frameCounter = 0;
//         next_pts = 0;
//     }
//     else
//     {
//         // Расчёт времени.
//         //++frameCounter;
//         frameCounter += 2; // Emulate skip frame.

//     }

//     // Ставим время кадра.
//     //ost->frame->pts = ost->next_pts++;
//     next_pts = frameCounter;
//     video_frame->pts = next_pts;
//     cout << "pts: " << video_frame->pts << endl;

//     ++globalStat;
//     return video_frame;
// }

// static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
// {
//     av_packet_rescale_ts(pkt, *time_base, st->time_base);
//     pkt->stream_index = st->index;

//     return av_interleaved_write_frame(fmt_ctx, pkt);
// }

// static int write_video_frame()
// {
//     int ret;
//     int ret1;
//     int flush = 0;
//     AVFrame *frame;
//     int got_packet = 0;
//     AVPacket pkt = { 0 };

//     frame = get_video_frame();

//     av_init_packet(&pkt);

//     ret = avcodec_send_frame(cc, frame);

//     if (ret < 0) {
//         fprintf(stderr, "Error sending a frame for video encoding\n");
//         if (ret == AVERROR(EAGAIN))
//         {
//             fprintf(stderr, "Error eagain\n");
//         }
//         else if (ret == AVERROR_EOF)
//         {
//             flush = 1;
//             fprintf(stderr, "Error EOF\n");
//         }
//         else if (ret == AVERROR(EINVAL))
//         {
//             fprintf(stderr, "Error EINVAL\n");
//         }
//         else if (ret == AVERROR(ENOMEM))
//         {
//             fprintf(stderr, "Error ENOMEM\n");
//         }

//         if (ret != AVERROR_EOF)
//         {
//             exit(1);
//         }
//     }

//     while (flush || ret >= 0) {
//         got_packet = 1;
//         ret = avcodec_receive_packet(cc, &pkt);

//         if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
//             {
//                 got_packet = 0;
//                 flush = 0;
//                 break;
//             }
//             else if (ret < 0)
//             {
//                 fprintf(stderr, "Error during encoding\n");
//                 exit(1);
//             }

//             ret1 = write_frame(oc, &cc->time_base, video_stream, &pkt);
//             if (ret1 < 0) {
//                 fprintf(stderr, "Error while writing video frame: %s\n",
//                     av_err2str(ret1));
//                 exit(1);
//             }
//     }

//     return (frame || got_packet) ? 0 : 1;
// }

// // Обработка.
// int process()
// {
//     int ret = 0;
//     const char *filename = "D:\\capture_test.mp4";

//     avformat_alloc_output_context2(&oc, NULL, NULL, filename);
//     if (!oc) 
//     {
//         cout << "Couldnt open file in format." << endl;
//         avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
//     }
//     if (!oc)
//     {
//         cout << "Couldnt open file in MPEG format." << endl;
//         return 0;
//     }

//     fmt = oc->oformat;

//     if (fmt->video_codec == AV_CODEC_ID_NONE) 
//     {
//         cout << "Video codec dont podderzjka." << endl;
//     }

//     video_codec = avcodec_find_encoder(fmt->video_codec);
//     if (!video_codec)
//     {
//         cout << "Could not find encoder for " << avcodec_get_name(fmt->video_codec) << endl;
//         return 0;
//     }

//     video_stream = avformat_new_stream(oc, NULL);
//     if (!video_stream) 
//     {
//         cout << "Could not allocate stream" << endl;
//         return 0;
//     }
//     video_stream->id = oc->nb_streams - 1;
//     video_stream->avg_frame_rate = AVRational{ 1, STREAM_FRAME_RATE };

//     cc = avcodec_alloc_context3(video_codec);
//     if (!cc) 
//     {
//         cout << "Could not alloc an encoding context" << endl;
//         return 0;
//     }

//     cc->codec_id = video_codec->id;
//     cc->bit_rate = 400000;
//     cc->width = FRAME_WIDTH;
//     cc->height = FRAME_HEIGHT;
//     cc->gop_size = 12;
//     cc->pix_fmt = CODEC_PIX_FMT;
//     if (cc->codec_id == AV_CODEC_ID_MPEG2VIDEO) 
//     {   
//         cc->max_b_frames = 2;
//     }
//     if (cc->codec_id == AV_CODEC_ID_MPEG1VIDEO) 
//     {
//         cc->mb_decision = 2;
//     }

//     video_stream->time_base = AVRational{ 1, STREAM_FRAME_RATE };
//     cc->time_base = video_stream->time_base;

//     if (fmt->flags & AVFMT_GLOBALHEADER) cc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

//     video_frame = alloc_picture(cc->pix_fmt, cc->width, cc->height);
//     if (!video_frame) 
//     {
//         cout << "Could not allocate video frame" << endl;
//         return 0;
//     }

//     // Открываем кодек.
//     ret = avcodec_open2(cc, video_codec, NULL);
//     if (ret < 0) 
//     {
//         cout << "Could not open video codec: " << av_err2str(ret) << endl;
//         return 0;
//     }

//     /* copy the stream parameters to the muxer */
//     ret = avcodec_parameters_from_context(video_stream->codecpar, cc);
//     if (ret < 0) 
//     {
//         cout << "Could not copy the stream parameters" << endl;
//         return 0;
//     }

//     av_dump_format(oc, 0, filename, 1);

//     /* open the output file, if needed */
//     if (!(fmt->flags & AVFMT_NOFILE)) 
//     {
//         ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
//         if (ret < 0) 
//         {
//             cout << "Could not open " << filename << " : " << av_err2str(ret) << endl;
//             return 0;
//         }
//     }

//     ret = avformat_write_header(oc, &opt);
//     if (ret < 0) 
//     {
//         cout << "Error occurred when opening output file: " << av_err2str(ret) << endl;
//         return 0;
//     }


//     // Пишем данные.
//     int encode_video = 1;
//     while (encode_video)
//     {
//         cout << "Thread video 1" << endl;
//         encode_video = !write_video_frame();
//     }


//     // Закрываем.
//     av_write_trailer(oc);

//     /* Close each codec. */
//     avcodec_free_context(&cc);

//     if (!(fmt->flags & AVFMT_NOFILE))
//     {
//         avio_closep(&oc->pb);
//     }

//     /* free the stream */
//     avformat_free_context(oc);


//     return 0;
// }

// int main()
// {
//     setlocale(LC_ALL, "Russian");

//     process();

//     _getch();

//     return 0;
// }