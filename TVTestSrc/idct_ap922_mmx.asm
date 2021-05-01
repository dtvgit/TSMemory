;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; お約束
.586
.mmx
.xmm
.model flat
_TEXT64 segment page public use32 'CODE'
		align 16
;-------------------------------------------------------------------
;IEEE test conditions: -L = -256, +H = 255, sign = 1, #iters = 10000, elapsed = 1242
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
; idct_ap922_mmx - 方針
; 
; AP-945 idct_m64asm.cpp のコードを pshufw 除去して MMX 化
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

table_04     dq 0c000400040004000h ;-16384, 16384, 16384, 16384, w06_w04_w02_w00
             dq 0ac6122a322a3539fh ;-21407,  8867,  8867, 21407, w07_w05_w03_w01
             dq 040004000c0004000h ; 16384, 16384,-16384, 16384, w14_w12_w10_w08
             dq 0dd5dac61539fdd5dh ; -8867,-21407, 21407, -8867, w15_w13_w11_w09
             dq 0a73b4b42324958c5h ;-22725, 19266, 12873, 22725, w22_w20_w18_w16
             dq 0cdb7ee5811a84b42h ;-12873, -4520,  4520, 19266, w23_w21_w19_w17
             dq 04b4211a811a83249h ; 19266,  4520,  4520, 12873, w30_w28_w26_w24
             dq 0a73bcdb74b42a73bh ;-22725,-12873, 19266,-22725, w31_w29_w27_w25

table_17     dq 0a73b58c558c558c5h ;-22725, 22725, 22725, 22725, w06_w04_w02_w00 
             dq 08c04300b300b73fch ;-29692, 12299, 12299, 29692, w07_w05_w03_w01 
             dq 058c558c5a73b58c5h ; 22725, 22725,-22725, 22725, w14_w12_w10_w08 
             dq 0cff58c0473fccff5h ;-12299,-29692, 29692,-12299, w15_w13_w11_w09 
             dq 084df686245bf7b21h ;-31521, 26722, 17855, 31521, w22_w20_w18_w16 
             dq 0ba41e782187e6862h ;-17855, -6270,  6270, 26722, w23_w21_w19_w17 
             dq 06862187e187e45bfh ; 26722,  6270,  6270, 17855, w30_w28_w26_w24 
             dq 084dfba41686284dfh ;-31521,-17855, 26722,-31521, w31_w29_w27_w25 

table_26     dq 0ac61539f539f539fh ;-21407, 21407, 21407, 21407, w06_w04_w02_w00
             dq 092bf2d412d416d41h ;-27969, 11585, 11585, 27969, w07_w05_w03_w01
             dq 0539f539fac61539fh ; 21407, 21407,-21407, 21407, w14_w12_w10_w08
             dq 0d2bf92bf6d41d2bfh ;-11585,-27969, 27969,-11585, w15_w13_w11_w09
             dq 08c04625441b373fch ;-29692, 25172, 16819, 29692, w22_w20_w18_w16
             dq 0be4de8ee17126254h ;-16819, -5906,  5906, 25172, w23_w21_w19_w17
             dq 062541712171241b3h ; 25172,  5906,  5906, 16819, w30_w28_w26_w24
             dq 08c04be4d62548c04h ;-29692,-16819, 25172,-29692, w31_w29_w27_w25

table_35     dq 0b4be4b424b424b42h ;-19266, 19266, 19266, 19266, w06_w04_w02_w00
             dq 09dac28ba28ba6254h ;-25172, 10426, 10426, 25172, w07_w05_w03_w01
             dq 04b424b42b4be4b42h ; 19266, 19266,-19266, 19266, w14_w12_w10_w08
             dq 0d7469dac6254d746h ;-10426,-25172, 25172,-10426, w15_w13_w11_w09
             dq 0979e587e3b216862h ;-26722, 22654, 15137, 26722, w22_w20_w18_w16
             dq 0c4dfeb3d14c3587eh ;-15137, -5315,  5315, 22654, w23_w21_w19_w17
             dq 0587e14c314c33b21h ; 22654,  5315,  5315, 15137, w30_w28_w26_w24
             dq 0979ec4df587e979eh ;-26722,-15137, 22654,-26722, w31_w29_w27_w25

;-------------------------------------------------------------------
PUBLIC              C _idct_ap922_mmx@4
;      void __stdcall  idct_ap922_mmx(
; [esp + 4] = short *block,
; )
_idct_ap922_mmx@4 PROC
;-------------------------------------------------------------------
; レジスタ退避
                push       esi
                push       eax
;-------------------------------------------------------------------
; 引数からのデータ受け取り
                mov        esi, [esp+8+4]
;-------------------------------------------------------------------
            ; 行処理
                lea        eax, table_04 
            ; row 0
                movd       mm0, dword ptr [esi]     ; 00_00_x1_x0
                movd       mm1, dword ptr [esi+4]   ; 00_00_x3_x2
                movd       mm2, dword ptr [esi+8]   ; 00_00_x5_x4
                movd       mm3, dword ptr [esi+12]  ; 00_00_x7_x4
                movq       mm4, qword ptr [eax+16]  ; w14_w12_w10_w08
                movq       mm5, qword ptr [eax+24]  ; w15_w13_w11_w09
                movq       mm6, qword ptr [eax+48]  ; w30_w28_w26_w24
                movq       mm7, qword ptr [eax+56]  ; w31_w29_w27_w25
                punpcklwd  mm0, mm2       ; x5_x1_x4_x0
                punpcklwd  mm1, mm3       ; x7_x3_x6_x2
                movq       mm2, mm0
                movq       mm3, mm1
                punpckldq  mm0, mm0       ; x4_x0_x4_x0
                punpckhdq  mm2, mm2       ; x5_x1_x5_x1
                punpckldq  mm1, mm1       ; x6_x2_x6_x2
                punpckhdq  mm3, mm3       ; x7_x3_x7_x3
                pmaddwd    mm4, mm0       ; x4*w14+x0*w12_x4*w10+x0*w08
                pmaddwd    mm5, mm1       ; x6*w15+x2*w13_x6*w11+x2*w09
                pmaddwd    mm6, mm2       ; x5*w30+x1*w28_x5*w26+x1*w24
                pmaddwd    mm7, mm3       ; x7*w31+x3*x29_x7*w27+x3*w25
                pmaddwd    mm0, [eax]     ; x4*w06+x0*w04_x4*w02+x0*w00
                pmaddwd    mm1, [eax+8]   ; x6*w07+x0*w05_x6*w03+x0*w01
                pmaddwd    mm2, [eax+32]  ; x5*w22+x1*w20_x5*w18+x1*w16
                pmaddwd    mm3, [eax+40]  ; x7*w23+x3*w21_x7*w19+x3*w17
                paddd      mm4, mm5       ; a3_a2
                paddd      mm6, mm7       ; b3_b2
                paddd      mm0, mm1       ; a1_a0
                paddd      mm2, mm3       ; b1_b0
                paddd      mm4, half11
                paddd      mm0, half11
                movq       mm5, mm6
                movq       mm1, mm2
                paddd      mm2, mm0       ; a1+b1_a0+b0
                paddd      mm6, mm4       ; a3+b3_a2+b2
                psubd      mm0, mm1       ; a1-b1_a0-b0
                psubd      mm4, mm5       ; a3-b3_a2-b2
                psrad      mm2, 11        ; y1_y0
                psrad      mm6, 11        ; y3_y2
                psrad      mm0, 11        ; y6_y7
                psrad      mm4, 11        ; y4_y5
                packssdw   mm2, mm6       ; y3_y2_y1_y0
                packssdw   mm4, mm0       ; y6_y7_y4_y5
                movq       [esi], mm2     ;
                movq       mm7, mm4       ; 
                psrld      mm4, 16        ; 00_y6_00_y4
                pslld      mm7, 16        ; y7_00_y5_00
                por        mm4, mm7       ; y7_y6_y5_y4
                movq       [esi+8], mm4   ;
            ; row 4
                movd       mm0, dword ptr [esi+64]  ; 00_00_x1_x0
                movd       mm1, dword ptr [esi+68]  ; 00_00_x3_x2
                movd       mm2, dword ptr [esi+72]  ; 00_00_x5_x4
                movd       mm3, dword ptr [esi+76]  ; 00_00_x7_x4
                movq       mm4, qword ptr [eax+16]  ; w14_w12_w10_w08
                movq       mm5, qword ptr [eax+24]  ; w15_w13_w11_w09
                movq       mm6, qword ptr [eax+48]  ; w30_w28_w26_w24
                movq       mm7, qword ptr [eax+56]  ; w31_w29_w27_w25
                punpcklwd  mm0, mm2       ; x5_x1_x4_x0
                punpcklwd  mm1, mm3       ; x7_x3_x6_x2
                movq       mm2, mm0
                movq       mm3, mm1
                punpckldq  mm0, mm0       ; x4_x0_x4_x0
                punpckhdq  mm2, mm2       ; x5_x1_x5_x1
                punpckldq  mm1, mm1       ; x6_x2_x6_x2
                punpckhdq  mm3, mm3       ; x7_x3_x7_x3
                pmaddwd    mm4, mm0       ; x4*w14+x0*w12_x4*w10+x0*w08
                pmaddwd    mm5, mm1       ; x6*w15+x2*w13_x6*w11+x2*w09
                pmaddwd    mm6, mm2       ; x5*w30+x1*w28_x5*w26+x1*w24
                pmaddwd    mm7, mm3       ; x7*w31+x3*x29_x7*w27+x3*w25
                pmaddwd    mm0, [eax]     ; x4*w06+x0*w04_x4*w02+x0*w00
                pmaddwd    mm1, [eax+8]   ; x6*w07+x0*w05_x6*w03+x0*w01
                pmaddwd    mm2, [eax+32]  ; x5*w22+x1*w20_x5*w18+x1*w16
                pmaddwd    mm3, [eax+40]  ; x7*w23+x3*w21_x7*w19+x3*w17
                paddd      mm4, mm5       ; a3_a2
                paddd      mm6, mm7       ; b3_b2
                paddd      mm0, mm1       ; a1_a0
                paddd      mm2, mm3       ; b1_b0
                paddd      mm4, half11
                paddd      mm0, half11
                movq       mm5, mm6
                movq       mm1, mm2
                paddd      mm2, mm0       ; a1+b1_a0+b0
                paddd      mm6, mm4       ; a3+b3_a2+b2
                psubd      mm0, mm1       ; a1-b1_a0-b0
                psubd      mm4, mm5       ; a3-b3_a2-b2
                psrad      mm2, 11        ; y1_y0
                psrad      mm6, 11        ; y3_y2
                psrad      mm0, 11        ; y6_y7
                psrad      mm4, 11        ; y4_y5
                packssdw   mm2, mm6       ; y3_y2_y1_y0
                packssdw   mm4, mm0       ; y6_y7_y4_y5
                movq       [esi+64], mm2  ;
                movq       mm7, mm4       ; 
                psrld      mm4, 16        ; 00_y6_00_y4
                pslld      mm7, 16        ; y7_00_y5_00
                por        mm4, mm7       ; y7_y6_y5_y4
                movq       [esi+72], mm4  ;
            ; row 1
                lea        eax, table_17
                movd       mm0, dword ptr [esi+16]  ; 00_00_x1_x0
                movd       mm1, dword ptr [esi+20]  ; 00_00_x3_x2
                movd       mm2, dword ptr [esi+24]  ; 00_00_x5_x4
                movd       mm3, dword ptr [esi+28]  ; 00_00_x7_x4
                movq       mm4, qword ptr [eax+16]  ; w14_w12_w10_w08
                movq       mm5, qword ptr [eax+24]  ; w15_w13_w11_w09
                movq       mm6, qword ptr [eax+48]  ; w30_w28_w26_w24
                movq       mm7, qword ptr [eax+56]  ; w31_w29_w27_w25
                punpcklwd  mm0, mm2       ; x5_x1_x4_x0
                punpcklwd  mm1, mm3       ; x7_x3_x6_x2
                movq       mm2, mm0
                movq       mm3, mm1
                punpckldq  mm0, mm0       ; x4_x0_x4_x0
                punpckhdq  mm2, mm2       ; x5_x1_x5_x1
                punpckldq  mm1, mm1       ; x6_x2_x6_x2
                punpckhdq  mm3, mm3       ; x7_x3_x7_x3
                pmaddwd    mm4, mm0       ; x4*w14+x0*w12_x4*w10+x0*w08
                pmaddwd    mm5, mm1       ; x6*w15+x2*w13_x6*w11+x2*w09
                pmaddwd    mm6, mm2       ; x5*w30+x1*w28_x5*w26+x1*w24
                pmaddwd    mm7, mm3       ; x7*w31+x3*x29_x7*w27+x3*w25
                pmaddwd    mm0, [eax]     ; x4*w06+x0*w04_x4*w02+x0*w00
                pmaddwd    mm1, [eax+8]   ; x6*w07+x0*w05_x6*w03+x0*w01
                pmaddwd    mm2, [eax+32]  ; x5*w22+x1*w20_x5*w18+x1*w16
                pmaddwd    mm3, [eax+40]  ; x7*w23+x3*w21_x7*w19+x3*w17
                paddd      mm4, mm5       ; a3_a2
                paddd      mm6, mm7       ; b3_b2
                paddd      mm0, mm1       ; a1_a0
                paddd      mm2, mm3       ; b1_b0
                paddd      mm4, half11
                paddd      mm0, half11
                movq       mm5, mm6
                movq       mm1, mm2
                paddd      mm2, mm0       ; a1+b1_a0+b0
                paddd      mm6, mm4       ; a3+b3_a2+b2
                psubd      mm0, mm1       ; a1-b1_a0-b0
                psubd      mm4, mm5       ; a3-b3_a2-b2
                psrad      mm2, 11        ; y1_y0
                psrad      mm6, 11        ; y3_y2
                psrad      mm0, 11        ; y6_y7
                psrad      mm4, 11        ; y4_y5
                packssdw   mm2, mm6       ; y3_y2_y1_y0
                packssdw   mm4, mm0       ; y6_y7_y4_y5
                movq       [esi+16], mm2  ;
                movq       mm7, mm4       ; 
                psrld      mm4, 16        ; 00_y6_00_y4
                pslld      mm7, 16        ; y7_00_y5_00
                por        mm4, mm7       ; y7_y6_y5_y4
                movq       [esi+24], mm4  ;
            ; row 7
                movd       mm0, dword ptr [esi+112] ; 00_00_x1_x0
                movd       mm1, dword ptr [esi+116] ; 00_00_x3_x2
                movd       mm2, dword ptr [esi+120] ; 00_00_x5_x4
                movd       mm3, dword ptr [esi+124] ; 00_00_x7_x4
                movq       mm4, qword ptr [eax+16]  ; w14_w12_w10_w08
                movq       mm5, qword ptr [eax+24]  ; w15_w13_w11_w09
                movq       mm6, qword ptr [eax+48]  ; w30_w28_w26_w24
                movq       mm7, qword ptr [eax+56]  ; w31_w29_w27_w25
                punpcklwd  mm0, mm2       ; x5_x1_x4_x0
                punpcklwd  mm1, mm3       ; x7_x3_x6_x2
                movq       mm2, mm0
                movq       mm3, mm1
                punpckldq  mm0, mm0       ; x4_x0_x4_x0
                punpckhdq  mm2, mm2       ; x5_x1_x5_x1
                punpckldq  mm1, mm1       ; x6_x2_x6_x2
                punpckhdq  mm3, mm3       ; x7_x3_x7_x3
                pmaddwd    mm4, mm0       ; x4*w14+x0*w12_x4*w10+x0*w08
                pmaddwd    mm5, mm1       ; x6*w15+x2*w13_x6*w11+x2*w09
                pmaddwd    mm6, mm2       ; x5*w30+x1*w28_x5*w26+x1*w24
                pmaddwd    mm7, mm3       ; x7*w31+x3*x29_x7*w27+x3*w25
                pmaddwd    mm0, [eax]     ; x4*w06+x0*w04_x4*w02+x0*w00
                pmaddwd    mm1, [eax+8]   ; x6*w07+x0*w05_x6*w03+x0*w01
                pmaddwd    mm2, [eax+32]  ; x5*w22+x1*w20_x5*w18+x1*w16
                pmaddwd    mm3, [eax+40]  ; x7*w23+x3*w21_x7*w19+x3*w17
                paddd      mm4, mm5       ; a3_a2
                paddd      mm6, mm7       ; b3_b2
                paddd      mm0, mm1       ; a1_a0
                paddd      mm2, mm3       ; b1_b0
                paddd      mm4, half11
                paddd      mm0, half11
                movq       mm5, mm6
                movq       mm1, mm2
                paddd      mm2, mm0       ; a1+b1_a0+b0
                paddd      mm6, mm4       ; a3+b3_a2+b2
                psubd      mm0, mm1       ; a1-b1_a0-b0
                psubd      mm4, mm5       ; a3-b3_a2-b2
                psrad      mm2, 11        ; y1_y0
                psrad      mm6, 11        ; y3_y2
                psrad      mm0, 11        ; y6_y7
                psrad      mm4, 11        ; y4_y5
                packssdw   mm2, mm6       ; y3_y2_y1_y0
                packssdw   mm4, mm0       ; y6_y7_y4_y5
                movq       [esi+112], mm2 ;
                movq       mm7, mm4       ; 
                psrld      mm4, 16        ; 00_y6_00_y4
                pslld      mm7, 16        ; y7_00_y5_00
                por        mm4, mm7       ; y7_y6_y5_y4
                movq       [esi+120], mm4 ;
            ; row 2
                lea        eax, table_26
                movd       mm0, dword ptr [esi+32]  ; 00_00_x1_x0
                movd       mm1, dword ptr [esi+36]  ; 00_00_x3_x2
                movd       mm2, dword ptr [esi+40]  ; 00_00_x5_x4
                movd       mm3, dword ptr [esi+44]  ; 00_00_x7_x4
                movq       mm4, qword ptr [eax+16]  ; w14_w12_w10_w08
                movq       mm5, qword ptr [eax+24]  ; w15_w13_w11_w09
                movq       mm6, qword ptr [eax+48]  ; w30_w28_w26_w24
                movq       mm7, qword ptr [eax+56]  ; w31_w29_w27_w25
                punpcklwd  mm0, mm2       ; x5_x1_x4_x0
                punpcklwd  mm1, mm3       ; x7_x3_x6_x2
                movq       mm2, mm0
                movq       mm3, mm1
                punpckldq  mm0, mm0       ; x4_x0_x4_x0
                punpckhdq  mm2, mm2       ; x5_x1_x5_x1
                punpckldq  mm1, mm1       ; x6_x2_x6_x2
                punpckhdq  mm3, mm3       ; x7_x3_x7_x3
                pmaddwd    mm4, mm0       ; x4*w14+x0*w12_x4*w10+x0*w08
                pmaddwd    mm5, mm1       ; x6*w15+x2*w13_x6*w11+x2*w09
                pmaddwd    mm6, mm2       ; x5*w30+x1*w28_x5*w26+x1*w24
                pmaddwd    mm7, mm3       ; x7*w31+x3*x29_x7*w27+x3*w25
                pmaddwd    mm0, [eax]     ; x4*w06+x0*w04_x4*w02+x0*w00
                pmaddwd    mm1, [eax+8]   ; x6*w07+x0*w05_x6*w03+x0*w01
                pmaddwd    mm2, [eax+32]  ; x5*w22+x1*w20_x5*w18+x1*w16
                pmaddwd    mm3, [eax+40]  ; x7*w23+x3*w21_x7*w19+x3*w17
                paddd      mm4, mm5       ; a3_a2
                paddd      mm6, mm7       ; b3_b2
                paddd      mm0, mm1       ; a1_a0
                paddd      mm2, mm3       ; b1_b0
                paddd      mm4, half11
                paddd      mm0, half11
                movq       mm5, mm6
                movq       mm1, mm2
                paddd      mm2, mm0       ; a1+b1_a0+b0
                paddd      mm6, mm4       ; a3+b3_a2+b2
                psubd      mm0, mm1       ; a1-b1_a0-b0
                psubd      mm4, mm5       ; a3-b3_a2-b2
                psrad      mm2, 11        ; y1_y0
                psrad      mm6, 11        ; y3_y2
                psrad      mm0, 11        ; y6_y7
                psrad      mm4, 11        ; y4_y5
                packssdw   mm2, mm6       ; y3_y2_y1_y0
                packssdw   mm4, mm0       ; y6_y7_y4_y5
                movq       [esi+32], mm2  ;
                movq       mm7, mm4       ; 
                psrld      mm4, 16        ; 00_y6_00_y4
                pslld      mm7, 16        ; y7_00_y5_00
                por        mm4, mm7       ; y7_y6_y5_y4
                movq       [esi+40], mm4  ;
            ; row 6
                movd       mm0, dword ptr [esi+96]  ; 00_00_x1_x0
                movd       mm1, dword ptr [esi+100] ; 00_00_x3_x2
                movd       mm2, dword ptr [esi+104] ; 00_00_x5_x4
                movd       mm3, dword ptr [esi+108] ; 00_00_x7_x4
                movq       mm4, qword ptr [eax+16]  ; w14_w12_w10_w08
                movq       mm5, qword ptr [eax+24]  ; w15_w13_w11_w09
                movq       mm6, qword ptr [eax+48]  ; w30_w28_w26_w24
                movq       mm7, qword ptr [eax+56]  ; w31_w29_w27_w25
                punpcklwd  mm0, mm2       ; x5_x1_x4_x0
                punpcklwd  mm1, mm3       ; x7_x3_x6_x2
                movq       mm2, mm0
                movq       mm3, mm1
                punpckldq  mm0, mm0       ; x4_x0_x4_x0
                punpckhdq  mm2, mm2       ; x5_x1_x5_x1
                punpckldq  mm1, mm1       ; x6_x2_x6_x2
                punpckhdq  mm3, mm3       ; x7_x3_x7_x3
                pmaddwd    mm4, mm0       ; x4*w14+x0*w12_x4*w10+x0*w08
                pmaddwd    mm5, mm1       ; x6*w15+x2*w13_x6*w11+x2*w09
                pmaddwd    mm6, mm2       ; x5*w30+x1*w28_x5*w26+x1*w24
                pmaddwd    mm7, mm3       ; x7*w31+x3*x29_x7*w27+x3*w25
                pmaddwd    mm0, [eax]     ; x4*w06+x0*w04_x4*w02+x0*w00
                pmaddwd    mm1, [eax+8]   ; x6*w07+x0*w05_x6*w03+x0*w01
                pmaddwd    mm2, [eax+32]  ; x5*w22+x1*w20_x5*w18+x1*w16
                pmaddwd    mm3, [eax+40]  ; x7*w23+x3*w21_x7*w19+x3*w17
                paddd      mm4, mm5       ; a3_a2
                paddd      mm6, mm7       ; b3_b2
                paddd      mm0, mm1       ; a1_a0
                paddd      mm2, mm3       ; b1_b0
                paddd      mm4, half11
                paddd      mm0, half11
                movq       mm5, mm6
                movq       mm1, mm2
                paddd      mm2, mm0       ; a1+b1_a0+b0
                paddd      mm6, mm4       ; a3+b3_a2+b2
                psubd      mm0, mm1       ; a1-b1_a0-b0
                psubd      mm4, mm5       ; a3-b3_a2-b2
                psrad      mm2, 11        ; y1_y0
                psrad      mm6, 11        ; y3_y2
                psrad      mm0, 11        ; y6_y7
                psrad      mm4, 11        ; y4_y5
                packssdw   mm2, mm6       ; y3_y2_y1_y0
                packssdw   mm4, mm0       ; y6_y7_y4_y5
                movq       [esi+96], mm2  ;
                movq       mm7, mm4       ; 
                psrld      mm4, 16        ; 00_y6_00_y4
                pslld      mm7, 16        ; y7_00_y5_00
                por        mm4, mm7       ; y7_y6_y5_y4
                movq       [esi+104], mm4 ;
            ; row 3
                lea        eax, table_35
                movd       mm0, dword ptr [esi+48]  ; 00_00_x1_x0
                movd       mm1, dword ptr [esi+52]  ; 00_00_x3_x2
                movd       mm2, dword ptr [esi+56]  ; 00_00_x5_x4
                movd       mm3, dword ptr [esi+60]  ; 00_00_x7_x4
                movq       mm4, qword ptr [eax+16]  ; w14_w12_w10_w08
                movq       mm5, qword ptr [eax+24]  ; w15_w13_w11_w09
                movq       mm6, qword ptr [eax+48]  ; w30_w28_w26_w24
                movq       mm7, qword ptr [eax+56]  ; w31_w29_w27_w25
                punpcklwd  mm0, mm2       ; x5_x1_x4_x0
                punpcklwd  mm1, mm3       ; x7_x3_x6_x2
                movq       mm2, mm0
                movq       mm3, mm1
                punpckldq  mm0, mm0       ; x4_x0_x4_x0
                punpckhdq  mm2, mm2       ; x5_x1_x5_x1
                punpckldq  mm1, mm1       ; x6_x2_x6_x2
                punpckhdq  mm3, mm3       ; x7_x3_x7_x3
                pmaddwd    mm4, mm0       ; x4*w14+x0*w12_x4*w10+x0*w08
                pmaddwd    mm5, mm1       ; x6*w15+x2*w13_x6*w11+x2*w09
                pmaddwd    mm6, mm2       ; x5*w30+x1*w28_x5*w26+x1*w24
                pmaddwd    mm7, mm3       ; x7*w31+x3*x29_x7*w27+x3*w25
                pmaddwd    mm0, [eax]     ; x4*w06+x0*w04_x4*w02+x0*w00
                pmaddwd    mm1, [eax+8]   ; x6*w07+x0*w05_x6*w03+x0*w01
                pmaddwd    mm2, [eax+32]  ; x5*w22+x1*w20_x5*w18+x1*w16
                pmaddwd    mm3, [eax+40]  ; x7*w23+x3*w21_x7*w19+x3*w17
                paddd      mm4, mm5       ; a3_a2
                paddd      mm6, mm7       ; b3_b2
                paddd      mm0, mm1       ; a1_a0
                paddd      mm2, mm3       ; b1_b0
                paddd      mm4, half11
                paddd      mm0, half11
                movq       mm5, mm6
                movq       mm1, mm2
                paddd      mm2, mm0       ; a1+b1_a0+b0
                paddd      mm6, mm4       ; a3+b3_a2+b2
                psubd      mm0, mm1       ; a1-b1_a0-b0
                psubd      mm4, mm5       ; a3-b3_a2-b2
                psrad      mm2, 11        ; y1_y0
                psrad      mm6, 11        ; y3_y2
                psrad      mm0, 11        ; y6_y7
                psrad      mm4, 11        ; y4_y5
                packssdw   mm2, mm6       ; y3_y2_y1_y0
                packssdw   mm4, mm0       ; y6_y7_y4_y5
                movq       [esi+48], mm2  ;
                movq       mm7, mm4       ; 
                psrld      mm4, 16        ; 00_y6_00_y4
                pslld      mm7, 16        ; y7_00_y5_00
                por        mm4, mm7       ; y7_y6_y5_y4
                movq       [esi+56], mm4  ;
            ; row 5
                movd       mm0, dword ptr [esi+80]  ; 00_00_x1_x0
                movd       mm1, dword ptr [esi+84]  ; 00_00_x3_x2
                movd       mm2, dword ptr [esi+88]  ; 00_00_x5_x4
                movd       mm3, dword ptr [esi+92]  ; 00_00_x7_x4
                movq       mm4, qword ptr [eax+16]  ; w14_w12_w10_w08
                movq       mm5, qword ptr [eax+24]  ; w15_w13_w11_w09
                movq       mm6, qword ptr [eax+48]  ; w30_w28_w26_w24
                movq       mm7, qword ptr [eax+56]  ; w31_w29_w27_w25
                punpcklwd  mm0, mm2       ; x5_x1_x4_x0
                punpcklwd  mm1, mm3       ; x7_x3_x6_x2
                movq       mm2, mm0
                movq       mm3, mm1
                punpckldq  mm0, mm0       ; x4_x0_x4_x0
                punpckhdq  mm2, mm2       ; x5_x1_x5_x1
                punpckldq  mm1, mm1       ; x6_x2_x6_x2
                punpckhdq  mm3, mm3       ; x7_x3_x7_x3
                pmaddwd    mm4, mm0       ; x4*w14+x0*w12_x4*w10+x0*w08
                pmaddwd    mm5, mm1       ; x6*w15+x2*w13_x6*w11+x2*w09
                pmaddwd    mm6, mm2       ; x5*w30+x1*w28_x5*w26+x1*w24
                pmaddwd    mm7, mm3       ; x7*w31+x3*x29_x7*w27+x3*w25
                pmaddwd    mm0, [eax]     ; x4*w06+x0*w04_x4*w02+x0*w00
                pmaddwd    mm1, [eax+8]   ; x6*w07+x0*w05_x6*w03+x0*w01
                pmaddwd    mm2, [eax+32]  ; x5*w22+x1*w20_x5*w18+x1*w16
                pmaddwd    mm3, [eax+40]  ; x7*w23+x3*w21_x7*w19+x3*w17
                paddd      mm4, mm5       ; a3_a2
                paddd      mm6, mm7       ; b3_b2
                paddd      mm0, mm1       ; a1_a0
                paddd      mm2, mm3       ; b1_b0
                paddd      mm4, half11
                paddd      mm0, half11
                movq       mm5, mm6
                movq       mm1, mm2
                paddd      mm2, mm0       ; a1+b1_a0+b0
                paddd      mm6, mm4       ; a3+b3_a2+b2
                psubd      mm0, mm1       ; a1-b1_a0-b0
                psubd      mm4, mm5       ; a3-b3_a2-b2
                psrad      mm2, 11        ; y1_y0
                psrad      mm6, 11        ; y3_y2
                psrad      mm0, 11        ; y6_y7
                psrad      mm4, 11        ; y4_y5
                packssdw   mm2, mm6       ; y3_y2_y1_y0
                packssdw   mm4, mm0       ; y6_y7_y4_y5
                movq       [esi+80], mm2  ;
                movq       mm7, mm4       ; 
                psrld      mm4, 16        ; 00_y6_00_y4
                pslld      mm7, 16        ; y7_00_y5_00
                por        mm4, mm7       ; y7_y6_y5_y4
                movq       [esi+88], mm4  ;
            ; 列処理
            ; col 0-3
                movq       mm0, [esi+5*16]
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
                movq   	    [esi+3*16], mm6 ; temporal save t4
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
;-------------------------------------------------------------------
; 後始末
                pop        eax
                pop        esi

                ret        4
;-------------------------------------------------------------------
_idct_ap922_mmx@4 ENDP
;-------------------------------------------------------------------
; 終了

END
