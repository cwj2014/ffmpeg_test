#ifndef PW_TRUECUT_HDR_H_H_
#define PW_TRUECUT_HDR_H_H_

#define IMAGE_CSP_NONE                      -1
#define IMAGE_CSP_NV12                      0 // 10bits是高位10bit， 其它都是低位10bit
#define IMAGE_CSP_I420                      1
#define IMAGE_CSP_YV12                      2
#define IMAGE_CSP_YUV422                    10
#define IMAGE_CSP_YUV444F      12 //planar
#define IMAGE_CSP_V210       13 //UYVY bit zip

typedef struct MediaInfo
{
    int iformat; //input format
    int oformat; //output format
    int w;   
    int h;
    int in_depth; // 8bits or 10bits
    int out_depth; // 8bits or 10bits
    int in_linesize; 
    int out_linesize;
    int left, top; // 注：格式变换可能会添加黑边，黑边可能会影响图像处理效果。
    int in_vlinesize; // 有些硬解有高度对齐的yuv数据
    int out_vlinesize;
}MediaInfo;


typedef struct PWTCHDRMetadata
{
}PWTCHDRMetadata;

typedef struct PWTCHDRHandle PWTCHDRHandle;

#ifdef  __cplusplus
extern "C"{
#endif

//初始化函数
PWTCHDRHandle* pw_truecut_hdr_init(MediaInfo mi, void* config, int mode, int gpu_core_Id, char *key);
//获取metadata
PWTCHDRMetadata* get_pw_truecut_metadata(PWTCHDRHandle* handle);
//hdr处理函数
void pw_truecut_hdr_process(void* in, void* out, PWTCHDRHandle* handle);
//释放函数
void pw_truecut_hdr_uninit(PWTCHDRHandle* handle);

#ifdef __cplusplus
}
#endif

#endif
