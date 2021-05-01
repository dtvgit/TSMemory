/*******************************************************************
                 0x00, 0x00, 0x01 related interface
 *******************************************************************/
#ifndef ZERO_ZERO_ONE_H
#define ZERO_ZERO_ONE_H

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char *find_next_001(unsigned char *p, unsigned char *last);
extern unsigned char *find_prev_001(unsigned char *p, unsigned char *head);

#ifdef __cplusplus
}
#endif

#endif
