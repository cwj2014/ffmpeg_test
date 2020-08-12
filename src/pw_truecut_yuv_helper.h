#include "pw_truecut_hdr.h"
#include <memory.h>
#include <stdint.h>

#define CUDA 0


static inline void copyUVPlaner(uint8_t* dst_u,uint8_t* dst_v, uint8_t* src_u, uint8_t* src_v, int w, int h,  int linesize, int in_linesize, int depth){
    int m = w;
    int step = 1;
    int size = 1;
    if(depth > 8){
        m = 2*w;
        step = 2;
        size = 2;
    }
    for(int j=0; j<h; j++){
        for(int k = 0; k < m; k += step){
            #if CUDA
                cudaMemcpy(dst_u + j * linesize + k, src_u + j / 2 * in_linesize / 2 + k / (2 * size) * size, size, cudaMemcpyDeviceToDevice);
                cudaMemcpy(dst_v + j * linesize + k, src_v + j / 2 * in_linesize / 2 + k / (2 * size) * size, size, cudaMemcpyDeviceToDevice);
            #else
                memcpy(dst_u + j * linesize + k, src_u + j / 2 * in_linesize / 2 + k / (2 * size) * size, size);
                memcpy(dst_v + j * linesize + k, src_v + j / 2 * in_linesize / 2 + k / (2 * size) * size, size);
            #endif
        }
    }
}

static inline void copyYPlaner(uint8_t* dst_y, uint8_t* src_y, int h, int linesize, int in_linesize){
    for(int i=0; i<h; i++){
    #if CUDA
        cudaMemcpy(dst_yuv + i * linesize, src_yuv + i * in_linesize, linesize, cudaMemcpyDeviceToDevice);
    #else
        memcpy(dst_y + i * linesize, src_y + i * in_linesize, linesize);
    #endif
    }
}

/**
 * dst是复制有效数据的buffer
 * linesize是dst的长度，　实质上是（w－２＊left）＊datasize
 */
bool copyValidYUVDataAndToYUV444(void* in, void* dst, int linesize, int top, int left, int w, int h, int in_linesize, int in_vlinesize, int format, int depth){
    
    int vaild_yuv_width = w - 2 * left;
    int vaild_yuv_height = h - 2 * top;

    uint8_t* src_y = (uint8_t*)in + top * in_linesize + left * (depth > 8 ? 2 : 1);
    uint8_t* dst_y = (uint8_t*)dst;
    uint8_t* dst_u = dst_y + vaild_yuv_height * linesize;
    uint8_t* dst_v = dst_u + vaild_yuv_height * linesize;
    switch(format){
        case IMAGE_CSP_NV12:{
            copyYPlaner(dst_y, src_y, vaild_yuv_height, linesize, in_linesize);
            uint8_t* src_uv = (uint8_t*)in + in_vlinesize * in_linesize + top / 2 * in_linesize + left / 2 * (depth > 8 ? 2 : 1);
            int m = vaild_yuv_width - 1;
            int step = 2;
            int size = 1;
            if(depth > 8){
                m = 2*(vaild_yuv_width-1);
                step = 4;
                size = 2;
            }
            for(int j=0; j< vaild_yuv_height - 1; j += 2){
                for(int k = 0; k < m; k += step){
                    #if CUDA
                        cudaMemcpy(dst_u + j * linesize + k, src_uv + j/2 * in_linesize + k, size, cudaMemcpyDeviceToDevice);
                        cudaMemcpy(dst_u + j * linesize + k + size, src_uv + j/2 * in_linesize + k, size, cudaMemcpyDeviceToDevice);
                        cudaMemcpy(dst_u + (j + 1) * linesize + k, src_uv + j/2 * in_linesize + k, size, cudaMemcpyDeviceToDevice);
                        cudaMemcpy(dst_u + (j + 1) * linesize + k + size, src_uv + j/2 * in_linesize + k, size, cudaMemcpyDeviceToDevice);
                        cudaMemcpy(dst_v + j * linesize + k, src_uv + j/2 * in_linesize + k + 1, size, cudaMemcpyDeviceToDevice);
                        cudaMemcpy(dst_v + j * linesize + k + size, src_uv + j/2 * in_linesize + k + 1, size, cudaMemcpyDeviceToDevice);
                        cudaMemcpy(dst_v + (j + 1) * linesize + k, src_uv + j/2 * in_linesize + k + 1, size, cudaMemcpyDeviceToDevice);
                        cudaMemcpy(dst_v + (j + 1) * linesize + k + size, src_uv + j/2 * in_linesize + k + 1, size, cudaMemcpyDeviceToDevice);
                    #else
                        memcpy(dst_u + j * linesize + k, src_uv + j/2 * in_linesize + k, size);
                        memcpy(dst_u + j * linesize + k + size, src_uv + j/2 * in_linesize + k, size);
                        memcpy(dst_u + (j + 1) * linesize + k, src_uv + j/2 * in_linesize + k, size);
                        memcpy(dst_u + (j + 1) * linesize + k + size, src_uv + j/2 * in_linesize + k, size);
                        memcpy(dst_v + j * linesize + k, src_uv + j/2 * in_linesize + k + size, size);
                        memcpy(dst_v + j * linesize + k + size, src_uv + j/2 * in_linesize + k + size, size);
                        memcpy(dst_v + (j + 1) * linesize + k, src_uv + j/2 * in_linesize + k + size, size);
                        memcpy(dst_v + (j + 1) * linesize + k + size, src_uv + j/2 * in_linesize + k + size, size);
                    #endif
                }
            }
            break;
        }
        case IMAGE_CSP_I420:{
            copyYPlaner(dst_y, src_y, vaild_yuv_height, linesize, in_linesize);
            uint8_t* src_u = (uint8_t*)in + in_vlinesize * in_linesize + top / 2 * in_linesize / 2 + left / 2 * (depth > 8 ? 2 : 1);
            uint8_t* src_v = (uint8_t*)in + (in_vlinesize * in_linesize * 5) / 4 +  top / 2 * in_linesize / 2 + left / 2 * (depth > 8 ? 2 : 1);
            copyUVPlaner(dst_u, dst_v, src_u, src_v, vaild_yuv_width, vaild_yuv_height, linesize, in_linesize, depth);
            break;
        }
        case IMAGE_CSP_YV12:{
            copyYPlaner(dst_y, src_y, vaild_yuv_height, linesize, in_linesize);
            uint8_t* src_v = (uint8_t*)in + in_vlinesize * in_linesize + top / 2 * in_linesize / 2 + left / 2 * (depth > 8 ? 2 : 1);
            uint8_t* src_u = (uint8_t*)in + (in_vlinesize * in_linesize * 5) / 4 +  top / 2 * in_linesize / 2 + left / 2 * (depth > 8 ? 2 : 1);
            copyUVPlaner(dst_u, dst_v, src_u, src_v, vaild_yuv_width, vaild_yuv_height, linesize, in_linesize, depth);
            break;
        }
        case IMAGE_CSP_YUV422:{
            copyYPlaner(dst_y, src_y, vaild_yuv_height, linesize, in_linesize);
            uint8_t* src_u = (uint8_t*)in + in_vlinesize * in_linesize + top * in_linesize / 2 + left / 2 * (depth > 8 ? 2 : 1);
            uint8_t* src_v = (uint8_t*)in + in_vlinesize * in_linesize * 3 / 2 + top * in_linesize / 2 + left / 2 * (depth > 8 ? 2 : 1);
            int m = vaild_yuv_width;
            int step = 1;
            int size = 1;
            if(depth > 8){
                m = 2*vaild_yuv_width;
                step = 2;
                size = 2;
            }
            for(int j = 0; j < vaild_yuv_height; j++){
                for(int k = 0; k < m; k += step){
                #if CUDA
                    cudaMemcpy(dst_u + j * linesize + k, src_u + j * in_linesize / 2 + k / (2 * size) * size, size, cudaMemcpyDeviceToDevice);
                    cudaMemcpy(dst_v + j * linesize + k, src_v + j * in_linesize / 2 + k / (2 * size) * size, size, cudaMemcpyDeviceToDevice);
                #else
                   memcpy(dst_u + j * linesize + k, src_u + j * in_linesize / 2 + k / (2 * size) * size, size);
                   memcpy(dst_v + j * linesize + k, src_v + j * in_linesize / 2 + k / (2 * size) * size, size);
                #endif
                }
            }
            break;
        }
        case IMAGE_CSP_YUV444F:{
            copyYPlaner(dst_y, src_y, vaild_yuv_height, linesize, in_linesize);
            uint8_t* src_u = (uint8_t*)in + in_vlinesize * in_linesize + top * in_linesize + left * (depth > 8 ? 2 : 1);
            uint8_t* src_v = (uint8_t*)in + in_vlinesize * in_linesize * 2 + top * in_linesize + left * (depth > 8 ? 2 : 1);
            int size = vaild_yuv_width;
            if(depth > 8){
                size = 2 * vaild_yuv_width;
            }
            for(int j = 0; j < vaild_yuv_height; j++){
                #if CUDA
                    cudaMemcpy(dst_u + j * linesize, src_u + j * in_linesize, size, cudaMemcpyDeviceToDevice);
                    cudaMemcpy(dst_v + j * linesize, src_v + j * in_linesize, size, cudaMemcpyDeviceToDevice);
                #else
                   memcpy(dst_u + j * linesize, src_u + j * in_linesize, size);
                   memcpy(dst_v + j * linesize, src_v + j * in_linesize, size);
                #endif
            }
            break;
        }
        case IMAGE_CSP_V210:{
            int size = depth > 8 ? 2 : 1;
            uint8_t* src_yuv = (uint8_t*)in;
            for(int i = 0; i < vaild_yuv_height; i++){
                for(int j = 0, k = 0; j < vaild_yuv_width; j += 2, k += 4){
                #if CUDA
                    cudaMemcpy(dst_u + i * linesize + j * size, src_yuv + (top + i) * in_linesize + (left / 2 * 4 + k) * size,  size, cudaMemcpyDeviceToDevice);
                    cudaMemcpy(dst_u + i * linesize + (j + 1) * size, src_yuv + (top + i) * in_linesize + (left / 2 * 4 + k) * size,  size, cudaMemcpyDeviceToDevice);

                    cudaMemcpy(dst_y + i * linesize + j * size, src_yuv + (top + i) * in_linesize + (left / 2 * 4 + k + 1) * size, size, cudaMemcpyDeviceToDevice);
                    cudaMemcpy(dst_y + i * linesize + (j + 1) * size, src_yuv + (top + i) * in_linesize + (left / 2 * 4 + k + 3) * size, size, cudaMemcpyDeviceToDevice);

                    cudaMemcpy(dst_v + i * linesize + j * size, src_yuv + (top + i) * in_linesize + (left / 2 * 4 + k + 2) * size, size, cudaMemcpyDeviceToDevice);
                    cudaMemcpy(dst_v + i * linesize + (j + 1) * size, src_yuv + (top + i) * in_linesize + (left / 2 * 4 + k + 2) * size, size, cudaMemcpyDeviceToDevice);
                #else           
                    memcpy(dst_u + i * linesize + j * size, src_yuv + (top + i) * in_linesize + (left / 2 * 4 + k) * size,  size);
                    memcpy(dst_u + i * linesize + (j + 1) * size, src_yuv + (top + i) * in_linesize + (left / 2 * 4 + k) * size,  size);

                    memcpy(dst_y + i * linesize + j * size, src_yuv + (top + i) * in_linesize + (left / 2 * 4 + k + 1) * size, size);
                    memcpy(dst_y + i * linesize + (j + 1) * size, src_yuv + (top + i) * in_linesize + (left / 2 * 4 + k + 3) * size, size);

                    memcpy(dst_v + i * linesize + j * size, src_yuv + (top + i) * in_linesize + (left / 2 * 4 + k + 2) * size, size);
                    memcpy(dst_v + i * linesize + (j + 1) * size, src_yuv + (top + i) * in_linesize + (left / 2 * 4 + k + 2) * size, size);
                #endif
                }
            }
            break;
        }
        default:{
            return false;
        }
    }
    return true;
}

bool copyYUV444(void* in, void* dst, int linesize, int top, int left, int w, int h, int o_linesize, int o_vlinesize, int oformat, int depth){
    uint8_t* src_y = (uint8_t*)in;
    uint8_t* dst_y = (uint8_t*)dst + top * o_linesize + left * (depth > 8 ? 2 : 1);;
    uint8_t* src_u = (uint8_t*)in + linesize * h;
    uint8_t* src_v = src_u + linesize * h;
    switch(oformat){
        case IMAGE_CSP_YUV422:{
            copyYPlaner(dst_y, src_y, h, o_linesize, linesize);
            uint8_t* dst_u = (uint8_t*)dst + o_linesize * o_vlinesize + top * o_linesize / 2 + left / 2 * (depth > 8 ? 2 : 1);
            uint8_t* dst_v = (uint8_t*)dst + (o_linesize * o_vlinesize * 3) / 2 + top * o_linesize / 2 + left / 2 * (depth > 8 ? 2 : 1);
            int m = w / 2;
            int step = 1;
            int size = 1;
            if(depth > 8){
                m = w;
                step = 2;
                size = 2;
            }
            for(int j = 0; j < h; j++){
                for(int k = 0; k < m; k += step){
                #if CUDA
                    cudaMemcpy(dst_u + j * o_linesize / 2 + k, src_u + j * linesize + k * 2, size, cudaMemcpyDeviceToDevice);
                    cudaMemcpy(dst_v + j * o_linesize / 2 + k, src_v + j * linesize + k * 2, size, cudaMemcpyDeviceToDevice);
                #else
                   memcpy(dst_u + j * o_linesize / 2 + k, src_u + j * linesize + k * 2, size);
                   memcpy(dst_v + j * o_linesize / 2 + k, src_v + j * linesize + k * 2, size);
                #endif
                }
            }
            break;
        }
        case IMAGE_CSP_I420:{
            copyYPlaner(dst_y, src_y, h, o_linesize, linesize);
            uint8_t* dst_u = (uint8_t*)dst + o_linesize * o_vlinesize + top / 2 * o_linesize / 2 + left / 2 * (depth > 8 ? 2 : 1);
            uint8_t* dst_v = (uint8_t*)dst + (o_linesize * o_vlinesize * 5) / 4 + top / 2 * o_linesize / 2 + left / 2 * (depth > 8 ? 2 : 1);
            int m = w / 2;
            int step = 1;
            int size = 1;
            if(depth > 8){
                m = w;
                step = 2;
                size = 2;
            }
            for(int j = 0; j < h / 2; j++){
                for(int k = 0; k < m; k += step){
                #if CUDA
                    cudaMemcpy(dst_u + j * o_linesize / 2 + k, src_u + j * 2 * linesize + k * 2, size, cudaMemcpyDeviceToDevice);
                    cudaMemcpy(dst_v + j * o_linesize / 2 + k, src_v + j * 2 *  linesize + k * 2, size, cudaMemcpyDeviceToDevice);
                #else
                   memcpy(dst_u + j * o_linesize / 2 + k, src_u + j * 2 * linesize + k * 2, size);
                   memcpy(dst_v + j * o_linesize / 2 + k, src_v + j * 2 *  linesize + k * 2, size);
                #endif
                }
            }
            break;
        }
        default:{
            return false;
        }
    }
    return true;
}