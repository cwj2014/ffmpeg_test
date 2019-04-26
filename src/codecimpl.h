#ifndef CODEC_IMPL_H_H_
#define CODEC_IMPL_H_H_

#include <functional>
#include <memory>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;

//编码回调接口
class EncodeCallback{
public:
    /**
     * 编码成功回调函数
     * ctx: 编码ctx
     * avpkt: 编码后的数据结构体
     */
    virtual void OnSuccess(AVCodecContext *ctx, const AVPacket* avpkt) = 0;
};

class DecodeCallback{
public:
    /**
     * 解码成功回调函数
     * ctx: 解码ctx
     * frame: 解码后的数据结构体
     */
    virtual void OnSuccess(AVCodecContext *ctx, const AVFrame* frame) = 0;
};

//编码成功回调函数
using OnEncodeSuccess = std::function<void(AVCodecContext*, const AVPacket*)>;

//解码成功回调函数
using OnDecodeSuccess = std::function<void(AVCodecContext*, const AVFrame*)>;

/**
 * 编码实现函数
 * ctx: 编码ctx
 * frame: 需要编码的数据
 * callback: 回调接口
 * return: 成功：0, 失败：非0
 */
int encode(AVCodecContext *ctx, const AVFrame *frame, std::shared_ptr<EncodeCallback> callback);

/**
 * 编码实现函数
 * ctx: 编码ctx
 * frame: 需要编码的数据
 * onSucess: 成功回调函数
 * return: 成功：0, 失败：非0
 */
int encode(AVCodecContext *ctx, const AVFrame *frame, OnEncodeSuccess onSucess);
/**
 * 解码实现函数
 * ctx: 解码ctx
 * packet: 需要解码的数据
 * callback: 回调接口
 * return: 成功：0, 失败：非0
 */
int decode(AVCodecContext *ctx, const AVPacket* packet, std::shared_ptr<DecodeCallback> callback);

/**
 * 解码实现函数
 * ctx: 解码ctx
 * packet: 需要解码的数据
 * onSucess: 成功回调函数
 * return: 成功：0, 失败：非0
 */
int decode(AVCodecContext *ctx, const AVPacket* packet, OnDecodeSuccess onSucess);

#endif