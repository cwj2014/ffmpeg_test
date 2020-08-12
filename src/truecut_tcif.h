#ifndef TRUECUT_TCIF_H_H_

#include <stdint.h>

#ifdef __cplusplus 
extern "C" { 
#endif

typedef struct TCIFHandle TCIFHandle;
typedef struct AVIOContext AVIOContext;
typedef struct  AVPacket AVPacket;

/**
 * create tcif handle
 */
TCIFHandle* createHandle(const uint8_t* extradata, uint64_t extradata_size, uint32_t width, uint32_t height);

#define TCIF_SINGLE_IMAGE_FLAG 0 //single frame
#define TCIF_MULTIPLE_IMAGE_FLAG 1 //more than one frame
#define TCIF_VIDEO_FLAG 2 //video
/**
* configure param
*/
int configHandle(TCIFHandle* handle, int flag, uint32_t timeScale, uint64_t duration);
/**
 * delete tcif handle
 */
void deleteHandle(TCIFHandle* handle);
/**
 * save tcif 
 */
int saveImage(TCIFHandle* handle,  AVIOContext* avioContext, const char* url);

/**
 * add avpacket into tcif
 */ 
int addAVPacket(TCIFHandle* handle, AVPacket* avpkt);

#ifdef __cplusplus 
} 
#endif

#endif