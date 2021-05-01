;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; お約束
.586
.mmx
.xmm
.model flat
_TEXT64 segment page public use32 'CODE'
		align 16
;-------------------------------------------------------------------
;IEEE test conditions: -L = -256, +H = 255, sign = 1, #iters = 10000, elapsed = 1261
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
; idct_ap922_sse2 - 方針
; 
; AP-945 idct_m128asm.cpp のコードをコピペ
;-------------------------------------------------------------------
; 定数
one          dq 00001000100010001h ;     1,     1,     1,     1,
             dq 00001000100010001h ;     1,     1,     1,     1,
half11       dq 00000040000000400h ;         1024,         1024,
             dq 00000040000000400h ;         1024,         1024,
half06       dq 00020002000200020h ;    32,    32,    32,    32,
             dq 00020002000200020h ;    32,    32,    32,    32,
half06a      dq 0001f001f001f001fh ;    31,    31,    31,    31,
             dq 0001f001f001f001fh ;    31,    31,    31,    31,
tan_1        dq 032ec32ec32ec32ech ; 13036, 13036, 13036, 13036,
             dq 032ec32ec32ec32ech ; 13036, 13036, 13036, 13036,
tan_2        dq 06a0a6a0a6a0a6a0ah ; 27146, 27146, 27146, 27146,
             dq 06a0a6a0a6a0a6a0ah ; 27146, 27146, 27146, 27146,
tan_3        dq 0ab0eab0eab0eab0eh ; 43790, 43790, 43790, 43790,
             dq 0ab0eab0eab0eab0eh ; 43790, 43790, 43790, 43790,
                                   ;-21746,-21746,-21746,-21746,
                                   ;-21746,-21746,-21746,-21746,
cos_4        dq 0b505b505b505b505h ; 46341, 46341, 46341, 46341,
             dq 0b505b505b505b505h ; 46341, 46341, 46341, 46341,
                                   ;-19195,-19195,-19195,-19195,
                                   ;-19195,-19195,-19195,-19195,

table_04     dq 022a34000539f4000h ;  8867, 16384, 21407, 16384, w05_w04_w01_w00
             dq 0ac614000dd5d4000h ;-21407, 16384, -8867, 16384, w13_w12_w09_w08
             dq 0ac61c00022a34000h ;-21407,-16384,  8867, 16384, w07_w06_w03_w02
             dq 0dd5d4000539fc000h ; -8867, 16384, 21407,-16384, w15_w14_w11_w10
             dq 0ee584b424b4258c5h ; -4520, 19266, 19266, 22725, w21_w20_w17_w16
             dq 0cdb711a8a73b3249h ;-12873,  4520,-22725, 12873, w29_w28_w25_w24
             dq 0cdb7a73b11a83249h ;-12873,-22725,  4520, 12873, w23_w22_w19_w18
             dq 0a73b4b424b4211a8h ;-22725, 19266, 19266,  4520, w31_w30_w27_w26

table_17     dq 0300b58c573fc58c5h ; 12299, 22725, 29692, 22725, w05_w04_w01_w00
             dq 08c0458c5cff558c5h ;-29692, 22725,-12299, 22725, w13_w12_w09_w08
             dq 08c04a73b300b58c5h ;-29692,-22725, 12299, 22725, w07_w06_w03_w02
             dq 0cff558c573fca73bh ;-12299, 22725, 29692,-22725, w15_w14_w11_w10
             dq 0e782686268627b21h ; -6270, 26722, 26722, 31521, w21_w20_w17_w16
             dq 0ba41187e84df45bfh ;-17855,  6270,-31521, 17855, w29_w28_w25_w24
             dq 0ba4184df187e45bfh ;-17855,-31521,  6270, 17855, w23_w22_w19_w18
             dq 084df68626862187eh ;-31521, 26722, 26722,  6270, w31_w30_w27_w26

table_26     dq 02d41539f6d41539fh ; 11585, 21407, 27969, 21407, w05_w04_w01_w00
             dq 092bf539fd2bf539fh ;-27969, 21407,-11585, 21407, w13_w12_w09_w08
             dq 092bfac612d41539fh ;-27969,-21407, 11585, 21407, w07_w06_w03_w02
             dq 0d2bf539f6d41ac61h ;-11585, 21407, 27969,-21407, w15_w14_w11_w10
             dq 0e8ee6254625473fch ; -5906, 25172, 25172, 29692, w21_w20_w17_w16
             dq 0be4d17128c0441b3h ;-16819,  5906,-29692, 16819, w29_w28_w25_w24
             dq 0be4d8c04171241b3h ;-16819,-29692,  5906, 16819, w23_w22_w19_w18
             dq 08c04625462541712h ;-29692, 25172, 25172,  5906, w31_w30_w27_w26

table_35     dq 028ba4b4262544b42h ; 10426, 19266, 25172, 19266, w05_w04_w01_w00
             dq 09dac4b42d7464b42h ;-25172, 19266,-10426, 19266, w13_w12_w09_w08
             dq 09dacb4be28ba4b42h ;-25172,-19266, 10426, 19266, w07_w06_w03_w02
             dq 0d7464b426254b4beh ;-10426, 19266, 25172,-19266, w15_w14_w11_w10
             dq 0eb3d587e587e6862h ; -5315, 22654, 22654, 26722, w21_w20_w17_w16
             dq 0c4df14c3979e3b21h ;-15137,  5315,-26722, 15137, w29_w28_w25_w24
             dq 0c4df979e14c33b21h ;-15137,-26722,  5315, 15137, w23_w22_w19_w18
             dq 0979e587e587e14c3h ;-26722, 22654, 22654,  5315, w31_w30_w27_w26

;-------------------------------------------------------------------
PUBLIC              C _idct_ap922_sse2@4
;      void __stdcall  idct_ap922_sse2(
; [esp + 4] = short *block,
; )
_idct_ap922_sse2@4 PROC
;-------------------------------------------------------------------
; レジスタ退避
                push       esi
                push       eax
                push       ebx
;-------------------------------------------------------------------
; 引数からのデータ受け取り
                mov        esi, [esp+12+4]
;-------------------------------------------------------------------
                ; row 0, row 2
                lea        eax, table_04
                lea        ebx, table_26
                movdqa     xmm0, [esi]       ; row 0
                movdqa     xmm4, [esi+2*16]  ; row 2
                pshuflw    xmm0, xmm0, 11011000b ; x7_x6_x5_x4_x3_x1_x2_x0 
                pshufhw    xmm0, xmm0, 11011000b ; x7_x5_x6_x4_x3_x1_x2_x0 
                pshufd     xmm3, xmm0, 055h      ; x3_x1_x3_x1_x3_x1_x3_x1
                pshufd     xmm1, xmm0, 0         ; x2_x0_x2_x0_x2_x0_x2_x0
                pshufd     xmm2, xmm0, 0AAh      ; x6_x4_x6_x4_x6_x4_x6_x4
                pshufd     xmm0, xmm0, 0FFh      ; x7_x5_x7_x5_x7_x5_x7_x5
                pmaddwd    xmm1, [eax]
                pmaddwd    xmm2, [eax+16]
                pmaddwd    xmm3, [eax+32]
                pmaddwd    xmm0, [eax+48]
                paddd      xmm0, xmm3
                pshuflw    xmm4, xmm4, 0D8h
                pshufhw    xmm4, xmm4, 0D8h
                movdqa     xmm7, xmmword ptr half11
                paddd      xmm1, xmm7
                pshufd     xmm6, xmm4, 0AAh
                pshufd     xmm5, xmm4, 0
                pmaddwd    xmm5, [ebx]
                paddd      xmm5, xmm7
                pmaddwd    xmm6, [ebx+16]
                pshufd     xmm7, xmm4, 055h
                pmaddwd    xmm7, [ebx+32]
                pshufd     xmm4, xmm4, 0FFh
                pmaddwd    xmm4, [ebx+48]
                paddd      xmm1, xmm2
                movdqa     xmm2, xmm1
                psubd      xmm2, xmm0
                psrad      xmm2, 11
                pshufd     xmm2, xmm2, 01Bh
                paddd      xmm0, xmm1
                psrad      xmm0, 11
                paddd      xmm5, xmm6
                packssdw   xmm0, xmm2
                paddd      xmm4, xmm7
                movdqa     xmm6, xmm5
                psubd      xmm6, xmm4
                psrad      xmm6, 11
                paddd      xmm4, xmm5
                psrad      xmm4, 11
                pshufd     xmm6, xmm6, 01Bh
                packssdw   xmm4, xmm6
                movdqa     [esi], xmm0
                movdqa     [esi+2*16], xmm4
                ; row 4, row 6
                movdqa     xmm0, [esi+4*16]
                movdqa     xmm4, [esi+6*16]
                pshuflw    xmm0, xmm0, 0D8h 
                pshufhw    xmm0, xmm0, 0D8h
                pshufd     xmm3, xmm0, 055h
                pshufd     xmm1, xmm0, 0
                pshufd     xmm2, xmm0, 0AAh
                pshufd     xmm0, xmm0, 0FFh
                pmaddwd    xmm1, [eax]
                pmaddwd    xmm2, [eax+16]
                pmaddwd    xmm3, [eax+32]
                pmaddwd    xmm0, [eax+48]
                paddd      xmm0, xmm3
                pshuflw    xmm4, xmm4, 0D8h
                pshufhw    xmm4, xmm4, 0D8h
                movdqa     xmm7, xmmword ptr half11
                paddd      xmm1, xmm7
                pshufd     xmm6, xmm4, 0AAh
                pshufd     xmm5, xmm4, 0
                pmaddwd    xmm5, [ebx]
                paddd      xmm5, xmm7
                pmaddwd    xmm6, [ebx+16]
                pshufd     xmm7, xmm4, 055h
                pmaddwd    xmm7, [ebx+32]
                pshufd     xmm4, xmm4, 0FFh
                pmaddwd    xmm4, [ebx+48]
                paddd      xmm1, xmm2
                movdqa     xmm2, xmm1
                psubd      xmm2, xmm0
                psrad      xmm2, 11
                pshufd     xmm2, xmm2, 01Bh
                paddd      xmm0, xmm1
                psrad      xmm0, 11
                paddd      xmm5, xmm6
                packssdw   xmm0, xmm2
                paddd      xmm4, xmm7
                movdqa     xmm6, xmm5
                psubd      xmm6, xmm4
                psrad      xmm6, 11
                paddd      xmm4, xmm5
                psrad      xmm4, 11
                pshufd     xmm6, xmm6, 01Bh
                packssdw   xmm4, xmm6
                movdqa     [esi+4*16], xmm0
                movdqa     [esi+6*16], xmm4
                ; row 3, row 1
                movdqa     xmm0, [esi+3*16]
                lea        eax, table_35
                movdqa     xmm4, [esi+1*16]
                lea        ebx, table_17
                pshuflw    xmm0, xmm0, 0D8h 
                pshufhw    xmm0, xmm0, 0D8h
                pshufd     xmm3, xmm0, 055h
                pshufd     xmm1, xmm0, 0
                pshufd     xmm2, xmm0, 0AAh
                pshufd     xmm0, xmm0, 0FFh
                pmaddwd    xmm1, [eax]
                pmaddwd    xmm2, [eax+16]
                pmaddwd    xmm3, [eax+32]
                pmaddwd    xmm0, [eax+48]
                paddd      xmm0, xmm3
                pshuflw    xmm4, xmm4, 0D8h
                pshufhw    xmm4, xmm4, 0D8h
                movdqa     xmm7, xmmword ptr half11
                paddd      xmm1, xmm7
                pshufd     xmm6, xmm4, 0AAh
                pshufd     xmm5, xmm4, 0
                pmaddwd    xmm5, [ebx]
                paddd      xmm5, xmm7
                pmaddwd    xmm6, [ebx+16]
                pshufd     xmm7, xmm4, 055h
                pmaddwd    xmm7, [ebx+32]
                pshufd     xmm4, xmm4, 0FFh
                pmaddwd    xmm4, [ebx+48]
                paddd      xmm1, xmm2
                movdqa     xmm2, xmm1
                psubd      xmm2, xmm0
                psrad      xmm2, 11
                pshufd     xmm2, xmm2, 01Bh
                paddd      xmm0, xmm1
                psrad      xmm0, 11
                paddd      xmm5, xmm6
                packssdw   xmm0, xmm2
                paddd      xmm4, xmm7
                movdqa     xmm6, xmm5
                psubd      xmm6, xmm4
                psrad      xmm6, 11
                paddd      xmm4, xmm5
                psrad      xmm4, 11
                pshufd     xmm6, xmm6, 01Bh
                packssdw   xmm4, xmm6
                movdqa     [esi+3*16], xmm0
                movdqa     [esi+1*16], xmm4
                ; row 5, row 7
                movdqa     xmm0, [esi+5*16]
                movdqa     xmm4, [esi+7*16]
                pshuflw    xmm0, xmm0, 0D8h 
                pshufhw    xmm0, xmm0, 0D8h
                pshufd     xmm3, xmm0, 055h
                pshufd     xmm1, xmm0, 0
                pshufd     xmm2, xmm0, 0AAh
                pshufd     xmm0, xmm0, 0FFh
                pmaddwd    xmm1, [eax]
                pmaddwd    xmm2, [eax+16]
                pmaddwd    xmm3, [eax+32]
                pmaddwd    xmm0, [eax+48]
                paddd      xmm0, xmm3
                pshuflw    xmm4, xmm4, 0D8h
                pshufhw    xmm4, xmm4, 0D8h
                movdqa     xmm7, xmmword ptr half11
                paddd      xmm1, xmm7
                pshufd     xmm6, xmm4, 0AAh
                pshufd     xmm5, xmm4, 0
                pmaddwd    xmm5, [ebx]
                paddd      xmm5, xmm7
                pmaddwd    xmm6, [ebx+16]
                pshufd     xmm7, xmm4, 055h
                pmaddwd    xmm7, [ebx+32]
                pshufd     xmm4, xmm4, 0FFh
                pmaddwd    xmm4, [ebx+48]
                paddd      xmm1, xmm2
                movdqa     xmm2, xmm1
                psubd      xmm2, xmm0
                psrad      xmm2, 11
                pshufd     xmm2, xmm2, 01Bh
                paddd      xmm0, xmm1
                psrad      xmm0, 11
                paddd      xmm5, xmm6
                packssdw   xmm0, xmm2
                paddd      xmm4, xmm7
                movdqa     xmm6, xmm5
                psubd      xmm6, xmm4
                psrad      xmm6, 11
                paddd      xmm4, xmm5
                psrad      xmm4, 11
                pshufd     xmm6, xmm6, 01Bh
                packssdw   xmm4, xmm6
                ; col 0-7
                movdqa     xmm6, xmm4			
                movdqa     xmm2, xmm0			
                movdqa     xmm3, [esi+3*16]
                movdqa     xmm1, xmmword ptr tan_3
                pmulhw     xmm0, xmm1			
                movdqa     xmm5, xmmword ptr tan_1
                pmulhw     xmm1, xmm3			
                paddsw     xmm1, xmm3			
                pmulhw     xmm4, xmm5			
                movdqa     xmm7, [esi+6*16]
                pmulhw     xmm5, [esi+1*16]		
                psubsw     xmm5, xmm6			
                movdqa     xmm6, xmm5			
                paddsw     xmm4, [esi+1*16]		
                paddsw     xmm0, xmm2			
                paddsw     xmm0, xmm3			
                psubsw     xmm2, xmm1			
                movdqa     xmm1, xmm0			
                movdqa     xmm3, xmmword ptr tan_2
                pmulhw     xmm7, xmm3			
                pmulhw     xmm3, [esi+2*16]		
                paddsw     xmm0, xmm4			
                psubsw     xmm4, xmm1			
                paddsw     xmm0, xmmword ptr one
                movdqa     [esi+7*16],  xmm0		
                psubsw     xmm5, xmm2			
                paddsw     xmm5, xmmword ptr one
                paddsw     xmm6, xmm2			
                movdqa     [esi+3*16],  xmm6		
                movdqa     xmm1, xmm4			
                movdqa     xmm0, xmmword ptr cos_4
                movdqa     xmm2, xmm0			
                paddsw     xmm4, xmm5			
                psubsw     xmm1, xmm5			
                paddsw     xmm7, [esi+2*16]		
                psubsw     xmm3, [esi+6*16]		
                movdqa     xmm6, [esi]			
                pmulhw     xmm0, xmm1			
                movdqa     xmm5, [esi+4*16]		
                paddsw     xmm5, xmm6			
                psubsw     xmm6, [esi+4*16]		
                pmulhw     xmm2, xmm4			
                paddsw     xmm4, xmm2			
                movdqa     xmm2, xmm5			
                psubsw     xmm2, xmm7			
                por        xmm4, xmmword ptr one
                paddsw     xmm0, xmm1			
                por        xmm0, xmmword ptr one
                paddsw     xmm5, xmm7			
                paddsw     xmm5, xmmword ptr half06
                movdqa     xmm1, xmm6			
                movdqa     xmm7, [esi+7*16]		
                paddsw     xmm7, xmm5			
                psraw      xmm7, 6
                movdqa     [esi], xmm7			
                paddsw     xmm6, xmm3			
                paddsw     xmm6, xmmword ptr half06
                psubsw     xmm1, xmm3			
                paddsw     xmm1, xmmword ptr half06a
                movdqa     xmm7, xmm1			
                movdqa     xmm3, xmm6			
                paddsw     xmm6, xmm4			
                paddsw     xmm2, xmmword ptr half06a
                psraw      xmm6, 6
                movdqa     [esi+1*16], xmm6		
                paddsw     xmm1, xmm0			
                psraw      xmm1, 6
                movdqa     [esi+2*16], xmm1
                movdqa     xmm1, [esi+3*16] 		
                movdqa     xmm6, xmm1           		
                psubsw     xmm7, xmm0			
                psraw      xmm7, 6
                movdqa     [esi+5*16],  xmm7		
                psubsw     xmm5, [esi+7*16]		
                psraw      xmm5, 6
                movdqa     [esi+7*16], xmm5		
                psubsw     xmm3, xmm4			
                paddsw     xmm6, xmm2			
                psubsw     xmm2, xmm1			
                psraw      xmm6, 6
                movdqa     [esi+3*16],  xmm6		
                psraw      xmm2, 6
                movdqa     [esi+4*16],  xmm2		
                psraw      xmm3, 6
                movdqa     [esi+6*16],  xmm3		
;-------------------------------------------------------------------
; 後始末
                pop        ebx
                pop        eax
                pop        esi

                ret        4
;-------------------------------------------------------------------
_idct_ap922_sse2@4 ENDP
;-------------------------------------------------------------------
; 終了

END
