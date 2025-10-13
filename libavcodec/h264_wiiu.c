/**
 * Decoder using Wii U's h264.rpl
 * Created 2020 by GaryOderNichts
 */ 

#include "h264_wiiu.h"

#include "libavutil/imgutils.h"
#include "internal.h"
#include "codec_internal.h"

#include <malloc.h>

static void frame_callback(H264DecodeOutput *output) { }

static int h264_wiiu_decode_init(AVCodecContext *avctx)
{
    WIIUContext* ctx = avctx->priv_data;
    uint32_t memRequirement;
    int res;
    void* decoder;
    int framebuf_size;
    void* framebuffer;

    // memory requirement for the maximum supported level (level 42)
    memRequirement = 0x2200000 + 0x3ff + 0x480000;

    decoder = malloc(memRequirement);
    res = H264DECInitParam(memRequirement, decoder);
    if (res != 0)
    {
        av_log(avctx, AV_LOG_ERROR, "h264_wiiu: Error initializing decoder 0x%07X", res);
        return -1;
    }

    res = H264DECSetParam_FPTR_OUTPUT(decoder, frame_callback);
    if (res != 0)
    {
        av_log(avctx, AV_LOG_ERROR, "h264_wiiu: Error setting callback 0x%07X", res);
        return -1;
    }

    res = H264DECSetParam_OUTPUT_PER_FRAME(decoder, 1);
    if (res != 0)
    {
        av_log(avctx, AV_LOG_ERROR, "h264_wiiu: Error setting OUTPUT_PER_FRAME 0x%07X", res);
        return -1;
    }

    res = H264DECOpen(decoder);
    if (res != 0)
    {
        av_log(avctx, AV_LOG_ERROR, "h264_wiiu: Error opening decoder 0x%07X", res);
        return -1;
    }

    res = H264DECBegin(decoder);
    if (res != 0)
    {
        av_log(avctx, AV_LOG_ERROR, "h264_wiiu: Error preparing decoder 0x%07X", res);
        return -1;
    }

    ctx->decoder = decoder;

    framebuf_size = (avctx->width * avctx->height * 3) >> 1;
    framebuffer = memalign(1024, framebuf_size);

    ctx->framebuffer = framebuffer;
    ctx->framebuffer_size = framebuf_size;

    // Wii U decoder only outputs NV12
    avctx->pix_fmt = AV_PIX_FMT_NV12;

    return 0;
}

static int h264_wiiu_decode_close(AVCodecContext *avctx)
{
    WIIUContext* ctx = avctx->priv_data;
    void* decoder = ctx->decoder;
    void* framebuffer = ctx->framebuffer;

    free(framebuffer);

    H264DECFlush(decoder);
    H264DECEnd(decoder);
    H264DECClose(decoder);

    free(decoder);

    return 0;
}

static int h264_wiiu_decode_frame(AVCodecContext *avctx, AVFrame *data, int *got_frame, AVPacket *avpkt)
{
    WIIUContext* ctx = avctx->priv_data;
    uint8_t* buf = avpkt->buf->data;
    int buf_size = avpkt->buf->size;
    void* decoder = ctx->decoder;
    void* framebuffer = ctx->framebuffer;
    uint8_t* pointers[2];
    AVFrame *avframe = data;
    int pitch;
    int linesize[2];
    int res;
    BOOL skippable = FALSE;

    res = H264DECCheckSkipableFrame(buf, buf_size, &skippable);
    if (res != 0)
    {
        av_log(avctx, AV_LOG_ERROR, "h264_wiiu: Error checking for skippable frame");
        return -1;
    }

    if (skippable)
    {
        // skip the frame
        return 0;
    }

    res = H264DECSetBitstream(decoder, buf, buf_size, 0);
    if (res != 0)
    {
        av_log(avctx, AV_LOG_ERROR, "h264_wiiu: Error setting bitstream 0x%07X", res);
        return -1;
    }

    res = H264DECExecute(decoder, framebuffer);
    if (res != 0xE4)
    {
        av_log(avctx, AV_LOG_ERROR, "h264_wiiu: Error decoding frame 0x%07X", res);
        return -1;
    }

    if (ff_get_buffer(avctx, avframe, 0) < 0) 
    {
        av_log(avctx, AV_LOG_ERROR, "h264_wiiu: Unable to allocate buffer");
        return AVERROR(ENOMEM);
    }

    pitch = (avctx->width + (256 - 1)) & ~(256 - 1);
    linesize[0] = linesize[1] = pitch;

    // Y
    pointers[0] = (uint8_t*) framebuffer;
    // U/V
    pointers[1] = ((uint8_t*) framebuffer) + (avctx->height * pitch);

    av_image_copy(avframe->data, avframe->linesize, (const uint8_t **) pointers, linesize, AV_PIX_FMT_NV12, avctx->width, avctx->height);

    *got_frame = 1;
    return avpkt->size;
}

const FFCodec ff_h264_wiiu_decoder =
{
    .p.name           = "h264_wiiu",
    CODEC_LONG_NAME(NULL_IF_CONFIG_SMALL("H.264 Decoder using h264.rpl")),
    .p.type           = AVMEDIA_TYPE_VIDEO,
    .p.id             = AV_CODEC_ID_H264,
    .priv_data_size = sizeof(WIIUContext),
    .init           = h264_wiiu_decode_init,
    .close          = h264_wiiu_decode_close,
    FF_CODEC_DECODE_CB(h264_wiiu_decode_frame),
    .bsfs           = "h264_mp4toannexb",
};