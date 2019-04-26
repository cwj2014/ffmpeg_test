#include "video_avfilter_test.h"
#include "separate_mp4_output_audio_video_mp4.h"
#include "merge_2mp4_output_mp4.h"
#include "exec_ffmpeg_test.h"
#include "encode_video_output_h264_test.h"
#include "encode_video_output_mp4_test.h"
#include "decode_video_output_yuv420_test.h"
#include "decode_h264_test.h"
#include "resample_audio_test.h"


int main(){
    //decode_h264_test();
    //decode_video_output_yuv420_test();
    //encode_video_output_h264_test();
    //encode_video_output_mp4_test();
    //exec_ffmpeg_test();
    //merge_2mp4_output_mp4();
    //separate_mp4_output_audio_video_mp4_test();
    //video_avfilter_test();
    resample_audio_test();
    return 0;
}