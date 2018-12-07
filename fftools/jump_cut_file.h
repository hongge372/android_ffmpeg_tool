#ifndef __JUMP_CUT_FILE_H__
#define __JUMP_CUT_FILE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct pice_save_control{
        //cut info
    long start_time;
    long end_time;
    int64_t last_para_pts;

        //watch info
    double chg_rate;
    int first_write_flg;
    int para_start_pts;
    int para_write_flg;
    int64_t end_pts;
    
        //chechkout if end
    int abort_flg;
    int video_out;
    int audio_out;
}PICE_SAVE_CNTROL;

static int pice_save_file(char *in_filename, AVFormatContext *ifmt_ctx, AVFormatContext *ofmt_ctx, AVOutputFormat *ofmt,
                          long pice_start_time, long pice_end_time, int64_t *last_pice_para_pts,
                          float v_persentation_ms, float a_persentation_ms,
                          int *stream_mapping,
                          int stream_mapping_size);

static int demuxing_cut(char *in_filename, char *out_filename);
#endif
