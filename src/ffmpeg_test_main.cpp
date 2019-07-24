#include "video_avfilter_test.h"
#include "separate_mp4_output_audio_video_mp4.h"
#include "merge_2mp4_output_mp4.h"
#include "exec_ffmpeg_test.h"
#include "encode_video_output_h264_test.h"
#include "encode_video_output_mp4_test.h"
#include "decode_video_output_yuv420_test.h"
#include "decode_h264_test.h"
#include "resample_audio_test.h"
#include "remuxing_test.h"
#include "av_util_dictionary_test.h"
#include "av_util_avclass_avoption_test.h"
#include "decode_video_output_one_image.h"
#include "decode_audio_output_pcm_test.h"
#include "decode_audio_mix_output_pcm.test.h"
#include "audio_filter_aformat_output_pcm.h"
#include "audio_filter_aresample_output_pcm.h"
#include "cut_mp4_test.h"


int main(){
    //decode_h264_test();
    //decode_video_output_yuv420_test();
    //encode_video_output_h264_test();
    //encode_video_output_mp4_test();
    // exec_ffmpeg_test();
    //merge_2mp4_output_mp4();
    //separate_mp4_output_audio_video_mp4_test();
    //video_avfilter_test();
    //resample_audio_test();
    //remuxing_test();
    //av_dictionary_test();
    //avclass_avoption_test();
    //decode_video_output_one_image_test();
    //decode_audio_output_pcm_test();
    // audio_filter_aformat_test();
    //audio_filter_aresample_test();
    // audio_filter_test();
    // decode_mix_audio_test();
    cut_media_file_test();
    return 0;
}