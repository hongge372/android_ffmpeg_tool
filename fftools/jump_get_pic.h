/**
 * create by wanglizheng, at 2018.08.14
 * for get cur video pic
 * but cur is using cpu for work
 */

#include<libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
//#include <jni.h>

typedef struct nothing_str{
    
}JNIEnv;

typedef struct nothing_jobject{
    
}jobject;

typedef struct {
    AVFormatContext *fmt_ctx;
    AVCodecContext *video_dec_ctx;
    int width, height;
    enum AVPixelFormat pix_fmt;
    AVStream *video_stream;
    const char *src_filename;
    const char *video_dst_filename;
    FILE *video_dst_file;

    uint8_t *video_dst_data[4];
    int video_dst_linesize[4];
    int video_dst_bufsize;

    int video_stream_idx;
    AVFrame *frame;
    AVPacket pkt;
    int video_frame_count;

/* Enable or disable frame reference counting. You are not supposed to support
 * both paths in your application but pick the one most appropriate to your
 * needs. Look for the use of refcount in this example to see what are the
 * differences of API usage between them. */
    int refcount;

    long pic_usec;
    int got_seek_frame;
    jobject bmp;
    JNIEnv *env;
} CONTROL_T;

static int  init_control(CONTROL_T *control);
static void uinit_control(CONTROL_T *control);
static int  bmp_get(CONTROL_T *control, int cached);
static void avframe_save(CONTROL_T *control);
static int  jpg_save(const char *dst, AVFrame* pFrame, int width, int height, char *extra);
static int  yuv_image_pass(CONTROL_T *control, int cached);
static int  decode_packet(CONTROL_T *control, int *got_frame, int cached);
static int  main_seek (JNIEnv *env, const char *in_file, long pic_msec, const char* out_file);
static int  open_codec_context(CONTROL_T *control, int *stream_idx,
                       AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type);
