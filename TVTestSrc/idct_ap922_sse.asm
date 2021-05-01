;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; お約束
.586
.mmx
.xmm
.model flat
_TEXT64 segment page public use32 'CODE'
		align 16
;-------------------------------------------------------------------
;IEEE test conditions: -L = -256, +H = 255, sign = 1, #iters = 10000, elapsed = 1262
;Peak absolute values of errors:
;   1   1   1   1   1   1   1   1
;   1   1   1   1   1   1   1   1
;   1   1   1   1   1   1   1   1
;   1   1   1   1   1   1   1   1
;   1   1   1   1   1   1   1   1
;   1   1   1   1   1   1   1   1
;   1   1   1   1   1   1   1   1
;   1   1   1   1   1   1   1   1
;Worst peak error = 1  (meets spec limit 1)
;
;Mean square errors:
;   0.0064   0.0065   0.0060   0.0060   0.0059   0.0060   0.0058   0.0070
;   0.0070   0.0078   0.0073   0.0078   0.0057   0.0098   0.0069   0.0065
;   0.0092   0.0085   0.0073   0.0061   0.0077   0.0097   0.0092   0.0074
;   0.0060   0.0057   0.0058   0.0056   0.0064   0.0071   0.0068   0.0054
;   0.0045   0.0074   0.0061   0.0065   0.0054   0.0059   0.0073   0.0078
;   0.0076   0.0081   0.0063   0.0063   0.0072   0.0080   0.0087   0.0075
;   0.0082   0.0076   0.0065   0.0082   0.0071   0.0073   0.0083   0.0080
;   0.0059   0.0067   0.0056   0.0065   0.0064   0.0063   0.0063   0.0053
;Worst pmse = 0.009800  (meets spec limit 0.06)
;Overall mse = 0.006923  (meets spec limit 0.02)
;
;Mean errors:
;   0.0016  -0.0007   0.0008  -0.0004   0.0005   0.0014   0.0016   0.0006
;  -0.0004  -0.0014   0.0007  -0.0008   0.0019   0.0010   0.0007  -0.0001
;  -0.0020   0.0001  -0.0003  -0.0007  -0.0011  -0.0007   0.0002  -0.0004
;  -0.0020  -0.0001  -0.0006  -0.0002   0.0008  -0.0003  -0.0008   0.0002
;  -0.0003   0.0012  -0.0001   0.0007   0.0002  -0.0001  -0.0005   0.0010
;   0.0008   0.0013   0.0011  -0.0015  -0.0002  -0.0006   0.0005  -0.0005
;  -0.0008  -0.0010   0.0001   0.0014  -0.0001  -0.0001   0.0003   0.0002
;  -0.0005   0.0001   0.0014   0.0019  -0.0008   0.0001  -0.0013  -0.0005
;Worst mean error = 0.002000  (meets spec limit 0.015)
;Overall mean error = 0.000039  (meets spec limit 0.0015)
;
;0 elements of IDCT(0) were not zero
;-------------------------------------------------------------------
; idct_ap922_sse - 方針
; 
; AP-945 idct_m64asm.cpp のコードをコピペ
;-------------------------------------------------------------------
; 定数
one          dq 00001000100010001h ;     1,     1,     1,     1,
half11       dq 00000040000000400h ;         1024,         1024,
half06       dq 00020002000200020h ;    32,    32,    32,    32,
half06a      dq 0001f001f001f001fh ;    31,    31,    31,    31,
tan_1        dq 032ec32ec32ec32ech ; 13036, 13036, 13036, 13036,
tan_2        dq 06a0a6a0a6a0a6a0ah ; 27146, 27146, 27146, 27146,
tan_3        dq 0ab0eab0eab0eab0eh ; 43790, 43790, 43790, 43790,
                                   ;-21746,-21746,-21746,-21746,
cos_4        dq 0b505b505b505b505h ; 46341, 46341, 46341, 46341,
                                   ;-19195,-19195,-19195,-19195,

table_04     dq 022a34000539f4000h ;  8867, 16384, 21407, 16384, w05_w04_w01_w00
             dq 0ac61c00022a34000h ;-21407,-16384,  8867, 16384, w07_w06_w03_w02
             dq 0ac614000dd5d4000h ;-21407, 16384, -8867, 16384, w13_w12_w09_w08
             dq 0dd5d4000539fc000h ; -8867, 16384, 21407,-16384, w15_w14_w11_w10
             dq 0ee584b424b4258c5h ; -4520, 19266, 19266, 22725, w21_w20_w17_w16
             dq 0cdb7a73b11a83249h ;-12873,-22725,  4520, 12873, w23_w22_w19_w18
             dq 0cdb711a8a73b3249h ;-12873,  4520,-22725, 12873, w29_w28_w25_w24
             dq 0a73b4b424b4211a8h ;-22725, 19266, 19266,  4520, w31_w30_w27_w26

table_17     dq 0300b58c573fc58c5h ; 12299, 22725, 29692, 22725, w05_w04_w01_w00
             dq 08c04a73b300b58c5h ;-29692,-22725, 12299, 22725, w07_w06_w03_w02
             dq 08c0458c5cff558c5h ;-29692, 22725,-12299, 22725, w13_w12_w09_w08
             dq 0cff558c573fca73bh ;-12299, 22725, 29692,-22725, w15_w14_w11_w10
             dq 0e782686268627b21h ; -6270, 26722, 26722, 31521, w21_w20_w17_w16
             dq 0ba4184df187e45bfh ;-17855,-31521,  6270, 17855, w23_w22_w19_w18
             dq 0ba41187e84df45bfh ;-17855,  6270,-31521, 17855, w29_w28_w25_w24
             dq 084df68626862187eh ;-31521, 26722, 26722,  6270, w31_w30_w27_w26

table_26     dq 02d41539f6d41539fh ; 11585, 21407, 27969, 21407, w05_w04_w01_w00
             dq 092bfac612d41539fh ;-27969,-21407, 11585, 21407, w07_w06_w03_w02
             dq 092bf539fd2bf539fh ;-27969, 21407,-11585, 21407, w13_w12_w09_w08
             dq 0d2bf539f6d41ac61h ;-11585, 21407, 27969,-21407, w15_w14_w11_w10
             dq 0e8ee6254625473fch ; -5906, 25172, 25172, 29692, w21_w20_w17_w16
             dq 0be4d8c04171241b3h ;-16819,-29692,  5906, 16819, w23_w22_w19_w18
             dq 0be4d17128c0441b3h ;-16819,  5906,-29692, 16819, w29_w28_w25_w24
             dq 08c04625462541712h ;-29692, 25172, 25172,  5906, w31_w30_w27_w26

table_35     dq 028ba4b4262544b42h ; 10426, 19266, 25172, 19266, w05_w04_w01_w00
             dq 09dacb4be28ba4b42h ;-25172,-19266, 10426, 19266, w07_w06_w03_w02
             dq 09dac4b42d7464b42h ;-25172, 19266,-10426, 19266, w13_w12_w09_w08
             dq 0d7464b426254b4beh ;-10426, 19266, 25172,-19266, w15_w14_w11_w10
             dq 0eb3d587e587e6862h ; -5315, 22654, 22654, 26722, w21_w20_w17_w16
             dq 0c4df979e14c33b21h ;-15137,-26722,  5315, 15137, w23_w22_w19_w18
             dq 0c4df14c3979e3b21h ;-15137,  5315,-26722, 15137, w29_w28_w25_w24
             dq 0979e587e587e14c3h ;-26722, 22654, 22654,  5315, w31_w30_w27_w26

;-------------------------------------------------------------------
PUBLIC              C _idct_ap922_sse@4
;      void __stdcall  idct_ap922_sse(
; [esp + 4] = short *block,
; )
_idct_ap922_sse@4 PROC
;-------------------------------------------------------------------
; レジスタ退避
                push       esi
                push       eax
;-------------------------------------------------------------------
; 引数からのデータ受け取り
                mov        esi, [esp+8+4]
;-------------------------------------------------------------------
                ; row 0
                movq       mm0, [esi]            ; x3_x2_x1_x0
                movq       mm1, [esi+8]          ; x7_x6_x5_x4
                lea        eax, table_04
                movq       mm2, mm0              ; x3_x2_x1_x0
                movq       mm3, [eax]            ; w05_w04_w01_w00
                pshufw     mm0, mm0, 10001000b   ; x2_x0_x2_x0
                movq       mm4, [eax+8]          ; w07_w06_w03_w02
                movq       mm5, mm1              ; x7_x6_x5_x4
                pmaddwd    mm3, mm0              ; x2*w05+x0*w04_x1*w01+x0*w00
                movq       mm6, [eax+32]         ; x21_w20_w17_w16
                pshufw     mm1, mm1, 10001000b   ; x6_x4_x6_x4
                pmaddwd    mm4, mm1              ; x6*w07+x4*w06_x6*w03+x4*w02
                movq       mm7, [eax+40]         ; w23_w22_w19_w18
                pshufw     mm2, mm2, 11011101b   ; x3_x1_x3_x1
                pmaddwd    mm6, mm2              ; x3*w21+x1*w20_x3*17+x1*w16
                pshufw     mm5, mm5, 11011101b   ; x7_x5_x7_x5
                pmaddwd    mm7, mm5              ; x7*w23+x5*w22_x7*w19+w5*w18
                paddd      mm3, half11           ; +half
                pmaddwd    mm0, [eax+16]         ; x2*w13+x0*w12_x2*w09+x0*w08
                paddd      mm3, mm4              ; a1_a0
                pmaddwd    mm1, [eax+24]         ; x6*w15+x4*w14_x6*w11+x4*w10
                movq       mm4, mm3              ; a1_a0
                pmaddwd    mm2, [eax+48]         ; x3*w29+x1*w28_x3*w25+x1*w24
                paddd      mm6, mm7              ; b1_b0
                pmaddwd    mm5, [eax+56]         ; x7*w31+x5*w30_x7*w27+x7*w26
                paddd      mm3, mm6              ; a1+b1_a0+b0
                paddd      mm0, half11           ; +half
                psrad      mm3, 11               ; y1_y0
                paddd      mm0, mm1              ; a3_a2
                psubd      mm4, mm6              ; a1-b1_a0-b0
                movq       mm7, mm0              ; a3_a2
                paddd      mm2, mm5              ; b3_b2
                paddd      mm0, mm2              ; a3+b3_a2+b2
                psrad      mm4, 11               ; y6_y7
                psubd      mm7, mm2              ; a3-b3_a2-b2
                psrad      mm0, 11               ; y3_y2
                psrad      mm7, 11               ; y4_y5
                packssdw   mm3, mm0              ; y3_y2_y1_y0
                packssdw   mm7, mm4              ; y6_y7_y4_y5
                pshufw     mm7, mm7, 10110001b   ; y7_y6_y5_y4
                ; row 1
                movq       mm0, [esi+16]
                movq       [esi], mm3
                movq       mm1, [esi+24]
                movq       [esi+8], mm7
                lea        eax, table_17
                movq       mm2, mm0
                movq       mm3, [eax]
                pshufw     mm0, mm0, 10001000b
                movq       mm4, [eax+8]
                movq       mm5, mm1
                pmaddwd    mm3, mm0
                movq       mm6, [eax+32]
                pshufw     mm1, mm1, 10001000b
                pmaddwd    mm4, mm1
                movq       mm7, [eax+40]
                pshufw     mm2, mm2, 11011101b
                pmaddwd    mm6, mm2
                pshufw     mm5, mm5, 11011101b
                pmaddwd    mm7, mm5
                paddd      mm3, half11
                pmaddwd    mm0, [eax+16]
                paddd      mm3, mm4
                pmaddwd    mm1, [eax+24]
                movq       mm4, mm3
                pmaddwd    mm2, [eax+48]
                paddd      mm6, mm7
                pmaddwd    mm5, [eax+56]
                paddd      mm3, mm6
                paddd      mm0, half11
                psrad      mm3, 11
                paddd      mm0, mm1
                psubd      mm4, mm6
                movq       mm7, mm0
                paddd      mm2, mm5
                paddd      mm0, mm2
                psrad      mm4, 11
                psubd      mm7, mm2
                psrad      mm0, 11
                psrad      mm7, 11
                packssdw   mm3, mm0
                packssdw   mm7, mm4
                pshufw     mm7, mm7, 10110001b
                ; row 2
                movq       mm0, [esi+32]
                movq       [esi+16], mm3
                movq       mm1, [esi+40]
                movq       [esi+24], mm7
                lea        eax, table_26
                movq       mm2, mm0
                movq       mm3, [eax]
                pshufw     mm0, mm0, 10001000b
                movq       mm4, [eax+8]
                movq       mm5, mm1
                pmaddwd    mm3, mm0
                movq       mm6, [eax+32]
                pshufw     mm1, mm1, 10001000b
                pmaddwd    mm4, mm1
                movq       mm7, [eax+40]
                pshufw     mm2, mm2, 11011101b
                pmaddwd    mm6, mm2
                pshufw     mm5, mm5, 11011101b
                pmaddwd    mm7, mm5
                paddd      mm3, half11
                pmaddwd    mm0, [eax+16]
                paddd      mm3, mm4
                pmaddwd    mm1, [eax+24]
                movq       mm4, mm3
                pmaddwd    mm2, [eax+48]
                paddd      mm6, mm7
                pmaddwd    mm5, [eax+56]
                paddd      mm3, mm6
                paddd      mm0, half11
                psrad      mm3, 11
                paddd      mm0, mm1
                psubd      mm4, mm6
                movq       mm7, mm0
                paddd      mm2, mm5
                paddd      mm0, mm2
                psrad      mm4, 11
                psubd      mm7, mm2
                psrad      mm0, 11
                psrad      mm7, 11
                packssdw   mm3, mm0
                packssdw   mm7, mm4
                pshufw     mm7, mm7, 10110001b
                ; row 3
                movq       mm0, [esi+48]
                movq       [esi+32], mm3
                movq       mm1, [esi+56]
                movq       [esi+40], mm7
                lea        eax, table_35
                movq       mm2, mm0
                movq       mm3, [eax]
                pshufw     mm0, mm0, 10001000b
                movq       mm4, [eax+8]
                movq       mm5, mm1
                pmaddwd    mm3, mm0
                movq       mm6, [eax+32]
                pshufw     mm1, mm1, 10001000b
                pmaddwd    mm4, mm1
                movq       mm7, [eax+40]
                pshufw     mm2, mm2, 11011101b
                pmaddwd    mm6, mm2
                pshufw     mm5, mm5, 11011101b
                pmaddwd    mm7, mm5
                paddd      mm3, half11
                pmaddwd    mm0, [eax+16]
                paddd      mm3, mm4
                pmaddwd    mm1, [eax+24]
                movq       mm4, mm3
                pmaddwd    mm2, [eax+48]
                paddd      mm6, mm7
                pmaddwd    mm5, [eax+56]
                paddd      mm3, mm6
                paddd      mm0, half11
                psrad      mm3, 11
                paddd      mm0, mm1
                psubd      mm4, mm6
                movq       mm7, mm0
                paddd      mm2, mm5
                paddd      mm0, mm2
                psrad      mm4, 11
                psubd      mm7, mm2
                psrad      mm0, 11
                psrad      mm7, 11
                packssdw   mm3, mm0
                packssdw   mm7, mm4
                pshufw     mm7, mm7, 10110001b
                ; row 4
                movq       mm0, [esi+64]
                movq       [esi+48], mm3
                movq       mm1, [esi+72]
                movq       [esi+56], mm7
                lea        eax, table_04
                movq       mm2, mm0
                movq       mm3, [eax]
                pshufw     mm0, mm0, 10001000b
                movq       mm4, [eax+8]
                movq       mm5, mm1
                pmaddwd    mm3, mm0
                movq       mm6, [eax+32]
                pshufw     mm1, mm1, 10001000b
                pmaddwd    mm4, mm1
                movq       mm7, [eax+40]
                pshufw     mm2, mm2, 11011101b
                pmaddwd    mm6, mm2
                pshufw     mm5, mm5, 11011101b
                pmaddwd    mm7, mm5
                paddd      mm3, half11
                pmaddwd    mm0, [eax+16]
                paddd      mm3, mm4
                pmaddwd    mm1, [eax+24]
                movq       mm4, mm3
                pmaddwd    mm2, [eax+48]
                paddd      mm6, mm7
                pmaddwd    mm5, [eax+56]
                paddd      mm3, mm6
                paddd      mm0, half11
                psrad      mm3, 11
                paddd      mm0, mm1
                psubd      mm4, mm6
                movq       mm7, mm0
                paddd      mm2, mm5
                paddd      mm0, mm2
                psrad      mm4, 11
                psubd      mm7, mm2
                psrad      mm0, 11
                psrad      mm7, 11
                packssdw   mm3, mm0
                packssdw   mm7, mm4
                pshufw     mm7, mm7, 10110001b
                ; row 5
                movq       mm0, [esi+80]
                movq       [esi+64], mm3
                movq       mm1, [esi+88]
                movq       [esi+72], mm7
                lea        eax, table_35
                movq       mm2, mm0
                movq       mm3, [eax]
                pshufw     mm0, mm0, 10001000b
                movq       mm4, [eax+8]
                movq       mm5, mm1
                pmaddwd    mm3, mm0
                movq       mm6, [eax+32]
                pshufw     mm1, mm1, 10001000b
                pmaddwd    mm4, mm1
                movq       mm7, [eax+40]
                pshufw     mm2, mm2, 11011101b
                pmaddwd    mm6, mm2
                pshufw     mm5, mm5, 11011101b
                pmaddwd    mm7, mm5
                paddd      mm3, half11
                pmaddwd    mm0, [eax+16]
                paddd      mm3, mm4
                pmaddwd    mm1, [eax+24]
                movq       mm4, mm3
                pmaddwd    mm2, [eax+48]
                paddd      mm6, mm7
                pmaddwd    mm5, [eax+56]
                paddd      mm3, mm6
                paddd      mm0, half11
                psrad      mm3, 11
                paddd      mm0, mm1
                psubd      mm4, mm6
                movq       mm7, mm0
                paddd      mm2, mm5
                paddd      mm0, mm2
                psrad      mm4, 11
                psubd      mm7, mm2
                psrad      mm0, 11
                psrad      mm7, 11
                packssdw   mm3, mm0
                packssdw   mm7, mm4
                pshufw     mm7, mm7, 10110001b
                ; row 6
                movq       mm0, [esi+96]
                movq       [esi+80], mm3
                movq       mm1, [esi+104]
                movq       [esi+88], mm7
                lea        eax, table_26
                movq       mm2, mm0
                movq       mm3, [eax]
                pshufw     mm0, mm0, 10001000b
                movq       mm4, [eax+8]
                movq       mm5, mm1
                pmaddwd    mm3, mm0
                movq       mm6, [eax+32]
                pshufw     mm1, mm1, 10001000b
                pmaddwd    mm4, mm1
                movq       mm7, [eax+40]
                pshufw     mm2, mm2, 11011101b
                pmaddwd    mm6, mm2
                pshufw     mm5, mm5, 11011101b
                pmaddwd    mm7, mm5
                paddd      mm3, half11
                pmaddwd    mm0, [eax+16]
                paddd      mm3, mm4
                pmaddwd    mm1, [eax+24]
                movq       mm4, mm3
                pmaddwd    mm2, [eax+48]
                paddd      mm6, mm7
                pmaddwd    mm5, [eax+56]
                paddd      mm3, mm6
                paddd      mm0, half11
                psrad      mm3, 11
                paddd      mm0, mm1
                psubd      mm4, mm6
                movq       mm7, mm0
                paddd      mm2, mm5
                paddd      mm0, mm2
                psrad      mm4, 11
                psubd      mm7, mm2
                psrad      mm0, 11
                psrad      mm7, 11
                packssdw   mm3, mm0
                packssdw   mm7, mm4
                pshufw     mm7, mm7, 10110001b
                ; row 7
                movq       mm0, [esi+112]
                movq       [esi+96], mm3
                movq       mm1, [esi+120]
                movq       [esi+104], mm7
                lea        eax, table_17
                movq       mm2, mm0
                movq       mm3, [eax]
                pshufw     mm0, mm0, 10001000b
                movq       mm4, [eax+8]
                movq       mm5, mm1
                pmaddwd    mm3, mm0
                movq       mm6, [eax+32]
                pshufw     mm1, mm1, 10001000b
                pmaddwd    mm4, mm1
                movq       mm7, [eax+40]
                pshufw     mm2, mm2, 11011101b
                pmaddwd    mm6, mm2
                pshufw     mm5, mm5, 11011101b
                pmaddwd    mm7, mm5
                paddd      mm3, half11
                pmaddwd    mm0, [eax+16]
                paddd      mm3, mm4
                pmaddwd    mm1, [eax+24]
                movq       mm4, mm3
                pmaddwd    mm2, [eax+48]
                paddd      mm6, mm7
                pmaddwd    mm5, [eax+56]
                paddd      mm3, mm6
                paddd      mm0, half11
                psrad      mm3, 11
                paddd      mm0, mm1
                psubd      mm4, mm6
                movq       mm7, mm0
                paddd      mm2, mm5
                paddd      mm0, mm2
                psrad      mm4, 11
                psubd      mm7, mm2
                psrad      mm0, 11
                psrad      mm7, 11
                packssdw   mm3, mm0
                packssdw   mm7, mm4
                pshufw     mm7, mm7, 10110001b
                movq       [esi+112], mm3
                movq       mm0, [esi+5*16] ; x5
                movq       [esi+120], mm7
                ; col 0-3
                movq       mm1, tan_3      ; tan_3
                movq       mm2, mm0        ; x5
                movq       mm3, [esi+3*16] ; x3
                pmulhw     mm0, mm1        ; x5 * tan_3
                movq       mm4, [esi+7*16] ; x7
                pmulhw     mm1, mm3        ; x3 * tan_3
                movq       mm5, tan_1      ; tan_1
                movq       mm6, mm4        ; x7
                pmulhw     mm4, mm5        ; x7 * tan_1
                paddsw     mm0, mm2        ; x5 * tan_3 
                pmulhw     mm5, [esi+1*16] ; x1 * tan_1
                paddsw     mm1, mm3        ; x3 * tan_3
                movq       mm7, [esi+6*16] ; x6
                paddsw     mm0, mm3        ; x5 * tan_3 + x3 : tm765 
                movq       mm3, tan_2      ; tan_2
                psubsw     mm2, mm1        ; x5 - x3 * tan_3 : tm465
                pmulhw     mm7, mm3        ; x6 * tan_2
                movq       mm1, mm0        ; tm765
                pmulhw     mm3, [esi+2*16] ; x2 * tan_2
                psubsw     mm5, mm6        ; x1 * tan_1 - x7 : tp465
                paddsw     mm4, [esi+1*16] ; x1 + x7 * tan_1 : tp765
                paddsw     mm0, mm4        ; tp765 + tm765   : t7
                paddsw     mm0, one        ; t7 + round
                psubsw     mm4, mm1        ; tp765 - tm765   : tp65 
                paddsw     mm7, [esi+2*16] ; x2 + x6 * tan_2 : tm03
                movq       mm6, mm5        ; tp465
                psubsw     mm3, [esi+6*16] ; x2 * tan_2 - x6 : tm12
                psubsw     mm5, mm2        ; tp465 - tm465   : tm65
                paddsw     mm5, one        ; tm65 + round
                paddsw     mm6, mm2        ; tp465 + tm465   : t4
                movq       [esi+7*16], mm0 ; temporal save t7
                movq       mm1, mm4        ; tp65
                movq       mm2, cos_4      ; cos_4
                paddsw     mm4, mm5        ; tp65 + tm65
                movq       mm0, cos_4      ; cos_4
                pmulhw     mm2, mm4        ; (tp65 + tm65) * cos_4
                movq       [esi+3*16], mm6 ; temporal save t4
                psubsw     mm1, mm5        ; tp65 - tm65
                movq       mm6, [esi]      ; x0
                pmulhw     mm0, mm1        ; (tp65 - tm65) * cos_4
                movq       mm5, [esi+4*16] ; x4
                paddsw     mm4, mm2        ; (tp65 + tm65) * cos_4 : t6
                por        mm4, one        ; t6 |= round
                paddsw     mm5, mm6        ; x0 + x4 : tp03      
                psubsw     mm6, [esi+4*16] ; x0 - x4 : tp12
                paddsw     mm0, mm1        ; (tp65 - tm65) * cos_4 : t5
                por        mm0, one        ; t5 + round
                movq       mm2, mm5        ; tp03
                paddsw     mm5, mm7        ; tp03 + tm03 : t0
                movq       mm1, mm6        ; x6
                paddsw     mm5, half06     ; t0 + half
                psubsw     mm2, mm7        ; tp03 - tm03 : t3
                movq       mm7, [esi+7*16] ; t7
                paddsw     mm6, mm3        ; tp12 + tm12 : t1 
                paddsw     mm6, half06     ; t1 + half
                paddsw     mm7, mm5        ; t7 + (t0 + half)
                psraw      mm7, 6          ; (t0 + t7 + half) >> 6 : y0
                psubsw     mm1, mm3        ; tp12 - tm12 : t2
                paddsw     mm2, half06a    ; t3 + half
                movq       mm3, mm6        ; t1 + half
                paddsw     mm1, half06a    ; t2 + half
                paddsw     mm6, mm4        ; (t1 + half) + t6
                movq       [esi], mm7      ; save y0
                psraw      mm6, 6          ; (t1 + t6 + half) >> 6 : y1 
                movq       mm7, mm1        ; t2 + half
                paddsw     mm1, mm0        ; t2 + half + t5
                movq       [esi+1*16], mm6 ; save y1
                psraw      mm1, 6          ; (t2 + half + t5) >> 6 : y2
                movq       mm6, [esi+3*16] ; t4
                psubsw     mm7, mm0        ; t2 + half - t5
                paddsw     mm6, mm2        ; t4 + t3 + half
                psubsw     mm2, [esi+3*16] ; t3 + half - t4
                psraw      mm7, 6          ; (t2 + half - t5) >> 6 ; y5
                movq       [esi+2*16], mm1 ; save y2
                psraw      mm6, 6          ; (t3 + half + t4) >> 6 ; y3
                psubsw     mm5, [esi+7*16] ; t0 + half - t7
                psraw      mm2, 6          ; (t3 + half - t4) >> 6 ; y4
                movq       [esi+3*16], mm6 ; save y3
                psubsw     mm3, mm4        ; t1 + half - t6
                movq       [esi+4*16], mm2 ; save y4
                psraw      mm3, 6          ; (t1 + half - t6) >> 6 ; y6
                movq       [esi+5*16], mm7 ; save y5
                psraw      mm5, 6          ; (t0 + half - t7) >> 6 ; y7
                movq       [esi+6*16], mm3 ; save y6
                movq       [esi+7*16], mm5 ; save y7
                ; col 4-7
                movq       mm0, [esi+5*16+8] ; x5
                add        esi, 8 
                movq       mm1, tan_3      ; tan_3
                movq       mm2, mm0        ; x5
                movq       mm3, [esi+3*16] ; x3
                pmulhw     mm0, mm1        ; x5 * tan_3
                movq       mm4, [esi+7*16] ; x7
                pmulhw     mm1, mm3        ; x3 * tan_3
                movq       mm5, tan_1      ; tan_1
                movq       mm6, mm4        ; x7
                pmulhw     mm4, mm5        ; x7 * tan_1
                paddsw     mm0, mm2        ; x5 * tan_3 
                pmulhw     mm5, [esi+1*16] ; x1 * tan_1
                paddsw     mm1, mm3        ; x3 * tan_3
                movq       mm7, [esi+6*16] ; x6
                paddsw     mm0, mm3        ; x5 * tan_3 + x3 : tm765 
                movq       mm3, tan_2      ; tan_2
                psubsw     mm2, mm1        ; x5 - x3 * tan_3 : tm465
                pmulhw     mm7, mm3        ; x6 * tan_2
                movq       mm1, mm0        ; tm765
                pmulhw     mm3, [esi+2*16] ; x2 * tan_2
                psubsw     mm5, mm6        ; x1 * tan_1 - x7 : tp465
                paddsw     mm4, [esi+1*16] ; x1 + x7 * tan_1 : tp765
                paddsw     mm0, mm4        ; tp765 + tm765   : t7
                paddsw     mm0, one        ; t7 + round
                psubsw     mm4, mm1        ; tp765 - tm765   : tp65 
                paddsw     mm7, [esi+2*16] ; x2 + x6 * tan_2 : tm03
                movq       mm6, mm5        ; tp465
                psubsw     mm3, [esi+6*16] ; x2 * tan_2 - x6 : tm12
                psubsw     mm5, mm2        ; tp465 - tm465   : tm65
                paddsw     mm5, one        ; tm65 + round
                paddsw     mm6, mm2        ; tp465 + tm465   : t4
                movq       [esi+7*16], mm0 ; temporal save t7
                movq       mm1, mm4        ; tp65
                movq       mm2, cos_4      ; cos_4
                paddsw     mm4, mm5        ; tp65 + tm65
                movq       mm0, cos_4      ; cos_4
                pmulhw     mm2, mm4        ; (tp65 + tm65) * cos_4
                movq       [esi+3*16], mm6 ; temporal save t4
                psubsw     mm1, mm5        ; tp65 - tm65
                movq       mm6, [esi]      ; x0
                pmulhw     mm0, mm1        ; (tp65 - tm65) * cos_4
                movq       mm5, [esi+4*16] ; x4
                paddsw     mm4, mm2        ; (tp65 + tm65) * cos_4 : t6
                por        mm4, one        ; t6 |= round
                paddsw     mm5, mm6        ; x0 + x4 : tp03      
                psubsw     mm6, [esi+4*16] ; x0 - x4 : tp12
                paddsw     mm0, mm1        ; (tp65 - tm65) * cos_4 : t5
                por        mm0, one        ; t5 |= round
                movq       mm2, mm5        ; tp03
                paddsw     mm5, mm7        ; tp03 + tm03 : t0
                movq       mm1, mm6        ; x6
                paddsw     mm5, half06     ; t0 + half
                psubsw     mm2, mm7        ; tp03 - tm03 : t3
                movq       mm7, [esi+7*16] ; t7
                paddsw     mm6, mm3        ; tp12 + tm12 : t1 
                paddsw     mm6, half06     ; t1 + half
                paddsw     mm7, mm5        ; t7 + (t0 + half)
                psraw      mm7, 6          ; (t0 + t7 + half) >> 6 : y0
                psubsw     mm1, mm3        ; tp12 - tm12 : t2
                paddsw     mm2, half06a    ; t3 + half
                movq       mm3, mm6        ; t1 + half
                paddsw     mm1, half06a    ; t2 + half
                paddsw     mm6, mm4        ; (t1 + half) + t6
                movq       [esi], mm7      ; save y0
                psraw      mm6, 6          ; (t1 + t6 + half) >> 6 : y1 
                movq       mm7, mm1        ; t2 + half
                paddsw     mm1, mm0        ; t2 + half + t5
                movq       [esi+1*16], mm6 ; save y1
                psraw      mm1, 6          ; (t2 + half + t5) >> 6 : y2
                movq       mm6, [esi+3*16] ; t4
                psubsw     mm7, mm0        ; t2 + half - t5
                paddsw     mm6, mm2        ; t4 + t3 + half
                psubsw     mm2, [esi+3*16] ; t3 + half - t4
                psraw      mm7, 6          ; (t2 + half - t5) >> 6 ; y5
                movq       [esi+2*16], mm1 ; save y2
                psraw      mm6, 6          ; (t3 + half + t4) >> 6 ; y3
                psubsw     mm5, [esi+7*16] ; t0 + half - t7
                psraw      mm2, 6          ; (t3 + half - t4) >> 6 ; y4
                movq       [esi+3*16], mm6 ; save y3
                psubsw     mm3, mm4        ; t1 + half - t6
                movq       [esi+4*16], mm2 ; save y4
                psraw      mm3, 6          ; (t1 + half - t6) >> 6 ; y6
                movq       [esi+5*16], mm7 ; save y5
                psraw      mm5, 6          ; (t0 + half - t7) >> 6 ; y7
                movq       [esi+6*16], mm3 ; save y6
                movq       [esi+7*16], mm5 ; save y7
;-------------------------------------------------------------------
; 後始末
                pop        eax
                pop        esi

                ret        4
;-------------------------------------------------------------------
_idct_ap922_sse@4 ENDP
;-------------------------------------------------------------------
; 終了

END
