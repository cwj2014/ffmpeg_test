#include "pw_truecut_yuv_helper.h"
#include "video_filter_tool.h"
#include <stdio.h>
#include <stdint.h>
#include "libyuv.h"

#define YUV10BIT 1

void MergeUVRow(const uint8_t* src_u,
                  const uint8_t* src_v,
                  uint8_t* dst_uv,
                  int width, int depth){
    if(depth <= 8){
        int x;
        for (x = 0; x < width - 1; x += 2) {
            dst_uv[0] = src_u[x];
            dst_uv[1] = src_v[x];
            dst_uv[2] = src_u[x + 1];
            dst_uv[3] = src_v[x + 1];
            dst_uv += 4;
        }
        if (width & 1) {
            dst_uv[0] = src_u[width - 1];
            dst_uv[1] = src_v[width - 1];
        }
    }else{
        int x;
        for (x = 0; x < 2*(width - 1); x += 4) {
            //16bit u
            dst_uv[0] = src_u[x];
            dst_uv[1] = src_u[x+1];
            //16bit v
            dst_uv[2] = src_v[x];
            dst_uv[3] = src_v[x+1];
            //16bit u
            dst_uv[4] = src_u[x + 2];
            dst_uv[5] = src_u[x + 3];
            //16bit v
            dst_uv[6] = src_v[x + 2];
            dst_uv[7] = src_v[x + 3];
            
            dst_uv += 8;
        }
        //width is odd number
        if (width & 1) {
            dst_uv[0] = src_u[2*(width - 1)];
            dst_uv[1] = src_u[2*width - 1];
            dst_uv[2] = src_v[2*(width - 1)];
            dst_uv[3] = src_v[2*width - 1];
        }
    }
}


void MergeUVPlane(const uint8_t* src_u,
                  int src_stride_u,
                  const uint8_t* src_v,
                  int src_stride_v,
                  uint8_t* dst_uv,
                  int dst_stride_uv,
                  int width,
                  int height, 
                  int depth){
    for (int y = 0; y < height; ++y) {
        // Merge a row of U and V into a row of UV.
        MergeUVRow(src_u, src_v, dst_uv, width, depth);
        src_u += src_stride_u;
        src_v += src_stride_v;
        dst_uv += dst_stride_uv;
    }
    
 }


int yuv_transfer_test(){
    int width = 352;
    int height = 288;
    int format = IMAGE_CSP_YUV444F;
    int src_yuv_size, dst_yuv_size;
    int linesize;
    int in_linesize;
    int in_vlinesize;
    int depth;

    int top = 5;
    int left = 5;

    #if YUV10BIT
    FILE* rFile = fopen("./output.yuv","rb");
    FILE* oFile = fopen("./akiyo_cif_10bit444.yuv","wb");
    FILE* ooFile = fopen("./akiyo_cif_10bitnv12.yuv", "wb");
    src_yuv_size = width * height * 3;
    dst_yuv_size = (width - 2 * left) * (height - 2 * top) * 6;
    depth = 10;
    linesize = 2 * (width - 2 * left);
    in_linesize = 2 * width;
    in_vlinesize = height;
    #else
    FILE* rFile = fopen("./akiyo_cif.yuv", "rb");
    FILE* oFile = fopen("./akiyo_cif444.yuv","wb");
    FILE* ooFile = fopen("./akiyo_cif_nv12.yuv", "wb");
    src_yuv_size = width * height * 3 >> 1;
    dst_yuv_size = (width - 2 * left) * (height - 2 * top) * 3;
    depth = 8;
    linesize = (width - 2 * left);
    in_linesize = width;
    in_vlinesize = height;
    #endif

	if(rFile == NULL)
		return -1;
    
    
    uint8_t* src = new uint8_t[src_yuv_size];
    uint8_t* dst = new uint8_t[dst_yuv_size];
    
    while(1){
		if(feof(rFile))
			break;
        fread(src, 1, src_yuv_size, rFile);

        switch(format){
            case IMAGE_CSP_NV12:{
                int uv_size = in_linesize * height >> 2;
                uint8_t *src_u = new uint8_t[uv_size];
                memcpy(src_u, src + in_linesize * height, uv_size);
                uint8_t *src_v = new uint8_t[uv_size];
                memcpy(src_v, src + in_linesize * height + uv_size, uv_size);
                int src_stride_u = in_linesize >> 1;
                int src_stride_v = in_linesize >> 1;
                uint8_t *dst_uv = src + in_linesize * height;
                int dst_stride_uv = in_linesize;

                MergeUVPlane(src_u, src_stride_u, src_v, src_stride_v, dst_uv, dst_stride_uv,
                                    width, height >> 1, depth);

                copyValidYUVDataAndToYUV444(src, dst, linesize, top, left, width, height, in_linesize, in_vlinesize, format, depth);
                delete []src_u;
                delete []src_v;
                break;
            }
            case IMAGE_CSP_YV12:{
                int uv_size = in_linesize * height >> 2;
                uint8_t *temp = new uint8_t[uv_size];
                memcpy(temp, src + in_linesize * height, uv_size);//u
                memcpy(src + in_linesize * height, src + in_linesize * height + uv_size, uv_size); //v copy u's postion
                memcpy(src + in_linesize * height + uv_size, temp, uv_size);
                copyValidYUVDataAndToYUV444(src, dst, linesize, top, left, width, height, in_linesize, in_vlinesize, format, depth);
                delete[] temp;
                break;
            }
            case IMAGE_CSP_YUV422:{
                int uv_size = in_linesize * height >> 1;
                uint8_t *temp = new uint8_t[in_linesize * height * 2];
                memcpy(temp, src, in_linesize * height);//copy y
                uint8_t* dst_u = temp + in_linesize * height;
                uint8_t* dst_v = dst_u + uv_size;
                uint8_t* src_u = src + in_linesize*height;
                uint8_t* src_v = src_u + (in_linesize * height >> 2);
                for(int i = 0; i < height; i += 2){
                    memcpy(dst_u + i*in_linesize/2, src_u + i/2*in_linesize/2, in_linesize/2);
                    memcpy(dst_u + (i+1)*in_linesize/2, src_u + i/2*in_linesize/2, in_linesize/2);
                    memcpy(dst_v + i*in_linesize/2, src_v + i/2*in_linesize/2, in_linesize/2);
                    memcpy(dst_v + (i+1)*in_linesize/2, src_v + i/2*in_linesize/2, in_linesize/2);
                }
                //fwrite(temp, 1, in_linesize * height * 2, ooFile);
                copyValidYUVDataAndToYUV444(temp, dst, linesize, top, left, width, height, in_linesize, in_vlinesize, format, depth);
                delete[] temp;
                break;
            }
            case IMAGE_CSP_YUV444F:{
                uint8_t *temp = new uint8_t[in_linesize * height * 3];
                memcpy(temp, src, in_linesize * height);//copy y
                uint8_t* dst_u = temp + in_linesize * height;
                uint8_t* dst_v = dst_u + in_linesize * height;
                uint8_t* src_u = src + in_linesize*height;
                uint8_t* src_v = src_u + (in_linesize * height >> 2);
                int size = depth > 8 ? 2 : 1;
                for(int i = 0; i < height; i += 2){
                    for(int j = 0; j < width; j += 2){
                        memcpy(dst_u + i*in_linesize + j*size, src_u + i/2*in_linesize/2 + j/2*size, size);
                        memcpy(dst_u + i*in_linesize + (j + 1) * size, src_u + i/2*in_linesize/2 + j/2*size, size);
                        memcpy(dst_u + (i + 1) * in_linesize + j*size, src_u + i/2*in_linesize/2 + j/2*size, size);
                        memcpy(dst_u + (i + 1) * in_linesize + (j + 1) * size, src_u + i/2*in_linesize/2 + j/2*size, size);

                        memcpy(dst_v + i*in_linesize + j*size, src_v + i/2*in_linesize/2 + j/2*size, size);
                        memcpy(dst_v + i*in_linesize + (j + 1) * size, src_v + i/2*in_linesize/2 + j/2*size, size);
                        memcpy(dst_v + (i + 1) * in_linesize + j*size, src_v + i/2*in_linesize/2 + j/2*size, size);
                        memcpy(dst_v + (i + 1) * in_linesize + (j + 1) * size, src_v + i/2*in_linesize/2 + j/2*size, size);
                    }
                }
                // fwrite(temp, 1, in_linesize * height * 3, ooFile);
                copyValidYUVDataAndToYUV444(temp, dst, linesize, top, left, width, height, in_linesize, in_vlinesize, format, depth);

                //bool copyYUV444(void* in, void* dst, int linesize, int top, int left, int w, int h, int o_linesize, int o_vlinesize, int oformat, int depth)
                int datasize = depth > 8 ? 2 : 1;
                int o_linesize = (width + 10) * datasize;
                int o_vlinesize = (height + 10);
                int buffer_size = o_linesize * o_vlinesize * 2;
                int oformat = IMAGE_CSP_YUV422;
                uint8_t* YUVBuffer = new uint8_t[buffer_size];
                memset(YUVBuffer, 0, buffer_size);
                copyYUV444(temp, YUVBuffer, in_linesize, 5, 5, width, height, o_linesize, o_vlinesize, oformat, depth);
                fwrite(YUVBuffer, 1, buffer_size, ooFile);
                delete[] temp;
                break;
            }
            case IMAGE_CSP_V210:{
                int uv_size = in_linesize * height >> 1;
                uint8_t *temp = new uint8_t[in_linesize * height * 2];
                memcpy(temp, src, in_linesize * height);//copy y
                uint8_t* dst_u = temp + in_linesize * height;
                uint8_t* dst_v = dst_u + uv_size;
                uint8_t* src_u = src + in_linesize*height;
                uint8_t* src_v = src_u + (in_linesize * height >> 2);
                for(int i = 0; i < height; i += 2){
                    memcpy(dst_u + i*in_linesize/2, src_u + i/2*in_linesize/2, in_linesize/2);
                    memcpy(dst_u + (i+1)*in_linesize/2, src_u + i/2*in_linesize/2, in_linesize/2);
                    memcpy(dst_v + i*in_linesize/2, src_v + i/2*in_linesize/2, in_linesize/2);
                    memcpy(dst_v + (i+1)*in_linesize/2, src_v + i/2*in_linesize/2, in_linesize/2);
                }
                
                uint8_t * UYVYBuffer = new uint8_t[in_linesize * height * 2];
                uint8_t *tempDest = UYVYBuffer;
                int size = depth > 8 ? 2 : 1;
                uint8_t* dst_y = temp;
                for(int h = 0; h < height; h++){
                    for(int w = 0, k = 0; w < width; w += 2, k += 4){
                       memcpy(tempDest + k * size, dst_u + h * in_linesize / 2 + w/2 * size, size);
                       memcpy(tempDest + (k + 1) * size, dst_y + h * in_linesize + w * size, size);
                       memcpy(tempDest + (k + 2) * size, dst_v + h * in_linesize / 2 + w/2 * size, size);
                       memcpy(tempDest + (k + 3) * size, dst_y + h * in_linesize + (w + 1) * size, size);
                    }
                    tempDest += (in_linesize * 2);
                }
                fwrite(UYVYBuffer, 1, in_linesize * height * 2, ooFile);
                copyValidYUVDataAndToYUV444(UYVYBuffer, dst, linesize, top, left, width, height, in_linesize * 2, in_vlinesize, format, depth);
                break;
            }
            default:
                copyValidYUVDataAndToYUV444(src, dst, linesize, top, left, width, height, in_linesize, in_vlinesize, format, depth);
                break;
        }
        fwrite(dst, 1, dst_yuv_size, oFile);
    }

    fclose(rFile);
    fclose(oFile);
    fclose(ooFile);

    delete[] src;
    delete[] dst;

    return 0;
}