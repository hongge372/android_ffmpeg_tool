/*
 *create by wanglizheng at 2018.11.19
 *for cut file
 *because ffmpeg cmd has some bug, I demuxing file and cut manually.
 *@para
 *@in_filename, input file for cut
 *@out_filename, output file after cut
 *@start_time, start time
 *@duration, cut duration
 */ 


#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

#include "jump_cut_file.h"

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           tag,
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find %s stream in input file\n",
		   av_get_media_type_string(type));
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

        if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
            av_log(NULL, AV_LOG_ERROR,  "Failed to open %s codec\n",
			 av_get_media_type_string(type));
            return ret;
        }
    }

    return 0;
}

static int pice_save_file(char *in_filename, AVFormatContext *ifmt_ctx, AVFormatContext *ofmt_ctx, AVOutputFormat *ofmt,
                          long pice_start_time, long pice_end_time, int64_t *last_pice_para_pts,
                          float v_persentation_ms, float a_persentation_ms,
                          int *stream_mapping,
                          int stream_mapping_size)
{
    int ret = -1;
    int64_t timestamp = 0;
    AVPacket pkt;
        //double float err, no this type !!!
    double pts_ms_time = 0;
    int64_t last_video_pts = 0;

    PICE_SAVE_CNTROL pice_save;
        //cut info
    pice_save.start_time = pice_start_time;
    pice_save.end_time = pice_end_time;
    pice_save.last_para_pts = *last_pice_para_pts;

        //watch info
    pice_save.chg_rate = 0;
    pice_save.first_write_flg = 0;
    pice_save.para_start_pts = 0;
    pice_save.para_write_flg = 0;
    pice_save.end_pts = 0; 
    
        //checkout if need  !!!
    pice_save.abort_flg = 0;
    pice_save.video_out = 0;
    pice_save.audio_out = 0;
    char *qc_stream_type;

    float shift_remove=0;
    
    memset(&pkt, 0, sizeof(AVPacket));
#if 1
    av_log(NULL, AV_LOG_ERROR, "qc cut, seek file for start (%ld) last_pice_para_pts:=%"PRId64" v_persentation_ms:=%f ", pice_save.start_time, *last_pice_para_pts, v_persentation_ms);
    timestamp = (int64_t)pice_save.start_time * 1000;
    if(ifmt_ctx->start_time != AV_NOPTS_VALUE){
        timestamp += ifmt_ctx->start_time;
    }
    
    av_log(NULL, AV_LOG_ERROR, "qc cut video,seek_file to timestamp:%lld ",(long long int) timestamp);
    ret = avformat_seek_file(ifmt_ctx, -1, INT64_MIN, timestamp, INT64_MAX, AVSEEK_FLAG_BACKWARD);
        //ret = avformat_seek_file(control->fmt_ctx, -1, INT64_MIN, timestamp, INT64_MAX,  AVSEEK_FLAG_ANY);
    if (ret < 0) {
        av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
               in_filename, (double)timestamp / AV_TIME_BASE);
    }
#endif
    av_log(NULL, AV_LOG_ERROR, "pts_ms_time:= %lf pice_save.start_time:=%ld end_time:=%ld \n",  pts_ms_time, pice_save.start_time, pice_save.end_time);
    while (1 && pice_save.abort_flg == 0) {
        AVStream *in_stream, *out_stream;
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;
        
        in_stream  = ifmt_ctx->streams[pkt.stream_index];
        if (pkt.stream_index >= stream_mapping_size ||
            stream_mapping[pkt.stream_index] < 0) {
            av_packet_unref(&pkt);
            continue;
        }

        pkt.stream_index = stream_mapping[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];
        log_packet(ifmt_ctx, &pkt, "**ffmpeg dump in**");
        
            /* copy packet */
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;

//        av_log(NULL, AV_LOG_ERROR, "(: ,pkt.dts:=%"PRId64" pkt.pts%"PRId64" , pkt.duration%"PRId64" \n", pkt.dts, pkt.pts, pkt.duration );
        log_packet(ofmt_ctx, &pkt, "**ffmpeg dump out** :");
        pice_save.chg_rate = av_q2d( ofmt_ctx->streams[pkt.stream_index]->time_base);
            //up to ms
        pts_ms_time = pkt.pts * pice_save.chg_rate  * 1000;

        if(pice_save.video_out && pice_save.audio_out){
            pice_save.abort_flg = 1;
            av_packet_unref(&pkt);
            break;
        }

        if(pkt.stream_index == AVMEDIA_TYPE_VIDEO &&
           pts_ms_time + v_persentation_ms/2 >= pice_save.end_time){
            av_log(NULL, AV_LOG_ERROR, "qc demux packet, final video packet get \n" );
            last_video_pts = pkt.pts;
            pice_save.video_out = 1;
            continue;
        }

        if(pkt.stream_index == AVMEDIA_TYPE_AUDIO &&
           pts_ms_time + a_persentation_ms >= pice_save.end_time + v_persentation_ms){
            av_log(NULL, AV_LOG_ERROR, "qc demux packet, final audio packet get \n" );
            pice_save.audio_out = 1;
            continue;
        }

            //llf is err, there is no this type !!!
        if(pkt.stream_index == AVMEDIA_TYPE_AUDIO){
            qc_stream_type = "AUDIO";
        }
        else if(pkt.stream_index == AVMEDIA_TYPE_VIDEO){
            qc_stream_type = "VIDEO";
        }
        
        av_log(NULL, AV_LOG_ERROR, "OOOO get %s ---> pice_save.abort_flg:=%d  pts_ms_time:= %lf pkt.pts:=%"PRId64" pkt.dts:=%"PRId64" pice_save.end_pts:= %"PRId64" pice_save.last_para_pts:=%"PRId64" \n",
               qc_stream_type, pice_save.abort_flg,  pts_ms_time, pkt.pts, pkt.dts, pice_save.end_pts, pice_save.last_para_pts);

        if(pice_save.abort_flg == 0 &&
           pts_ms_time >= pice_save.start_time &&
           pts_ms_time < pice_save.end_time){
            if(!pice_save.first_write_flg){
                pice_save.para_start_pts = pkt.pts;
                pice_save.first_write_flg = 1;
            }            
            if( pice_save.last_para_pts ){
                shift_remove = v_persentation_ms /(1000 * pice_save.chg_rate);
                pkt.dts = pkt.pts = pkt.pts - pice_save.para_start_pts + pice_save.last_para_pts + shift_remove;
            }else{
                pkt.dts = pkt.pts = pkt.pts - pice_save.para_start_pts + pice_save.last_para_pts;
            }
            if(pkt.stream_index == AVMEDIA_TYPE_VIDEO){
                *last_pice_para_pts = pice_save.end_pts = pkt.dts;
            }
            *last_pice_para_pts = pice_save.end_pts = pkt.dts;
            av_log(NULL, AV_LOG_ERROR, "OOOOOO dump pts data pkt.pts:=%"PRId64" pice_save.para_start_pts:=%"PRId64" pice_save.end_pts:= %"PRId64" *last_pice_para_pts:=%"PRId64 " <------!!!!!!!!!!!!\n",
                   pkt.pts , pice_save.para_start_pts, pice_save.end_pts, *last_pice_para_pts);
            ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
            if (ret < 0) {
                fprintf(stderr, "Error muxing packet\n");
                break;
            }            
        }
        av_packet_unref(&pkt);
    }
    return ret;
}

static int demuxing_cut(char *in_filename, char *out_filename)
{
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    int ret, i;
    int stream_index = 0;
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;
    int64_t last_pice_para_pts = 0;
        //int64_t pice.end_time = duration * 1000;
        //float qaingzhuan bixu !!!
        //float pice.end_time =(float) duration;
    int audio_stream_idx = 0;
    AVCodecContext *audio_dec_ctx = NULL;
    int frame_size = 0;
    int sample_rate = 0;
    float a_persentation_ms = 0.0f;
    float v_persentation_ms = 0;

#if 1
#define ARRAY_SIZE 4
    long int time_array[ARRAY_SIZE]={
        0, 300, 20000, 22000,
    };
#else
#define ARRAY_SIZE 2
    long int time_array[ARRAY_SIZE]={
        0, 300
    };
#endif
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        goto end;
    }


    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }

    
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    
    if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, ifmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
        frame_size = audio_dec_ctx->frame_size;
        sample_rate = audio_dec_ctx->sample_rate;
        a_persentation_ms = (float)frame_size * 1000 / sample_rate;
        av_log(NULL, AV_LOG_ERROR, "qc get audio frame_size %d  sample_rate:= %d a_persentation_ms:=%f ! \n", frame_size, sample_rate, a_persentation_ms);
        
        
    }

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    stream_mapping_size = ifmt_ctx->nb_streams;
    stream_mapping = av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
    if (!stream_mapping) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ofmt = ofmt_ctx->oformat;

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *out_stream;
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            fprintf(stderr, "Failed to copy codec parameters\n");
            goto end;
        }
        out_stream->codecpar->codec_tag = 0;

        if(in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
                //v_persentation_ms = in_stream;
            AVRational r_frame_rate = in_stream->r_frame_rate;
            AVRational avg_frame_rate = in_stream->avg_frame_rate;
            av_log(NULL, AV_LOG_ERROR, "here get r_frame_rate.den:= %d ", r_frame_rate.den);
            av_log(NULL, AV_LOG_ERROR, "r_frame_rate.num:= %d ", r_frame_rate.num);
            av_log(NULL, AV_LOG_ERROR, "avg_frame_rate.den:= %d ", avg_frame_rate.den);
            av_log(NULL, AV_LOG_ERROR, "avg_frame_rate.num:= %d \n", avg_frame_rate.num);
            if(avg_frame_rate.den != 0 && avg_frame_rate.num != 0){
                v_persentation_ms = avg_frame_rate.num / avg_frame_rate.den;
                v_persentation_ms = 1000 / v_persentation_ms;
            }
            else if(r_frame_rate.den != 0 && r_frame_rate.num != 0){
                v_persentation_ms = r_frame_rate.num / r_frame_rate.den;
                v_persentation_ms = 1000 / v_persentation_ms;
            }else{
                v_persentation_ms = 40;
            }
        }
    }
    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_filename);
            goto end;
        }
    }

    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }

    for(i=0; i < ARRAY_SIZE; i+=2){
        av_log(NULL, AV_LOG_ERROR, "\n\n----- cut loop %d --pice_save.start_time(%ld), end_time(%ld)-----\n" , i/2, time_array[i], time_array[i+1]);
        pice_save_file(in_filename, ifmt_ctx, ofmt_ctx, ofmt,
                       time_array[i], time_array[i+1], &last_pice_para_pts,
                       v_persentation_ms, a_persentation_ms,
                       stream_mapping,
                       stream_mapping_size
                       );
    }
    
    
    av_write_trailer(ofmt_ctx);

  end:
    avformat_close_input(&ifmt_ctx);

        /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    av_freep(&stream_mapping);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}
