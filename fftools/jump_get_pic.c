/**
 * create by wanglizheng, at 2018.08.14
 * for get cur video pic
 * but cur is using cpu for work
 */

#include <string.h>
//#include <jni.h>
#include "ffmpeg.h"
#include "cmdutils.h"
#include "libavutil/pixfmt.h"
//#include "Jni_FFmpegCmd.h"
//#include "ffmpeg_thread.h"
#include "jump_get_pic.h"
//#include "qc_log.h"

static  int init_control(CONTROL_T *control)
{
    int i=0;
    if(!control){
        return -1;
    }
  
    memset(control, 0, sizeof(CONTROL_T));
    control->fmt_ctx = NULL;
    control->video_dec_ctx = NULL;
    control->width = 0;
    control->height = 0;
    control->pix_fmt = -1;
    control->video_stream = NULL;
    control->src_filename = NULL;
    control->video_dst_filename = NULL;
    control->video_dst_file = NULL;
    for(i=0;i<4;i++){
        control->video_dst_data[i] = NULL;
    }
    for(i=0;i<4;i++){
        control->video_dst_linesize[i] = 0;
    }
    control->video_dst_bufsize = 0;
    control->video_stream_idx = -1;
    control->frame = NULL;
    memset(&control->pkt, 0, sizeof(AVPacket));
    control->video_frame_count = 0;
        //set default ref
    control->refcount = 0;
        //qc get frame flag
    control->got_seek_frame = 0;
    control->env = NULL;
    return 0;
}

static void uinit_control(CONTROL_T *control)
{
    init_control(control);
}

static int bmp_get(CONTROL_T *control, int cached){
#if 0
    struct SwsContext *img_convert_ctx = NULL;
    uint8_t *pic = NULL;
    int size = 0;
    AVFrame *frameRGB = NULL;
    AVFrame *frame = control->frame;
    AVCodecContext *codecCtx = control->video_dec_ctx;

    jobject picBuffer;
    jmethodID bitmap_copy_pixels;
  
    jclass clz = (*control->env)->FindClass(control->env, "android/graphics/Bitmap");
    jclass bitmap_class = (*control->env)->NewGlobalRef(control->env, clz);
    jmethodID create_bitmap_method = (*control->env)->GetStaticMethodID(control->env, bitmap_class, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    clz = (*control->env)->FindClass(control->env, "android/graphics/Bitmap$Config");
    jfieldID rgb565FieldID = (*control->env)->GetStaticFieldID(control->env, clz, "RGB_565", "Landroid/graphics/Bitmap$Config;");
    jobject rgb565_config = (*control->env)->NewGlobalRef(control->env, (*control->env)->GetStaticObjectField(control->env, clz, rgb565FieldID));
  
    control->bmp = (*control->env)->CallStaticObjectMethod(control->env, bitmap_class, create_bitmap_method, codecCtx->width, codecCtx->height, rgb565_config);
        //  size = avpicture_get_size(PIX_FMT_RGB565, codecCtx->width, codecCtx->height);
    size = avpicture_get_size(AV_PIX_FMT_RGB565, codecCtx->width, codecCtx->height);
    pic = (uint8_t *)av_malloc(size * sizeof(uint8_t));
    if(!pic){
        return -1;
    }

#define PIX_FMT_RGB565 AV_PIX_FMT_RGB565
        //img_convert_ctx = sws_getCachedContext(img_convert_ctx, codecCtx->width, codecCtx->height, codecCtx->pix_fmt, codecCtx->width, codecCtx->height, PIX_FMT_RGB565, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    img_convert_ctx = sws_getCachedContext(img_convert_ctx, codecCtx->width, codecCtx->height, codecCtx->pix_fmt, codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB565, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    LOGI("SwsScale %x", codecCtx->pix_fmt);
    if (img_convert_ctx) {
            //frameRGB = avcodec_alloc_frame();
        frameRGB = av_frame_alloc();
    
            //avpicture_fill((AVPicture *)frameRGB, pic, PIX_FMT_RGB565, codecCtx->width, codecCtx->height);
        avpicture_fill((AVPicture *)frameRGB, pic, AV_PIX_FMT_RGB565, codecCtx->width, codecCtx->height);
        sws_scale(img_convert_ctx, (const uint8_t * const*)frame->data, frame->linesize, 0, codecCtx->height, frameRGB->data, frameRGB->linesize);
    }
    bitmap_copy_pixels = (*control->env)->GetMethodID(control->env, bitmap_class, "copyPixelsFromBuffer", "(Ljava/nio/Buffer;)V");
    picBuffer = (*control->env)->NewDirectByteBuffer(control->env, pic, size);
    (*control->env)->CallVoidMethod(control->env, control->bmp, bitmap_copy_pixels, picBuffer);
    (*control->env)->DeleteLocalRef(control->env, picBuffer);
#endif
    return 0;
}

static int yuv_image_save(CONTROL_T *control, int cached)
{
    long usec = -1;
      
    if (control->frame->width != control->width || control->frame->height != control->height ||
        control->frame->format != control->pix_fmt) {
            /* To handle this change, one could call av_image_alloc again and
             * decode the following frames into another rawvideo file. */
        av_log(NULL, AV_LOG_ERROR, "Error decoding, frame is not correct\n");
        return -1;
    }

    av_log(NULL, AV_LOG_ERROR, "video_frame%s n:%d coded_n:%d\n",
           cached ? "(cached)" : "",
           control->video_frame_count++, control->frame->coded_picture_number);

        /* copy decoded frame to destination buffer:
         * this is required since rawvideo expects non aligned data */
    usec = av_q2d(control->video_stream->time_base)*control->frame->pts *1000 * 1000;
    if(usec >= control->pic_usec)
    {
        av_image_copy(control->video_dst_data, control->video_dst_linesize,
                      (const uint8_t **)(control->frame->data), control->frame->linesize,
                      control->pix_fmt, control->width, control->height);
		
            /* write to rawvideo file */
        fwrite(control->video_dst_data[0], 1, control->video_dst_bufsize, control->video_dst_file);
        control->got_seek_frame = 1;
    }

    return 0;
}

static void avframe_save(CONTROL_T *control)
{
    int i=0;
    AVFrame *avframe = control->frame;
    FILE *fdump = fopen(control->video_dst_filename, "wb");
    av_log(NULL, AV_LOG_DEBUG, "qc, save avframe into %s ", control->video_dst_filename);
    uint32_t pitchY = avframe->linesize[0];
    uint32_t pitchU = avframe->linesize[1];
    uint32_t pitchV = avframe->linesize[2];

    uint8_t *avY = avframe->data[0];
    uint8_t *avU = avframe->data[1];
    uint8_t *avV = avframe->data[2];

    for (i = 0; i < avframe->height; i++) {
        fwrite(avY, avframe->width, 1, fdump);
        avY += pitchY;
    }
    for (i = 0; i < avframe->height/2; i++) {
        fwrite(avU, avframe->width/2, 1, fdump);
        avU += pitchU;
    }
    for (i = 0; i < avframe->height/2; i++) {
        fwrite(avV, avframe->width/2, 1, fdump);
        avV += pitchV;
    }
    
    fclose(fdump);
}

static int jpg_save(const char *dst, AVFrame* pFrame, int width, int height, char *extra)
{
    int ret =-1;
    AVPacket pkt;
    char *out_file = dst;
    AVFormatContext* pFormatCtx = avformat_alloc_context();

    pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);

    if( avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
        printf("Couldn't open output file.");
        return -1;
    }

    AVStream* pAVStream = avformat_new_stream(pFormatCtx, 0);
    if( pAVStream == NULL ) {
        return -1;
    }
    
    AVCodecContext* pCodecCtx = pAVStream->codec;
    
    pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    pCodecCtx->width = width;
    pCodecCtx->height = height;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
    
    av_dump_format(pFormatCtx, 0, out_file, 1);

    AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if( !pCodec ) {
        printf("Codec not found.");
        return -1;
    }

    if( avcodec_open2(pCodecCtx, pCodec, NULL) < 0 ) {
        printf("Could not open codec.");
        return -1;
    }
    
    ret = avformat_write_header(pFormatCtx, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "err while write jpg header");
    }
    
    int y_size = pCodecCtx->width * pCodecCtx->height;

    av_new_packet(&pkt, y_size * 3);
    
    int got_picture = 0;
    ret = decode(pCodecCtx, &pkt, pFrame, &got_picture);
    if( ret < 0 ) {
        printf("Encode Error.\n");
        return -1;
    }
    if( got_picture == 1 ) {
            //pkt.stream_index = pAVStream->index;
        ret = av_write_frame(pFormatCtx, &pkt);
    }
 
    av_free_packet(&pkt);
    av_write_trailer(pFormatCtx);
    if( pAVStream ) {
        avcodec_close(pAVStream->codec);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
    
    return 0;
}

static int yuv_image_pass(CONTROL_T *control, int cached){
    if(control->video_dst_filename){
            //avframe_save(control);
        jpg_save(control->video_dst_filename, control->frame, control->width, control->height, NULL);
    }else{
        bmp_get(control, cached);
    }
    return 0;
}

static int decode_packet(CONTROL_T *control, int *got_frame, int cached)
{
    int ret = 0;
    int decoded = control->pkt.size;
    *got_frame = 0;
    long usec = -1;
    
    if (control->pkt.stream_index == control->video_stream_idx) {
            /* decode video frame */
#if 0
        ret = avcodec_decode_video2(control->video_dec_ctx, control->frame, got_frame, &control->pkt);
#else
        ret = decode(control->video_dec_ctx, control->frame, got_frame, &control->pkt);
#endif
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error decoding video frame (%s)\n", av_err2str(ret));
            return ret;
        }
	
        if (*got_frame) {
            usec = av_q2d(control->video_stream->time_base)*control->frame->pts *1000 * 1000;
            if(usec >= control->pic_usec)
            {
                yuv_image_pass(control, cached);
                control->got_seek_frame = 1;
            }
        }
    }

        /* If we use control->frame reference counting, we own the data and need
         * to de-reference it when we don't use it anymore */
    if (*got_frame && control->refcount)
        av_frame_unref(control->frame);

    return decoded;
}

static int open_codec_context(CONTROL_T *control, int *stream_idx,
                       AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find %s stream in input file '%s'\n",
		   av_get_media_type_string(type), control->src_filename);
        return ret;
    } else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            av_log(NULL, AV_LOG_ERROR,  "Failed to find %s codec\n",
			 av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate the %s codec context\n",
			 av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            av_log(NULL, AV_LOG_ERROR,  "Failed to copy %s codec parameters to decoder context\n",
			 av_get_media_type_string(type));
            return ret;
        }

        av_dict_set(&opts, "refcounted_frames", control->refcount ? "1" : "0", 0);
        if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
            av_log(NULL, AV_LOG_ERROR,  "Failed to open %s codec\n",
			 av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

static int seekto_pic_usec(CONTROL_T *control)
{
    int ret = 0;
    long pic_usec = control->pic_usec;
    if (pic_usec != AV_NOPTS_VALUE) {
        int64_t timestamp = 0;

        timestamp = pic_usec;
            /* add the stream start time */
        if (control->fmt_ctx->start_time != AV_NOPTS_VALUE)
            timestamp += control->fmt_ctx->start_time;
        ret = avformat_seek_file(control->fmt_ctx, -1, INT64_MIN, timestamp, INT64_MAX, AVSEEK_FLAG_BACKWARD);
            //ret = avformat_seek_file(control->fmt_ctx, -1, INT64_MIN, timestamp, INT64_MAX,  AVSEEK_FLAG_ANY);
        if (ret < 0) {
            av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
			 control->src_filename, (double)timestamp / AV_TIME_BASE);
        }
    }
    return ret;
}


static int control_seek(CONTROL_T *control, JNIEnv *env, const char *in_file, long pic_msec, const char* out_file)
{
  
    int ret = 0, got_frame =0;
    AVInputFormat *fmt=NULL;
    AVDictionary *options= NULL;
    
    control->src_filename = in_file;
    control->video_dst_filename = out_file;
    control->pic_usec = pic_msec*1000;
    control->env = env;
    
        //av_log_set_callback(av_log_qc_callback);
    av_log_set_level(AV_LOG_DEBUG);
    avcodec_register_all();
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
    avfilter_register_all();
    av_register_all();

    if (avformat_open_input(&control->fmt_ctx, control->src_filename, fmt, &options) < 0) {
        av_log(NULL, AV_LOG_ERROR,  "qc err not open source file %s\n", control->src_filename);
        return -1;
    }

    if (avformat_find_stream_info(control->fmt_ctx, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR,  "Could not find stream information\n");
        return -1;
    }

        /* qc seekto ,for get pic */
    seekto_pic_usec(control);
    
    if (open_codec_context(control, &control->video_stream_idx, &control->video_dec_ctx, control->fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        control->video_stream = control->fmt_ctx->streams[control->video_stream_idx];

        if(control->video_dst_filename){
            control->video_dst_file = fopen(control->video_dst_filename, "wb");
            if (!control->video_dst_file) {
                av_log(NULL, AV_LOG_ERROR,  "Could not open destination file %s\n", control->video_dst_filename);
                ret = 1;
                goto end;
            }
        }

        control->width = control->video_dec_ctx->width;
        control->height = control->video_dec_ctx->height;
        control->pix_fmt = control->video_dec_ctx->pix_fmt;
        ret = av_image_alloc(control->video_dst_data, control->video_dst_linesize,
                             control->width, control->height, control->pix_fmt, 1);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR,  "Could not allocate raw video buffer\n");
            goto end;
        }
        control->video_dst_bufsize = ret;
    }

#if 0
        /* dump input information to stderr */
    av_dump_format(control->fmt_ctx, 0, control->src_filename, 0);
#endif
    
    if (!control->video_stream) {
        av_log(NULL, AV_LOG_ERROR,  "Could not find video stream in the input, aborting\n");
        ret = 1;
        goto end;
    }

    control->frame = av_frame_alloc();
    if (!control->frame) {
        av_log(NULL, AV_LOG_ERROR,  "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    av_init_packet(&control->pkt);
    control->pkt.data = NULL;
    control->pkt.size = 0;

    if (control->video_stream){
        av_log(NULL, AV_LOG_DEBUG, "Demuxing video from file '%s'\n", control->src_filename);
        if(control->video_dst_filename){
            av_log(NULL, AV_LOG_DEBUG, "Demuxing video from file '%s' into '%s'\n", control->src_filename, control->video_dst_filename);
        }

    }
    
    while (av_read_frame(control->fmt_ctx, &control->pkt ) >= 0 &&
           !control->got_seek_frame) {
        AVPacket orig_pkt = control->pkt;
        do {
            ret = decode_packet(control, &got_frame, 0);
            if (ret < 0)
                break;
            control->pkt.data += ret;
            control->pkt.size -= ret;
        } while (control->pkt.size > 0 &&  !control->got_seek_frame);
        av_packet_unref(&orig_pkt);
    }

    control->pkt.data = NULL;
    control->pkt.size = 0;
    if( !control->got_seek_frame ){
        do {
            decode_packet(control, &got_frame, 1);
        } while (got_frame && !control->got_seek_frame);
    }

    if (control->video_stream) {
        av_log(NULL, AV_LOG_DEBUG, "Play the output video file with the command:\n"
		   "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n",
		   av_get_pix_fmt_name(control->pix_fmt), control->width, control->height,
		   control->video_dst_filename);
    }

  end:
    avcodec_free_context(&control->video_dec_ctx);
    avformat_close_input(&control->fmt_ctx);
    if (control->video_dst_file)
        fclose(control->video_dst_file);
    av_frame_free(&control->frame);
    av_free(control->video_dst_data[0]);

    return ret < 0;
}

