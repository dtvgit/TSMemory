;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; お約束
.586
.mmx
.xmm
.model flat
_TEXT64 segment page public use32 'CODE'
		align 16

;-------------------------------------------------------------------
; 定数
;-------------------------------------------------------------------
int16_all7      dq 00007000700070007h ;     7,     7,     7,     7,
int16_all5      dq 00005000500050005h ;     5,     5,     5,     5,
int16_all4      dq 00004000400040004h ;     4,     4,     4,     4,
int16_all3      dq 00003000300030003h ;     3,     3,     3,     3,
int16_all2      dq 00002000200020002h ;     2,     2,     2,     2,

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; chroma420i_to_422_mmx - インタレース 420 から 422 への補間
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (y*4+2) = ((y*2  ) * 5 + (y*2+2) * 3 )>> 3
; (y*4+3) = ((y*2+1) * 7 + (y*2+3) * 1 )>> 3
; (y*4+4) = ((y*2  ) * 1 + (y*2+2) * 7 )>> 3
; (y*4+5) = ((y*2+1) * 3 + (y*2+3) * 5 )>> 3
;-------------------------------------------------------------------
PUBLIC              C _chroma420i_to_422_mmx@12
;      void __stdcall  chroma420i_to_422_mmx(
; [esp + 4] = unsigned char *data,
; [esp + 8] = int            width,
; [esp +12] = int            height
; )
;
_chroma420i_to_422_mmx@12 PROC
;-------------------------------------------------------------------
; レジスタ退避
		push       edi
		push       esi
		push       edx
		push       ecx
		push       ebx
		push       eax
;-------------------------------------------------------------------
; パラメータ受け取り
		mov        esi, [esp+24+4]  ; data
		mov        edx, [esp+24+8]  ; width
		mov        eax, [esp+24+12] ; height
;-------------------------------------------------------------------
; ローカル変数領域確保
		sub        esp, 12
;-------------------------------------------------------------------
; ループパラメータ作成
; 
; 作るべきパラメータ
; (height/4)-1 - eax
; width        - edx
; width/8      - ecx & [esp+12]
; width*2-4    - ebx
; width/2*7    - [esp+4]
; width/2*3    - [esp+8]
; data+width/2 - edi
; all 0        - mm7
; all 4        - mm6
		mov        ecx, edx
		mov        ebx, edx
		movq       mm6, int16_all4
		pxor       mm7, mm7
		shr        eax, 2
		shr        ecx, 1
		shl        ebx, 1
		lea        edi, [esi+ecx]
		add        ecx, edx
		dec        eax
		add        ebx, ecx
		mov        [esp+4], ebx
		mov        [esp+8], ecx
		mov        ecx, edx
		mov        ebx, edx
		shr        ecx, 3
		shl        ebx, 1
		mov        [esp+12], ecx
		sub        ebx, 4
;-------------------------------------------------------------------
; 補間例外 - トップ２行そのままコピー
		cld
		rep movsd
		mov        ecx, esi
		mov        esi, edi
		lea        edi, [ecx+edx]
		mov        ecx, [esp+12]
		rep movsd
		lea        edi, [esi+edx]
		mov        esi, [esp+12+24+4]
		mov        ecx, [esp+12]
;-------------------------------------------------------------------
; 縦方向ループ
chroma420i_to_422_next_4_line:
;-------------------------------------------------------------------
; 横方向ループ
chroma420i_to_422_next_4_pixel:
;-------------------------------------------------------------------
; 補間コア
;
; ココでのレジスタ用途
; esi - 入力アドレス
; edi - 出力アドレス
; edx - 幅
; mm7 - all 0
; mm6 - all 4
		movd       mm0, dword ptr [esi]     ; c[0][x]
		add        esi, edx;
		movd       mm1, dword ptr [esi]     ; c[1][x]  
		add        esi, edx;
		movd       mm2, dword ptr [esi]     ; c[2][x]
		movd       mm3, dword ptr [esi+edx] ; c[3][x]
		movq       mm4, int16_all7  ; all 7
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		movq       mm5, mm4         ; all 7
		pmullw     mm4, mm1         ; 7*c[1][x]
		pmullw     mm1, int16_all3  ; 3*c[1][x]
		pmullw     mm5, mm2         ; 7*c[2][x]
		pmullw     mm2, int16_all3  ; 3*c[2][x]
		paddw      mm4, mm3         ; 7*c[1][x] + 1*c[3][x]
		paddw      mm5, mm0         ; 7*c[2][x] + 1*c[0][x]
		pmullw     mm0, int16_all5  ; 5*c[0][x]
		pmullw     mm3, int16_all5  ; 5*c[3][x] 
		paddw      mm4, mm6         ; 7*c[1][x] + 1*c[3][x] + 4
		paddw      mm5, mm6         ; 7*c[2][x] + 1*c[0][x] + 4
		paddw      mm0, mm2         ; 5*c[0][x] + 3*c[2][x]
		paddw      mm3, mm1         ; 5*c[3][x] + 3*c[1][x]
		paddw      mm0, mm6;        ; 5*c[0][x] + 3*c[2][x] + 4
		paddw      mm3, mm6;        ; 5*c[3][x] + 3*c[1][x] + 4
		psraw      mm0, 3;
		psraw      mm4, 3;
		psraw      mm5, 3;
		psraw      mm3, 3;
		packuswb   mm0, mm5;
		packuswb   mm4, mm3;
		movd       dword ptr [edi], mm0;
		add        edi, edx;
		movd       dword ptr [edi], mm4;
		add        edi, edx;
		psrlq      mm0, 32;
		psrlq      mm4, 32;
		movd       dword ptr [edi], mm0;
		movd       dword ptr [edi+edx], mm4;
;-------------------------------------------------------------------
; 横方向ループ終端チェック
		sub        esi, ebx
		sub        edi, ebx
		dec        ecx
		jnz        chroma420i_to_422_next_4_pixel
;-------------------------------------------------------------------
; 縦方向ループ終端チェック
		add        edi, [esp+4]
		add        esi, [esp+8]
		mov        ecx, [esp+12]
		dec        eax
		jnz        chroma420i_to_422_next_4_line
;-------------------------------------------------------------------
; 補間例外 - ボトム２行そのままコピー
		shr        edx, 1
		rep movsd
		lea        esi, [esi+edx]
		lea        edi, [edi+edx]
		mov        ecx, [esp+12]
		rep movsd
;-------------------------------------------------------------------
; 後始末
		add        esp, 12          ; ローカル変数解放

		pop        eax              ; レジスタ復元
		pop        ebx
		pop        ecx
		pop        edx
		pop        esi
		pop        edi

		ret        12
;-------------------------------------------------------------------
_chroma420i_to_422_mmx@12 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; chroma420p_to_422_mmx - プログレッシブ 420 から 422 への補間
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; [o1] = ([i0] * 3 + [i1]) / 4
; [o2] = ([i0] + [i1] * 3) / 4
;-------------------------------------------------------------------
PUBLIC              C _chroma420p_to_422_mmx@12
;      void __stdcall  chroma420p_to_422_mmx(
; [esp + 4] = unsigned char *data,
; [esp + 8] = int            width,
; [esp +12] = int            height
; )
;
_chroma420p_to_422_mmx@12 PROC
;-------------------------------------------------------------------
; レジスタ退避
		push       edi
		push       esi
		push       edx
		push       ecx
		push       ebx
		push       eax
;-------------------------------------------------------------------
; 引数からのデータ受け取り
		mov        esi, [esp+24+4]
		mov        edx, [esp+24+8]
		mov        ebx, [esp+24+12]
;-------------------------------------------------------------------
; ループパラメータ作成
; 
; 作るべきパラメータ
;
; data         - esi
; data+width/2 - edi
; width/2      - eax
; height/2-1   - ebx
; width        - edx
; all 2        - mm6
; all 0        - mm7
; 
		mov        eax, edx
		mov        ecx, edx
		shr        ebx, 1
		shr        eax, 1
		shr        ecx, 3
		movq       mm6, int16_all2;
		pxor       mm7, mm7
		dec        ebx
		lea        edi, [esi+eax]
;-------------------------------------------------------------------
; 補間例外 - 先頭 1 行コピー
		rep movsd
		mov        ecx, edx
		sub        esi, eax
		add        edi, eax
		shr        ecx, 4
;-------------------------------------------------------------------
; 縦方向ループ
chroma420p_to_422_mmx_next_2_line:
;-------------------------------------------------------------------
; 横方向ループ
chroma420p_to_422_mmx_next_8_pixel:
;-------------------------------------------------------------------
; 補間コア
; 
; ココでのパラメータ
; esi - 入力アドレス
; edi - 出力アドレス
; edx - 幅
; mm6 - all 2
; mm7 - all 0
		movd       mm0, dword ptr [esi]            ; i[0][0:3]
		movd       mm1, dword ptr [esi+4]          ; i[0][4:7]
		movd       mm2, dword ptr [esi+edx]        ; i[1][0:3]
		movd       mm3, dword ptr [esi+edx+4]      ; i[1][4:7]
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		add        esi, 8
		movq       mm4, mm0              ; i[0][0:3]
		movq       mm5, mm1              ; i[0][4:7]
		psllw      mm0, 1                ; i[0][0:3]*2
		psllw      mm1, 1                ; i[0][4:7]*2
		paddw      mm0, mm4              ; i[0][0:3]*3
		paddw      mm1, mm5              ; i[0][4:7]*3
		paddw      mm0, mm2              ; i[0][0:3]*3 + i[1][0:3]
		paddw      mm1, mm3              ; i[0][4:7]*3 + i[1][4:7]
		paddw      mm4, mm2              ; i[0][0:3] + i[1][0:3]
		paddw      mm5, mm3              ; i[0][4:7] + i[1][4:7]
		psllw      mm2, 1                ; i[1][0:3]*2
		psllw      mm3, 1                ; i[1][0:3]*2
		paddw      mm2, mm4              ; i[0][0:3] + i[1][0:3]*3
		paddw      mm3, mm5              ; i[0][4:7] + i[1][4:7]*3
		paddw      mm0, mm6              ; i[0][0:3]*3 + i[1][0:3] + 2
		paddw      mm1, mm6              ; i[0][4:7]*3 + i[1][4:7] + 2
		paddw      mm2, mm6              ; i[0][0:3] + i[1][0:3]*3 + 2
		paddw      mm3, mm6              ; i[0][4:7] + i[1][4:7]*3 + 2
		psraw      mm0, 2
		psraw      mm1, 2
		psraw      mm2, 2
		psraw      mm3, 2
		packuswb   mm0, mm1
		packuswb   mm2, mm3
		movq       [edi], mm0
		movq       [edi+edx], mm2
		add        edi, 8
;-------------------------------------------------------------------
; 横方向ループ終端チェック
		dec        ecx
		jnz        chroma420p_to_422_mmx_next_8_pixel
;-------------------------------------------------------------------
; 縦方向ループ終端チェック
		mov        ecx, edx
		add        edi, edx
		lea        esi, [esi+eax]
		lea        edi, [edi+eax]
		shr        ecx, 4
		dec        ebx
		jnz        chroma420p_to_422_mmx_next_2_line
;-------------------------------------------------------------------
; 補間例外 - ボトム 1 行コピー
		shl        ecx, 1
		rep movsd
;-------------------------------------------------------------------
; 後始末
		pop        eax; レジスタ復元
		pop        ebx
		pop        ecx
		pop        edx
		pop        esi
		pop        edi

		ret        12
;-------------------------------------------------------------------
_chroma420p_to_422_mmx@12 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; yuv422_to_bgr_mmx - YUV -> RGB 変換
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;-------------------------------------------------------------------
PUBLIC              C _yuv422_to_bgr_mmx@16
;      void __stdcall  yuv422_to_bgr_mmx(
; [esp + 4] = FRAME                    *top,
; [esp + 8] = FRAME                    *bottom,
; [esp +12] = unsigned char            *out,
; [esp +16] = BGR_CONVERSION_PARAMETER *prm
; )
_yuv422_to_bgr_mmx@16 PROC
;-------------------------------------------------------------------
; 使用するローカル変数
; 
; [esp+  8] y_offset
; [esp+ 16] y_gain
; [esp+ 24] half_13
; [esp+ 32] c_offset
; [esp+ 40] bu
; [esp+ 48] guv
; [esp+ 56] rv
; [esp+ 64] work
;  - ここまで 8 byte alignment 保証
; [esp+ 96] y_src
; [esp+100] y_src_next
; [esp+104] u_src
; [esp+108] v_src
; [esp+112] u_src_next
; [esp+116] v_src_next
; [esp+120] out
; [esp+124] width/8
; [esp+128] width%8*3
; [esp+132] in_step*2
; [esp+136] out_step
;
; total 140 + α
;-------------------------------------------------------------------
; レジスタの退避
		push       ebp
		push       edi
		push       esi
		push       edx
		push       ecx
		push       ebx
		push       eax
;-------------------------------------------------------------------
; ベースポインタにスタックアドレスを記憶
		mov        ebp, esp
;-------------------------------------------------------------------
; ローカル変数領域の確保
		sub        esp, 140
		and        esp, 0fffffff8h
;-------------------------------------------------------------------
; 変換係数の作成
;
; 作成すべきパラメータ
;
; y_offset   - [esp+8]
; y_gain     - [esp+16]
; half_13    - [esp+24]
; c_offset   - [esp+32]
; bu         - [esp+40]
; guv        - [esp+48]
; rv         - [esp+56]
;
; y_src      - esi & [esp+96]
; y_src_next - [esp+100]
; u_src      - eax & [esp+104]
; v_src      - ebx & [esp+108]
; u_src_next - [esp+112]
; v_src_next - [esp+116]
; out        - edi & [esp+120]
; width/8    - ecx & [esp+124]
; width%8*3  - [esp+128]
; height     - edx
; in_step*2  - [esp+132]
; out_step   - [esp+136]
;
		mov        edx, [ebp+28+16]        ; BGR_CONVERSION_PARAMETER
		pxor       mm0, mm0 ; all  0
		pxor       mm1, mm1 ; all  0
		pcmpeqd    mm2, mm2 ; all -1
		movd       mm3, dword ptr [edx+20] ; y_offset
		movd       mm4, dword ptr [edx+24] ; y_gain
		movd       mm5, dword ptr [edx+28] ; bu
		movd       mm6, dword ptr [edx+32] ; gu
		movd       mm7, dword ptr [edx+36] ; gv
		psubd      mm0, mm2 ; all  1
		psubw      mm1, mm2 ; all  1
		punpcklwd  mm3, mm3 ; xx_xx_yo_yo
		movq       mm2, mm0
		punpckldq  mm3, mm3 ; yo_yo_yo_yo
		pslld      mm0, 2  ; half_3
		psllw      mm1, 7  ; c_offset
		pslld      mm2, 12 ; half_13
		punpckldq  mm4, mm4 ; yg_yg
		punpckldq  mm5, mm5 ; bu_bu
		punpckldq  mm6, mm7 ; gv_gu
		movd       mm7, dword ptr [edx+40] ; rv
		paddd      mm4, mm0
		paddd      mm5, mm0
		paddd      mm6, mm0
		punpckldq  mm7, mm7 ; rv_rv
		psrad      mm4, 3
		psrad      mm5, 3
		paddd      mm7, mm0
		psrad      mm6, 3
		psrad      mm7, 3
		packssdw   mm6, mm6 ; gv_gu_gv_gu
		movq       [esp+8], mm3
		movq       [esp+16], mm4
		movq       [esp+24], mm2
		movq       [esp+32], mm1
		movq       [esp+40], mm5
		movq       [esp+48], mm6
		movq       [esp+56], mm7
		mov        eax, [edx] ; width
		mov        ecx, [edx] ; width
		movd       mm0, dword ptr [edx+8] ; in_step
		mov        ebx, [edx+12] ; out_step
		movd       mm1, dword ptr [edx+16] ; c_offset
		mov        edx, [edx+4] ; height
		mov        esi, [ebp+28+4] ; top
		mov        edi, [ebp+28+8] ; bottom
		and        eax, 7
		shr        ecx, 3
		mov        [esp+136], ebx ; out_step
		mov        ebx, eax
		shl        eax, 1
		movd       mm2, dword ptr [esi+ 8] ; top luma
		movd       mm3, dword ptr [esi+12] ; top cb
		movd       mm4, dword ptr [esi+16] ; top cr
		movd       mm5, dword ptr [edi+ 8] ; bottom luma
		movd       mm6, dword ptr [edi+12] ; bottom cb
		movd       mm7, dword ptr [edi+16] ; bottom cr
		add        eax, ebx
		punpckldq  mm0, mm0
		punpckldq  mm1, mm1
		punpckldq  mm3, mm4
		punpckldq  mm6, mm7
		paddd      mm5, mm0 ; bottom y + in_step
		paddd      mm6, mm1 ; bottom uv + c_offset
		paddd      mm3, mm1 ; top uv + c_offset
		paddd      mm6, mm0 ; bottom uv + in_step + c_offset
		punpckldq  mm2, mm5
		pslld      mm0, 1   ; in_step * 2
		mov        edi, [ebp+28+12] ; out
		movq       [esp+ 96], mm2 ; top y & bottom y
		movq       [esp+104], mm3 ; top uv
		movq       [esp+112], mm6 ; bottom uv
		mov        [esp+120], edi
		mov        [esp+124], ecx ; width/8
		mov        [esp+128], eax ; width%8*3
		movd       dword ptr [esp+132], mm0 ; in_step*2
		mov        esi, [esp+ 96]
		mov        eax, [esp+104]
		mov        ebx, [esp+108]
;-------------------------------------------------------------------
; 縦方向ループ
yuv422_to_bgr_next_line:
;-------------------------------------------------------------------
; 横方向ループ
yuv422_to_bgr_next_8_pixel:
;-------------------------------------------------------------------
; 変換コア
		pxor       mm7, mm7
		movd       mm0, dword ptr [esi]            ; Y[0:3]
		movd       mm2, dword ptr [esi+4]          ; Y[4:7]
		movd       mm4, dword ptr [eax]            ; U[0:3]
		movd       mm5, dword ptr [ebx]            ; V[0:3]
		lea        esi, [esi+8]
		lea        eax, [eax+4]
		lea        ebx, [ebx+4]
		punpcklbw  mm0, mm7
		punpcklbw  mm2, mm7
		psubw      mm0, [esp+8]
		psubw      mm2, [esp+8]
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		movq       mm1, mm0
		movq       mm3, mm2
		punpcklwd  mm0, mm7
		punpcklwd  mm2, mm7
		punpckhwd  mm1, mm7
		punpckhwd  mm3, mm7
		pmaddwd    mm0, [esp+16]
		pmaddwd    mm1, [esp+16]
		pmaddwd    mm2, [esp+16]
		pmaddwd    mm3, [esp+16]
		paddd      mm0, [esp+24]
		paddd      mm1, [esp+24]
		paddd      mm2, [esp+24]
		paddd      mm3, [esp+24]
		psubw      mm4, [esp+32]
		psubw      mm5, [esp+32]
		movq       [esp+64], mm0
		movq       [esp+72], mm1
		movq       [esp+80], mm2
		movq       [esp+88], mm3
		movq       mm0, mm4
		movq       mm1, mm4
		movq       mm2, mm4
		movq       mm3, mm5
		punpcklwd  mm0, mm7
		punpckhwd  mm1, mm7
		punpcklwd  mm2, mm5
		punpcklwd  mm3, mm7
		punpckhwd  mm4, mm5
		punpckhwd  mm5, mm7
		pmaddwd    mm0, [esp+40]
		pmaddwd    mm1, [esp+40]
		pmaddwd    mm2, [esp+48]
		pmaddwd    mm3, [esp+56]
		pmaddwd    mm4, [esp+48]
		pmaddwd    mm5, [esp+56]
		movq       mm6, mm0
		movq       mm7, mm1
		punpckldq  mm0, mm0
		punpckldq  mm1, mm1
		paddd      mm0, [esp+64]
		paddd      mm1, [esp+80]
		punpckhdq  mm6, mm6
		punpckhdq  mm7, mm7
		paddd      mm6, [esp+72]
		paddd      mm7, [esp+88]
		psrad      mm0, 13
		psrad      mm1, 13
		psrad      mm6, 13
		psrad      mm7, 13
		packssdw   mm0, mm6
		packssdw   mm1, mm7
		movq       mm6, mm2
		movq       mm7, mm4
		punpckldq  mm2, mm2
		punpckldq  mm4, mm4
		paddd      mm2, [esp+64]
		paddd      mm4, [esp+80]
		punpckhdq  mm6, mm6
		punpckhdq  mm7, mm7
		paddd      mm6, [esp+72]
		paddd      mm7, [esp+88]
		psrad      mm2, 13
		psrad      mm4, 13
		psrad      mm6, 13
		psrad      mm7, 13
		packssdw   mm2, mm6
		packssdw   mm4, mm7
		movq       mm6, mm3
		movq       mm7, mm5
		punpckldq  mm3, mm3
		punpckldq  mm5, mm5
		paddd      mm3, [esp+64]
		paddd      mm5, [esp+80]
		punpckhdq  mm6, mm6
		punpckhdq  mm7, mm7
		paddd      mm6, [esp+72]
		paddd      mm7, [esp+88]
		psrad      mm3, 13
		psrad      mm5, 13
		psrad      mm6, 13
		psrad      mm7, 13
		packssdw   mm3, mm6
		packssdw   mm5, mm7
;
; 現在のレジスタ状況
;
; mm0 - B[0:3]
; mm1 - B[4:7]
; mm2 - G[0:3]
; mm3 - R[0:3]
; mm4 - G[4:7]
; mm5 - R[4:7]
;
; ここから
;
; G2_B2_R1_G1_B1_R0_G0_B0
; B5_R4_G4_B4_R3_G3_B3_R2
; R7_G7_B7_R6_G6_B6_R5_G5
;
; とするのが目的
;
		packuswb   mm0, mm1   ; B7B6B5B4B3B2B1B0
		packuswb   mm2, mm4   ; G7G6G5G4G3G2G1G0
		packuswb   mm3, mm5   ; R7R6R5R4R3R2R1R0
		movq       mm1, mm0
		movq       mm4, mm0
		movq       mm5, mm2
		movq       mm6, mm3
		psrlq      mm1, 8     ; 00B7B6B5B4B3B2B1
		punpcklbw  mm0, mm2   ; G3B3G2B2G1B1G0B0 x3x0
		punpcklbw  mm2, mm3   ; R3G3R2G2R1G1R0G0 5x2x
		punpcklbw  mm3, mm1   ; B4R3B3R2B2R1B1R0 x4x1
		punpckhbw  mm4, mm5   ; G7B7G6B6G5B5G4B4 x9x6
		punpckhbw  mm5, mm6   ; R7G7R6G6R5G5R4G4 Bx8x
		punpckhbw  mm6, mm1   ; 00R7B7R6B6R5B5R4 xAx7
		movq       mm1, mm0
		movq       mm7, mm4
		psrlq      mm2, 16    ; 0000R3G3R2G2R1G1 x5x2
		psrlq      mm5, 16    ; 0000R7G7R6G6R5G5 xBx8
		punpcklwd  mm0, mm3   ; B2R1G1B1B1R0G0B0 xx10
		punpcklwd  mm4, mm6   ; B6R5G5B5B5R4G4B4 xx76
		psrlq      mm1, 32    ; 00000000G3B3G2B2 xxx3
		psrlq      mm7, 32    ; 00000000G7B7G6B6 xxx9
		punpckhwd  mm3, mm2   ; 0000B4R3R3G3B3R2 xx54
		punpckhwd  mm6, mm5   ; 000000R7R7G7B7R6 xxBA
		punpcklwd  mm2, mm1   ; G3B3R2G2G2B2R1G1 xx32
		punpcklwd  mm5, mm7   ; G7B7R6G6G6B6R5G5 xx98
		movd       dword ptr [edi], mm0
		movd       dword ptr [edi+4], mm2
		movd       dword ptr [edi+8], mm3
		movd       dword ptr [edi+12], mm4
		movd       dword ptr [edi+16], mm5
		movd       dword ptr [edi+20], mm6
		lea        edi, [edi+24]
;-------------------------------------------------------------------
; 横方向ループ終端チェック
		dec        ecx
		jnz        yuv422_to_bgr_next_8_pixel
;-------------------------------------------------------------------
; 端数処理
		mov        ecx, [esp+128]
		test       ecx, ecx
		jz         yuv422_to_bgr_line_end     
; 端数処理コア
		pxor       mm7, mm7
		movd       mm0, dword ptr [esi]            ; Y[0:3]
		movd       mm2, dword ptr [esi+4]          ; Y[4:7]
		movd       mm4, dword ptr [eax]            ; U[0:3]
		movd       mm5, dword ptr [ebx]            ; V[0:3]
		punpcklbw  mm0, mm7
		punpcklbw  mm2, mm7
		psubw      mm0, [esp+8]
		psubw      mm2, [esp+8]
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		movq       mm1, mm0
		movq       mm3, mm2
		punpcklwd  mm0, mm7
		punpcklwd  mm2, mm7
		punpckhwd  mm1, mm7
		punpckhwd  mm3, mm7
		pmaddwd    mm0, [esp+16]
		pmaddwd    mm1, [esp+16]
		pmaddwd    mm2, [esp+16]
		pmaddwd    mm3, [esp+16]
		paddd      mm0, [esp+24]
		paddd      mm1, [esp+24]
		paddd      mm2, [esp+24]
		paddd      mm3, [esp+24]
		psubw      mm4, [esp+32]
		psubw      mm5, [esp+32]
		movq       [esp+64], mm0
		movq       [esp+72], mm1
		movq       [esp+80], mm2
		movq       [esp+88], mm3
		movq       mm0, mm4
		movq       mm1, mm4
		movq       mm2, mm4
		movq       mm3, mm5
		punpcklwd  mm0, mm7
		punpckhwd  mm1, mm7
		punpcklwd  mm2, mm5
		punpcklwd  mm3, mm7
		punpckhwd  mm4, mm5
		punpckhwd  mm5, mm7
		pmaddwd    mm0, [esp+40]
		pmaddwd    mm1, [esp+40]
		pmaddwd    mm2, [esp+48]
		pmaddwd    mm3, [esp+56]
		pmaddwd    mm4, [esp+48]
		pmaddwd    mm5, [esp+56]
		movq       mm6, mm0
		movq       mm7, mm1
		punpckldq  mm0, mm0
		punpckldq  mm1, mm1
		paddd      mm0, [esp+64]
		paddd      mm1, [esp+80]
		punpckhdq  mm6, mm6
		punpckhdq  mm7, mm7
		paddd      mm6, [esp+72]
		paddd      mm7, [esp+88]
		psrad      mm0, 13
		psrad      mm1, 13
		psrad      mm6, 13
		psrad      mm7, 13
		packssdw   mm0, mm6
		packssdw   mm1, mm7
		movq       mm6, mm2
		movq       mm7, mm4
		punpckldq  mm2, mm2
		punpckldq  mm4, mm4
		paddd      mm2, [esp+64]
		paddd      mm4, [esp+80]
		punpckhdq  mm6, mm6
		punpckhdq  mm7, mm7
		paddd      mm6, [esp+72]
		paddd      mm7, [esp+88]
		psrad      mm2, 13
		psrad      mm4, 13
		psrad      mm6, 13
		psrad      mm7, 13
		packssdw   mm2, mm6
		packssdw   mm4, mm7
		movq       mm6, mm3
		movq       mm7, mm5
		punpckldq  mm3, mm3
		punpckldq  mm5, mm5
		paddd      mm3, [esp+64]
		paddd      mm5, [esp+80]
		punpckhdq  mm6, mm6
		punpckhdq  mm7, mm7
		paddd      mm6, [esp+72]
		paddd      mm7, [esp+88]
		psrad      mm3, 13
		psrad      mm5, 13
		psrad      mm6, 13
		psrad      mm7, 13
		packssdw   mm3, mm6
		packssdw   mm5, mm7
		packuswb   mm0, mm1   ; B7B6B5B4B3B2B1B0
		packuswb   mm2, mm4   ; G7G6G5G4G3G2G1G0
		packuswb   mm3, mm5   ; R7R6R5R4R3R2R1R0
		movq       mm1, mm0
		movq       mm4, mm0
		movq       mm5, mm2
		movq       mm6, mm3
		psrlq      mm1, 8     ; 00B7B6B5B4B3B2B1
		punpcklbw  mm0, mm2   ; G3B3G2B2G1B1G0B0 x3x0
		punpcklbw  mm2, mm3   ; R3G3R2G2R1G1R0G0 5x2x
		punpcklbw  mm3, mm1   ; B4R3B3R2B2R1B1R0 x4x1
		punpckhbw  mm4, mm5   ; G7B7G6B6G5B5G4B4 x9x6
		punpckhbw  mm5, mm6   ; R7G7R6G6R5G5R4G4 Bx8x
		punpckhbw  mm6, mm1   ; 00R7B7R6B6R5B5R4 xAx7
		movq       mm1, mm0
		movq       mm7, mm4
		psrlq      mm2, 16    ; 0000R3G3R2G2R1G1 x5x2
		psrlq      mm5, 16    ; 0000R7G7R6G6R5G5 xBx8
		punpcklwd  mm0, mm3   ; B2R1G1B1B1R0G0B0 xx10
		punpcklwd  mm4, mm6   ; B6R5G5B5B5R4G4B4 xx76
		psrlq      mm1, 32    ; 00000000G3B3G2B2 xxx3
		psrlq      mm7, 32    ; 00000000G7B7G6B6 xxx9
		punpckhwd  mm3, mm2   ; 0000B4R3R3G3B3R2 xx54
		punpckhwd  mm6, mm5   ; 000000R7R7G7B7R6 xxBA
		punpcklwd  mm2, mm1   ; G3B3R2G2G2B2R1G1 xx32
		punpcklwd  mm5, mm7   ; G7B7R6G6G6B6R5G5 xx98
		punpckldq  mm0, mm2   ; 3210
		punpckldq  mm3, mm4   ; 7654
		punpckldq  mm5, mm6   ; BA98
		movq       [esp+64], mm0
		movq       [esp+72], mm3
		movq       [esp+80], mm5
		lea        esi, [esp+64]
		rep movsb
;-------------------------------------------------------------------
; 縦方向ループ終端チェック
yuv422_to_bgr_line_end:
		movd       mm0, dword ptr [esp+132] ; in_step*2
		movd       mm1, dword ptr [esp+136] ; out_step
		movq       mm2, [esp+96]  ; y, current & next
		movq       mm3, [esp+104] ; current uv
		movq       mm4, [esp+112] ; next uv
		movd       mm5, dword ptr [esp+120] ; out
		paddd      mm2, mm0       ; current y + in_step * 2
		punpckldq  mm0, mm0
		paddd      mm5, mm1
		paddd      mm3, mm0
		movd       dword ptr [esp+100], mm2
		psrlq      mm2, 32
		movq       [esp+104], mm4
		movd       esi, mm2
		movq       [esp+112], mm3
		movd       edi, mm5
		mov        [esp+96], esi
		mov        [esp+120], edi
		mov        eax, [esp+104]
		mov        ebx, [esp+108]
		mov        ecx, [esp+124]
		dec        edx
		jnz        yuv422_to_bgr_next_line
;-------------------------------------------------------------------
; 後始末
		mov        esp, ebp

		pop        eax
		pop        ebx
		pop        ecx
		pop        edx
		pop        esi
		pop        edi
		pop        ebp

		ret        16
;
_yuv422_to_bgr_mmx@16 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; yuv422_to_yuy2_mmx - YUV -> YUY2 変換
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;-------------------------------------------------------------------
PUBLIC              C _yuv422_to_yuy2_mmx@16
;      void __stdcall  yuv422_to_yuy2_mmx(
; [esp + 4] = FRAME                    *top,
; [esp + 8] = FRAME                    *bottom,
; [esp +12] = unsigned char            *out,
; [esp +16] = CONVERSION_PARAMETER     *prm
; )
_yuv422_to_yuy2_mmx@16 PROC
;
;-------------------------------------------------------------------
; 使用するローカル変数
; 
; [esp+ 8] work
; [esp+40] y_src
; [esp+44] y_src_next
; [esp+48] u_src
; [esp+52] v_src
; [esp+56] u_src_next
; [esp+60] v_src_next
; [esp+64] out
; [esp+68] width/16
; [esp+72] width%16*3
; [esp+76] in_step*2
; [esp+80] out_step
; 
; total 84 + α
;-------------------------------------------------------------------
; レジスタの退避
		push       ebp
		push       edi
		push       esi
		push       edx
		push       ecx
		push       ebx
		push       eax
;-------------------------------------------------------------------
; ベースポインタにスタックアドレスを記憶
		mov        ebp, esp
;-------------------------------------------------------------------
; ローカル変数領域の確保
		sub        esp, 84
		and        esp, 0fffffff8h
;-------------------------------------------------------------------
; ループパラメータの作成
		mov        eax, [ebp+28+4] ; top
		mov        ebx, [ebp+28+8] ; bottom
		mov        edi, [ebp+28+12]
		mov        edx, [ebp+28+16]
		movd       mm0, dword ptr [eax+ 8] ; top luma
		movd       mm1, dword ptr [eax+12] ; top cb
		movd       mm2, dword ptr [eax+16] ; top cr
		movd       mm3, dword ptr [ebx+ 8] ; bottom luma
		movd       mm4, dword ptr [ebx+12] ; bottom cb
		movd       mm5, dword ptr [ebx+16] ; bottom cr
		mov        eax, [edx]    ; width
		movd       mm6, dword ptr [edx+8]  ; in_step
		mov        ebx, [edx+12] ; out_step
		movd       mm7, dword ptr [edx+16] ; c_offset
		mov        edx, [edx+4]  ; height
		mov        ecx, eax
		mov        [esp+80], ebx ; out_step
		and        eax, 0fh      ; width%16
		shr        ecx, 4        ; width/16
		punpckldq  mm1, mm2
		punpckldq  mm4, mm5
		punpckldq  mm6, mm6
		punpckldq  mm7, mm7
		mov        ebx, eax
		paddd      mm3, mm6      ; bottom y + in_step
		paddd      mm4, mm6      ; bottom uv + in_step
		paddd      mm1, mm7      ; top uv + c_offset
		shl        eax, 1
		paddd      mm4, mm7      ; bottom uv + in_step + c_offset
		punpckldq  mm0, mm3      ; y
		pslld      mm6, 1        ; in_step * 2
		add        eax, ebx      ; width%16*3
		movq       [esp+40], mm0
		movq       [esp+48], mm1
		movq       [esp+56], mm4
		mov        [esp+64], edi
		mov        [esp+68], ecx
		mov        [esp+72], eax
		movd       dword ptr [esp+76], mm6
		mov        esi, [esp+40]
		mov        eax, [esp+48]
		mov        ebx, [esp+52]
;-------------------------------------------------------------------
; 縦方向ループ
yuv422_to_yuy2_next_line:
;-------------------------------------------------------------------
; 横方向ループ
yuv422_to_yuy2_next_16_pixel:
;-------------------------------------------------------------------
; 変換コア
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+8]
		movd       mm3, dword ptr [esi+12]
		movd       mm4, dword ptr [eax]
		movd       mm5, dword ptr [eax+4]
		movd       mm6, dword ptr [ebx]
		movd       mm7, dword ptr [ebx+4]
		lea        esi, [esi+16]
		lea        eax, [eax+8]
		lea        ebx, [ebx+8]
		psllq      mm1, 32
		psllq      mm3, 32
		punpcklbw  mm4, mm6 ; v3_u3_v2_u2_v1_u1_v0_u0
		punpcklbw  mm5, mm7 ; v7_u7_v6_u6_v5_u5_v4_u4
		punpcklbw  mm0, mm4
		punpckhbw  mm1, mm4
		punpcklbw  mm2, mm5
		punpckhbw  mm3, mm5
		movq       [edi], mm0
		movq       [edi+8], mm1
		movq       [edi+16], mm2
		movq       [edi+24], mm3
		lea        edi, [edi+32]
;-------------------------------------------------------------------
; 横方向ループ終端チェック
		dec        ecx
		jnz        yuv422_to_yuy2_next_16_pixel
;-------------------------------------------------------------------
; 端数処理
		mov        ecx, [esp+72]
		test       ecx, ecx
		jz         yuv422_to_yuy2_line_end
; 端数処理コア
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+8]
		movd       mm3, dword ptr [esi+12]
		movd       mm4, dword ptr [eax]
		movd       mm5, dword ptr [eax+4]
		movd       mm6, dword ptr [ebx]
		movd       mm7, dword ptr [ebx+4]
		lea        esi, [esi+16]
		lea        eax, [eax+8]
		lea        ebx, [ebx+8]
		psllq      mm1, 32
		psllq      mm3, 32
		punpcklbw  mm4, mm6
		punpcklbw  mm5, mm7
		punpcklbw  mm0, mm4
		punpckhbw  mm1, mm4
		punpcklbw  mm2, mm5
		punpckhbw  mm3, mm5
		movq       [esp+8], mm0
		movq       [esp+16], mm1
		movq       [esp+24], mm2
		movq       [esp+32], mm3
		lea        esi, [esp+8]
		rep movsb
;-------------------------------------------------------------------
; 縦方向ループ終端チェック
yuv422_to_yuy2_line_end:
		movq       mm0, [esp+40]
		movq       mm1, [esp+48]
		movq       mm2, [esp+56]
		mov        edi, [esp+64]
		mov        ecx, [esp+68] ; width/16
		movd       mm3, dword ptr [esp+76]
		add        edi, [esp+80] ; out
		paddd      mm0, mm3
		punpckldq  mm3, mm3
		mov        [esp+64], edi ; out
		paddd      mm1, mm3
		movd       dword ptr [esp+44], mm0 ; next y
		psrlq      mm0, 32
		movq       [esp+48], mm2 ; current uv
		movq       [esp+56], mm1 ; next uv
		movd       esi, mm0      ; current y
		mov        eax, [esp+48]
		mov        ebx, [esp+52]
		mov        [esp+40], esi ; current y
		dec        edx
		jnz        yuv422_to_yuy2_next_line
;-------------------------------------------------------------------
; 後始末
		mov        esp, ebp

		pop        eax
		pop        ebx
		pop        ecx
		pop        edx
		pop        esi
		pop        edi
		pop        ebp

		ret        16
;
_yuv422_to_yuy2_mmx@16 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; yuy2_convert_mmx - YUY2 データの変換行列変更
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;-------------------------------------------------------------------
PUBLIC              C _yuy2_convert_mmx@16
;      void __stdcall  yuy2_convert_mmx(
; [esp + 4] = unsigned char             *yuy2
; [esp + 8] = int                        step
; [esp +12] = int                        height
; [esp +16] = YUY2_CONVERSION_PARAMETER *prm
; )
_yuy2_convert_mmx@16 PROC
;
;-------------------------------------------------------------------
; 使用するローカル変数
; 
; [esp+ 8] uv&vv 
; [esp+16] work[8]
; [esp+24] abs(step)/8
; [esp+28] p
;-------------------------------------------------------------------
; レジスタの退避
;
		push       ebp
		push       eax
		push       ebx
		push       ecx
		push       esi
		push       edi
;-------------------------------------------------------------------
; ベースポインタにスタックアドレスを記憶
		mov        ebp, esp
;-------------------------------------------------------------------
; ローカル変数領域の確保
		sub        esp, 32
		and        esp, 0fffffff8h
;-------------------------------------------------------------------
; 変数のセットアップ
		mov        esi, [ebp+24+4]
		mov        ecx, [ebp+24+8]
		mov        ebx, [ebp+24+12]
		mov        eax, [ebp+24+16]
		movq       mm5, [eax]
		movq       mm4, [eax+8]
		movq       mm3, [eax+16]
		mov        eax, ecx ; abs(step) phase-1
		pcmpeqw    mm2, mm2
		pxor       mm6, mm6
		pxor       mm7, mm7
		sar        ecx, 31  ; abs(step) phase-2
		psrad      mm3, 2
		psrad      mm4, 2
		psrad      mm5, 2
		xor        eax, ecx ; abs(step) phase-3
		psubw      mm6, mm2
		psubd      mm7, mm2
		sub        eax, ecx ; abs(step) phase-4
		packssdw   mm3, mm3
		packssdw   mm4, mm4
		packssdw   mm5, mm5
		mov        ecx, eax ; copy abs(step)
		psllw      mm6, 7        ; 128x4
		pslld      mm7, 12       ; 4096x2
		movq       [esp+8], mm3  ; vu&vv
		shr        ecx, 3   ; abs(step)/8
		and        eax, 7   ; abs(step)%8
		mov        [esp+24], ecx
;-------------------------------------------------------------------
; 縦方向ループ
yuy2_convert_mmx_v_head:
		test       ecx,ecx
		jz         yuy2_convert_mmx_h_tail
yuy2_convert_mmx_h_head:
		movq       mm0, [esi]
		movq       mm3, [esi]
		psrlw      mm0, 8   ; 00VB_00UB_00VA_00UA
		psllw      mm3, 8   ; YD00_YC00_YB00_YA00
		psubw      mm0, mm6 ; UV-128
		psrlw      mm3, 8   ; 00YD_00YC_00YB_00YA
		movq       mm1, mm0
		movq       mm2, mm0
		pmaddwd    mm0, mm5
		pmaddwd    mm1, mm4
		pmaddwd    mm2, [esp+8]
		paddd      mm0, mm7
		paddd      mm1, mm7
		paddd      mm2, mm7
		psrad      mm0, 14
		psrad      mm1, 14
		psrad      mm2, 14
		packssdw   mm0, mm0 ; YB_YA_YB_YA
		packssdw   mm1, mm1 ; UB_UA_UB_UA
		packssdw   mm2, mm2 ; VB_VA_VB_VA
		punpcklwd  mm0, mm0 ; YB_YB_YA_YA
		punpcklwd  mm1, mm2 ; VB_UB_VA_UA
		paddw      mm0, mm3 ; YD_YC_YB_YA
		paddw      mm1, mm6 ; UV+128
		packuswb   mm0, mm0 ; YD_YC_YB_YA_YD_YC_YB_YA
		packuswb   mm1, mm1 ; VB_UB_VA_UA_VB_UB_VA_UA
		punpcklbw  mm0, mm1 ; VB_YD_UB_YC_VA_YB_UA_YA
		movq       [esi], mm0
		add        esi, 8
		dec        ecx
		jnz        yuy2_convert_mmx_h_head
yuy2_convert_mmx_h_tail:
		test       eax,eax
		jz         yuy2_convert_mmx_h_last
		mov        [esp+28], esi
		lea        edi, [esp+16]
		mov        ecx, eax
		rep movsb
		movq       mm0, [esp+16]
		movq       mm3, [esp+16]
		psrlw      mm0, 8   ; 00VB_00UB_00VA_00UA
		psllw      mm3, 8   ; YD00_YC00_YB00_YA00
		psubw      mm0, mm6 ; UV-128
		psrlw      mm3, 8   ; 00YD_00YC_00YB_00YA
		movq       mm1, mm0
		movq       mm2, mm0
		pmaddwd    mm0, mm5
		pmaddwd    mm1, mm4
		pmaddwd    mm2, [esp+8]
		paddd      mm0, mm7
		paddd      mm1, mm7
		paddd      mm2, mm7
		psrad      mm0, 14
		psrad      mm1, 14
		psrad      mm2, 14
		packssdw   mm0, mm0 ; YB_YA_YB_YA
		packssdw   mm1, mm1 ; UB_UA_UB_UA
		packssdw   mm2, mm2 ; VB_VA_VB_VA
		punpcklwd  mm0, mm0 ; YB_YB_YA_YA
		punpcklwd  mm1, mm2 ; VB_UB_VA_UA
		paddw      mm0, mm3 ; YD_YC_YB_YA
		paddw      mm1, mm6 ; UV+128
		packuswb   mm0, mm0 ; YD_YC_YB_YA_YD_YC_YB_YA
		packuswb   mm1, mm1 ; VB_UB_VA_UA_VB_UB_VA_UA
		punpcklbw  mm0, mm1 ; VB_YD_UB_YC_VA_YB_UA_YA
		movq       [esp+16], mm0
		lea        esi, [esp+16]
		mov        edi, [esp+28]
		mov        ecx, eax
		rep        movsb
yuy2_convert_mmx_h_last:
		mov        ecx, [esp+24]
		mov        esi, [ebp+24+4]
		add        esi, [ebp+24+8]
		mov        [ebp+24+4], esi
		dec        ebx
		jnz        yuy2_convert_mmx_v_head
;-------------------------------------------------------------------
; 後始末
		mov        esp, ebp

		pop        edi
		pop        esi
		pop        ecx
		pop        ebx
		pop        eax
		pop        ebp

		ret        16
_yuy2_convert_mmx@16 ENDP
;-------------------------------------------------------------------
; 終了


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; _TEXT64 セグメントの終了
END
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

