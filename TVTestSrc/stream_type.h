/*******************************************************************
                         PES stream type
 *******************************************************************/

#ifndef STREAM_TYPE_H
#define STREAM_TYPE_H

#define PES_STREAM_TYPE_UNKNOWN 0x00
#define PES_STREAM_TYPE_VIDEO   0x01
#define PES_STREAM_TYPE_AUDIO   0x02
#define PES_STREAM_TYPE_PRIVATE 0x03
#define PES_STREAM_TYPE_AC3     0x04
#define PES_STREAM_TYPE_DTS     0x05
#define PES_STREAM_TYPE_LPCM    0x06
#define PES_STREAM_TYPE_AAC     0x07

#define TS_STREAM_TYPE_UNKNOWN   0x00
#define TS_STREAM_TYPE_PAT       0x01
#define TS_STREAM_TYPE_NIT       0x02
#define TS_STREAM_TYPE_PMT       0x03
#define TS_STREAM_TYPE_V11172_2  0x10
#define TS_STREAM_TYPE_V13818_2  0x11
#define TS_STREAM_TYPE_V14496_2  0x12
#define TS_STREAM_TYPE_A11172_3  0x20
#define TS_STREAM_TYPE_A13818_3  0x21
#define TS_STREAM_TYPE_A13818_7  0x22

#endif
