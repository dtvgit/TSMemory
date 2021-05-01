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
		dq 00007000700070007h ;     7,     7,     7,     7,
int16_all5      dq 00005000500050005h ;     5,     5,     5,     5,
		dq 00005000500050005h ;     5,     5,     5,     5,
int16_all4      dq 00004000400040004h ;     4,     4,     4,     4,
		dq 00004000400040004h ;     4,     4,     4,     4,
int16_all3      dq 00003000300030003h ;     3,     3,     3,     3,
		dq 00003000300030003h ;     3,     3,     3,     3,
int16_all2      dq 00002000200020002h ;     2,     2,     2,     2,
		dq 00002000200020002h ;     2,     2,     2,     2,

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; chroma420i_to_422_sse2 - インタレース 420 から 422 への補間
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (y*4+2) = ((y*2  ) * 5 + (y*2+2) * 3 )>> 3
; (y*4+3) = ((y*2+1) * 7 + (y*2+3) * 1 )>> 3
; (y*4+4) = ((y*2  ) * 1 + (y*2+2) * 7 )>> 3
; (y*4+5) = ((y*2+1) * 3 + (y*2+3) * 5 )>> 3
;-------------------------------------------------------------------
PUBLIC              C _chroma420i_to_422_sse2@12
;      void __stdcall  chroma420i_to_422_sse2(
; [esp + 4] = unsigned char *data,
; [esp + 8] = int            width,
; [esp +12] = int            height
; )
;
_chroma420i_to_422_sse2@12 PROC
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
; width/16     - ecx & [esp+12]
; width*2-8    - ebx
; width/2*7    - [esp+4]
; width/2*3    - [esp+8]
; data+width/2 - edi
; all 0        - xmm7
; all 4        - xmm6
		mov        ecx, edx
		mov        ebx, edx
		movdqa     xmm6, xmmword ptr int16_all4
		pxor       xmm7, xmm7
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
		shr        ecx, 4
		shl        ebx, 1
		mov        [esp+12], ecx
		sub        ebx, 8
		shl        ecx, 1
;-------------------------------------------------------------------
; 補間例外 - トップ２行そのままコピー
		cld
		rep movsd
		mov        ecx, esi
		mov        esi, edi
		lea        edi, [ecx+edx]
		mov        ecx, [esp+12]
		shl        ecx, 1
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
; xmm7 - all 0
; xmm6 - all 4
		movq       xmm0, qword ptr [esi]      ; c[0][x]
		add        esi, edx;
		movq       xmm1, qword ptr [esi]      ; c[1][x]  
		add        esi, edx;
		movq       xmm2, qword ptr [esi]      ; c[2][x]
		movq       xmm3, qword ptr [esi+edx]  ; c[3][x]
		movdqa     xmm4, xmmword ptr int16_all7 ; all 7
		punpcklbw  xmm0, xmm7
		punpcklbw  xmm1, xmm7
		punpcklbw  xmm2, xmm7
		punpcklbw  xmm3, xmm7
		movdqa     xmm5, xmm4       ; all 7
		pmullw     xmm4, xmm1       ; 7*c[1][x]
		pmullw     xmm1, xmmword ptr int16_all3 ; 3*c[1][x]
		pmullw     xmm5, xmm2       ; 7*c[2][x]
		pmullw     xmm2, xmmword ptr int16_all3 ; 3*c[2][x]
		paddw      xmm4, xmm3       ; 7*c[1][x] + 1*c[3][x]
		paddw      xmm5, xmm0       ; 7*c[2][x] + 1*c[0][x]
		pmullw     xmm0, xmmword ptr int16_all5 ; 5*c[0][x]
		pmullw     xmm3, xmmword ptr int16_all5 ; 5*c[3][x]
		paddw      xmm4, xmm6       ; 7*c[1][x] + 1*c[3][x] + 4
		paddw      xmm5, xmm6       ; 7*c[2][x] + 1*c[0][x] + 4
		paddw      xmm0, xmm2       ; 5*c[0][x] + 3*c[2][x]
		paddw      xmm3, xmm1       ; 5*c[3][x] + 3*c[1][x]
		paddw      xmm0, xmm6       ; 5*c[0][x] + 3*c[2][x] + 4
		paddw      xmm3, xmm6       ; 5*c[3][x] + 3*c[1][x] + 4
		psraw      xmm0, 3;
		psraw      xmm4, 3;
		psraw      xmm5, 3;
		psraw      xmm3, 3;
		packuswb   xmm0, xmm5;
		packuswb   xmm4, xmm3;
		movq       qword ptr [edi], xmm0;
		add        edi, edx;
		movq       qword ptr [edi], xmm4;
		add        edi, edx;
		psrldq     xmm0, 8;
		psrldq     xmm4, 8;
		movq       qword ptr [edi], xmm0;
		movq       qword ptr [edi+edx], xmm4;
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
		shl        ecx, 1
		rep movsd
		lea        esi, [esi+edx]
		lea        edi, [edi+edx]
		mov        ecx, [esp+12]
		shl        ecx, 1
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
_chroma420i_to_422_sse2@12 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; chroma420p_to_422_sse2 - プログレッシブ 420 から 422 への補間
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; [o1] = ([i0] * 3 + [i1]) / 4
; [o2] = ([i0] + [i1] * 3) / 4
;-------------------------------------------------------------------
PUBLIC              C _chroma420p_to_422_sse2@12
;      void __stdcall  chroma420p_to_422_sse2(
; [esp + 4] = unsigned char *data,
; [esp + 8] = int            width,
; [esp +12] = int            height
; )
;
_chroma420p_to_422_sse2@12 PROC
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
; all 2        - xmm6
; all 0        - xmm7
; 
		mov        eax, edx
		mov        ecx, edx
		shr        ebx, 1
		shr        eax, 1
		shr        ecx, 3
		movdqa     xmm6, xmmword ptr int16_all2
		pxor       xmm7, xmm7
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
chroma420p_to_422_sse2_next_2_line:
;-------------------------------------------------------------------
; 横方向ループ
chroma420p_to_422_sse2_next_8_pixel:
;-------------------------------------------------------------------
; 補間コア
; 
; ココでのパラメータ
; esi - 入力アドレス
; edi - 出力アドレス
; edx - 幅
; xmm6 - all 2
; xmm7 - all 0
		movq       xmm1, qword ptr [esi]            ; i[0][x]
		movq       xmm3, qword ptr [esi+edx]        ; i[1][x]
		add        esi, 8
		punpcklbw  xmm1, xmm7             ; i[0][x]
		punpcklbw  xmm3, xmm7             ; i[1][x]
		movdqa     xmm2, xmm1             ; i[0][x]
		movdqa     xmm0, xmm3             ; i[1][x]
		psllw      xmm1, 1                ; i[0][x]*2
		psllw      xmm3, 1                ; i[1][x]*2
		paddw      xmm1, xmm0             ; i[0][x]*2 + i[1][x]
		paddw      xmm0, xmm3             ; i[1][x]*3
		paddw      xmm1, xmm2             ; i[0][x]*3 + i[1][x]
		paddw      xmm0, xmm2             ; i[1][x]*3 + i[0][x]
		paddw      xmm1, xmm6             ; i[0][x]*3 + i[1][x] + 2
		paddw      xmm0, xmm6             ; i[1][x]*3 + i[0][x] + 2
		psraw      xmm1, 2
		psraw      xmm0, 2
		packuswb   xmm1, xmm7             ; o[0][0:7]
		packuswb   xmm0, xmm7             ; o[1][0:7]
		movq       qword ptr [edi], xmm1
		movq       qword ptr [edi+edx], xmm0
		add        edi, 8
;-------------------------------------------------------------------
; 横方向ループ終端チェック
		dec        ecx
		jnz        chroma420p_to_422_sse2_next_8_pixel
;-------------------------------------------------------------------
; 縦方向ループ終端チェック
		mov        ecx, edx
		add        edi, edx
		lea        esi, [esi+eax]
		lea        edi, [edi+eax]
		shr        ecx, 4
		dec        ebx
		jnz        chroma420p_to_422_sse2_next_2_line
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
_chroma420p_to_422_sse2@12 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; yuv422_to_bgr_sse2 - YUV -> RGB 変換
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;-------------------------------------------------------------------
PUBLIC              C _yuv422_to_bgr_sse2@16
;      void __stdcall  yuv422_to_bgr_sse2(
; [esp + 4] = FRAME                    *top,
; [esp + 8] = FRAME                    *bottom,
; [esp +12] = unsigned char            *out,
; [esp +16] = BGR_CONVERSION_PARAMETER *prm
; )
_yuv422_to_bgr_sse2@16 PROC
;-------------------------------------------------------------------
; 使用するローカル変数
; 
; [esp+  8] - work
; [esp+ 32] - y_gain
; [esp+ 48] - half_13
; [esp+ 64] - bu
; [esp+ 80] - guv
; [esp+ 96] - rv
; [esp+112] - width/8
; [esp+116] - width%8*3
; [esp+120] - in_step*2
; [esp+124] - out_step
;
; total 128 + α
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
		sub        esp, 128
		and        esp, 0fffffff0h
;-------------------------------------------------------------------
; 変換係数の作成
;
; 作成すべきパラメータ
;
; y_offset   - mm0
; c_offset   - mm1
; y_gain     - [esp+32]
; half_13    - [esp+48]
; bu         - [esp+64]
; guv        - [esp+80]
; rv         - [esp+96]
; y_src      - esi & mm2
; u_src      - eax & mm3
; v_src      - ebx & mm4
; y_src_next - mm5
; u_src_next - mm6
; v_src_next - mm7
; width/8    - ecx & [esp+112]
; width%8*3  - [esp+116]
; height     - edx
; in_step*2  - [esp+120]
; out_step   - [esp+124]
;
		mov        edx, [ebp+28+16]
		pcmpeqw    mm0, mm0
		pxor       xmm0, xmm0
		pxor       mm1, mm1
		pcmpeqw    xmm1, xmm1
		movd       mm2, dword ptr [edx+20]  ; y_offset
		movd       xmm2, dword ptr [edx+24] ; y_gain
		movd       xmm3, dword ptr [edx+28] ; bu
		movd       xmm4, dword ptr [edx+32] ; gu
		movd       xmm5, dword ptr [edx+36] ; gv
		movd       xmm6, dword ptr [edx+40] ; rv
		psubw      mm1, mm0       ; all 1
		psubd      xmm0, xmm1     ; all 1
		pshufw     mm0, mm2, 0    ; yo_yo_yo_yo
		psllw      mm1, 7         ; c_offset
		movdqa     xmm1, xmm0     ; all 1
		pshufd     xmm2, xmm2, 0  ; yg_yg_yg_yg
		pshufd     xmm3, xmm3, 0  ; bu_bu_bu_bu
		punpckldq  xmm4, xmm5     ; xx_xx_gv_gu
		pshufd     xmm6, xmm6, 0  ; rv_rv_rv_rv
		pslld      xmm0, 2        ; half_3
		pslld      xmm1, 12       ; half_13
		pshufd     xmm4, xmm4, 01000100b
		paddd      xmm2, xmm0
		paddd      xmm3, xmm0
		paddd      xmm4, xmm0
		paddd      xmm6, xmm0
		psrad      xmm2, 3
		psrad      xmm3, 3
		psrad      xmm4, 3
		psrad      xmm6, 3
		packssdw   xmm4, xmm4
		movdqa     [esp+32], xmm2
		movdqa     [esp+48], xmm1
		movdqa     [esp+64], xmm3
		movdqa     [esp+80], xmm4
		movdqa     [esp+96], xmm6
		mov        eax, [edx] ; width
		mov        ecx, [edx] ; width
		movd       mm7, dword ptr [edx+8] ; in_step
		movd       mm4, dword ptr [edx+16] ; c_offset
		mov        esi, [ebp+28+4] ; top
		mov        edi, [ebp+28+8] ; bottom
		punpckldq  mm7, mm7
		punpckldq  mm4, mm4
		movd       mm2, dword ptr [esi+ 8] ; top y
		movq       mm3, [esi+12] ; top uv
		movd       mm5, dword ptr [edi+ 8] ; bottom y
		movq       mm6, [edi+12] ; bottom uv
		shr        ecx, 3
		and        eax, 7
		paddd      mm5, mm7
		mov        ebx, eax
		paddd      mm6, mm7
		shl        eax, 1
		paddd      mm3, mm4
		add        eax, ebx
		paddd      mm6, mm4
		mov        ebx, [edx+12] ; out_step
		mov        edx, [edx+4] ; height
		pslld      mm7, 1
		mov        [esp+112], ecx
		mov        [esp+116], eax
		movd       dword ptr [esp+120], mm7
		mov        [esp+124], ebx
		movq       mm4, mm3
		movq       mm7, mm6
		psrlq      mm4, 32
		psrlq      mm7, 32
		movd       esi, mm2
		movd       eax, mm3
		movd       ebx, mm4
		mov        edi, [ebp+28+12]
;-------------------------------------------------------------------
; 縦方向ループ
yuv422_to_bgr_next_line:
;-------------------------------------------------------------------
; 横方向ループ
yuv422_to_bgr_next_8_pixel:
;-------------------------------------------------------------------
; 変換コア
;
		movd       xmm0, dword ptr [esi]
		movd       xmm1, dword ptr [esi+4]
		movd       xmm2, dword ptr [eax]
		movd       xmm4, dword ptr [ebx]
		movq2dq    xmm5, mm0 ; y_offset
		movq2dq    xmm6, mm1 ; c_offset
		pxor       xmm7, xmm7
		lea        esi, [esi+8]
		lea        eax, [eax+4]
		lea        ebx, [ebx+4]
		punpcklbw  xmm0, xmm7
		punpcklbw  xmm1, xmm7
		punpcklbw  xmm2, xmm7
		punpcklbw  xmm4, xmm7
		psubw      xmm0, xmm5
		psubw      xmm1, xmm5
		psubw      xmm2, xmm6
		psubw      xmm4, xmm6
		movdqa     xmm5, [esp+32] ; y_gain
		movdqa     xmm6, [esp+48] ; half_13
		movdqa     xmm3, xmm2
		punpcklwd  xmm0, xmm7
		punpcklwd  xmm1, xmm7
		punpcklwd  xmm2, xmm7
		punpcklwd  xmm3, xmm4
		punpcklwd  xmm4, xmm7
		pmaddwd    xmm0, xmm5
		pmaddwd    xmm1, xmm5
		pmaddwd    xmm2, [esp+64] ; bu
		pmaddwd    xmm3, [esp+80] ; guv
		pmaddwd    xmm4, [esp+96] ; rv
		paddd      xmm0, xmm6
		paddd      xmm1, xmm6		
		movdqa     xmm5, xmm2
		movdqa     xmm6, xmm3
		movdqa     xmm7, xmm4
		punpckldq  xmm2, xmm2
		punpckldq  xmm3, xmm3
		punpckldq  xmm4, xmm4
		punpckhdq  xmm5, xmm5
		punpckhdq  xmm6, xmm6
		punpckhdq  xmm7, xmm7
		paddd      xmm2, xmm0
		paddd      xmm3, xmm0
		paddd      xmm4, xmm0
		paddd      xmm5, xmm1
		paddd      xmm6, xmm1
		paddd      xmm7, xmm1
		psrad      xmm2, 13
		psrad      xmm3, 13
		psrad      xmm4, 13
		psrad      xmm5, 13
		psrad      xmm6, 13
		psrad      xmm7, 13
		packuswb   xmm2, xmm5 ; b
		packuswb   xmm3, xmm6 ; g
		packuswb   xmm4, xmm7 ; r
		movdqa     xmm5, xmm2 ; b'
		movdqa     xmm6, xmm3 ; g'
		movdqa     xmm7, xmm4 ; r'
		psrldq     xmm2, 1
		pslldq     xmm3, 1
		pslldq     xmm4, 1
		por        xmm7, xmm2 ; xAx7x4x1
		por        xmm5, xmm3 ; x9x6x3x0
		por        xmm6, xmm4 ; Bx8x5x2x
		pshufd     xmm7, xmm7, 11011000b ; xAx4x7x1
		pshufd     xmm5, xmm5, 11011000b ; x9x3x6x0
		pshufd     xmm6, xmm6, 11011000b ; Bx5x8x2x
		movdqa     xmm4, xmm5
		psrldq     xmm6, 2 ; xBx5x8x2
		punpcklwd  xmm5, xmm7 ; xx76xx10
		psrldq     xmm4, 8 ; xxxxx9x3
		punpckhwd  xmm7, xmm6 ; xxBAxx54
		punpcklwd  xmm6, xmm4 ; xx98xx32
		movd       dword ptr [edi], xmm5
		movd       dword ptr [edi+4], xmm6
		movd       dword ptr [edi+8], xmm7
		psrldq     xmm5, 8
		psrldq     xmm6, 8
		psrldq     xmm7, 8
		movd       dword ptr [edi+12], xmm5
		movd       dword ptr [edi+16], xmm6
		movd       dword ptr [edi+20], xmm7
		lea        edi, [edi+24]
;-------------------------------------------------------------------
; 横方向ループ終端チェック
		dec        ecx
		jnz        yuv422_to_bgr_next_8_pixel
;-------------------------------------------------------------------
; 端数処理
		mov        ecx, [esp+116] ; width%8*3
		test       ecx, ecx
		jz         yuv422_to_bgr_line_end
		movd       xmm0, dword ptr [esi]
		movd       xmm1, dword ptr [esi+4]
		movd       xmm2, dword ptr [eax]
		movd       xmm4, dword ptr [ebx]
		movq2dq    xmm5, mm0 ; y_offset
		movq2dq    xmm6, mm1 ; c_offset
		pxor       xmm7, xmm7
		punpcklbw  xmm0, xmm7
		punpcklbw  xmm1, xmm7
		punpcklbw  xmm2, xmm7
		punpcklbw  xmm4, xmm7
		psubw      xmm0, xmm5
		psubw      xmm1, xmm5
		psubw      xmm2, xmm6
		psubw      xmm4, xmm6
		movdqa     xmm5, [esp+32] ; y_gain
		movdqa     xmm6, [esp+48] ; half_13
		movdqa     xmm3, xmm2
		punpcklwd  xmm0, xmm7
		punpcklwd  xmm1, xmm7
		punpcklwd  xmm2, xmm7
		punpcklwd  xmm3, xmm4
		punpcklwd  xmm4, xmm7
		pmaddwd    xmm0, xmm5
		pmaddwd    xmm1, xmm5
		pmaddwd    xmm2, [esp+64] ; bu
		pmaddwd    xmm3, [esp+80] ; guv
		pmaddwd    xmm4, [esp+96] ; rv
		paddd      xmm0, xmm6
		paddd      xmm1, xmm6		
		movdqa     xmm5, xmm2
		movdqa     xmm6, xmm3
		movdqa     xmm7, xmm4
		punpckldq  xmm2, xmm2
		punpckldq  xmm3, xmm3
		punpckldq  xmm4, xmm4
		punpckhdq  xmm5, xmm5
		punpckhdq  xmm6, xmm6
		punpckhdq  xmm7, xmm7
		paddd      xmm2, xmm0
		paddd      xmm3, xmm0
		paddd      xmm4, xmm0
		paddd      xmm5, xmm1
		paddd      xmm6, xmm1
		paddd      xmm7, xmm1
		psrad      xmm2, 13
		psrad      xmm3, 13
		psrad      xmm4, 13
		psrad      xmm5, 13
		psrad      xmm6, 13
		psrad      xmm7, 13
		packuswb   xmm2, xmm5 ; b
		packuswb   xmm3, xmm6 ; g
		packuswb   xmm4, xmm7 ; r
		movdqa     xmm5, xmm2 ; b'
		movdqa     xmm6, xmm3 ; g'
		movdqa     xmm7, xmm4 ; r'
		pcmpeqw    xmm0, xmm0
		pcmpeqw    xmm1, xmm1
		psrldq     xmm2, 1
		pslldq     xmm3, 1
		pslldq     xmm4, 1
		psrld      xmm0, 16
		pslld      xmm1, 16
		por        xmm7, xmm2 ; xAx7x4x1
		por        xmm5, xmm3 ; x9x6x3x0
		por        xmm6, xmm4 ; Bx8x5x2x
		pshufd     xmm7, xmm7, 11011000b ; xAx4x7x1
		pshufd     xmm5, xmm5, 11011000b ; x9x3x6x0
		pshufd     xmm6, xmm6, 11011000b ; Bx5x8x2x
		movdqa     xmm4, xmm5
		psrldq     xmm6, 2 ; xBx5x8x2
		punpcklwd  xmm5, xmm7 ; xx76xx10
		psrldq     xmm4, 8 ; xxxxx9x3
		punpckhwd  xmm7, xmm6 ; xxBAxx54
		punpcklwd  xmm6, xmm4 ; xx98xx32
		movd       dword ptr [esp+8], xmm5
		movd       dword ptr [esp+12], xmm6
		movd       dword ptr [esp+16], xmm7
		psrldq     xmm5, 8
		psrldq     xmm6, 8
		psrldq     xmm7, 8
		movd       dword ptr [esp+20], xmm5
		movd       dword ptr [esp+24], xmm6
		movd       dword ptr [esp+28], xmm7
		lea        esi, [esp+8]
		rep movsb
;-------------------------------------------------------------------
; 縦方向ループ終端チェック
yuv422_to_bgr_line_end:
		mov        ecx, [esp+120]
		movd       esi, mm2
		movd       eax, mm3
		movd       ebx, mm4
		movq       mm2, mm5
		movq       mm3, mm6
		movq       mm4, mm7
		add        esi, ecx
		add        eax, ecx
		add        ebx, ecx
		mov        ecx, [esp+124]
		mov        edi, [ebp+28+12]
		movd       mm5, esi
		movd       mm6, eax
		movd       mm7, ebx
		movd       esi, mm2
		movd       eax, mm3
		movd       ebx, mm4
		add        edi, ecx
		mov        [ebp+28+12], edi
		mov        ecx,[esp+112]
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
_yuv422_to_bgr_sse2@16 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; yuv422_to_yuy2_sse2 - YUV -> YUY2 変換
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;-------------------------------------------------------------------
PUBLIC              C _yuv422_to_yuy2_sse2@16
;      void __stdcall  yuv422_to_yuy2_sse2(
; [esp + 4] = FRAME                    *top,
; [esp + 8] = FRAME                    *bottom,
; [esp +12] = unsigned char            *out,
; [esp +16] = CONVERSION_PARAMETER     *prm
; )
_yuv422_to_yuy2_sse2@16 PROC
;
;-------------------------------------------------------------------
; 使用するローカル変数
; 
; [esp+ 4] work
; [esp+36] out
; [esp+40] width/16
; [esp+44] width%16*3
; [esp+48] in_step*2
; [esp+52] out_step
; 
; total 88 + α
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
		sub        esp, 88
		and        esp, 0fffffff0h
		sub        esp, 4
;-------------------------------------------------------------------
; ループパラメータの作成
		mov        eax, [ebp+28+4]
		mov        ebx, [ebp+28+8]
		mov        edi, [ebp+28+12]
		mov        edx, [ebp+28+16]
		movd       mm0, dword ptr [eax+ 8]
		movq       mm1, qword ptr [eax+12]
		movd       mm3, dword ptr [ebx+ 8]
		movq       mm4, qword ptr [ebx+12]
		mov        eax, [edx]    ; width
		movd       mm6, dword ptr [edx+8]  ; in_step
		mov        ebx, [edx+12] ; out_step
		movd       mm7, dword ptr [edx+16] ; c_offset
		mov        edx, [edx+4]  ; height
		mov        ecx, eax
		mov        [esp+52], ebx ; out_step
		and        eax, 0fh      ; width%16
		shr        ecx, 4        ; width/16
		punpckldq  mm6, mm6
		punpckldq  mm7, mm7
		mov        ebx, eax
		paddd      mm3, mm6      ; bottom y + in_step
		paddd      mm4, mm6      ; bottom uv + in_step
		paddd      mm1, mm7      ; top uv + c_offset
		shl        eax, 1
		paddd      mm4, mm7      ; bottom uv + in_step + c_offset
		pslld      mm6, 1        ; in_step * 2
		add        eax, ebx      ; width%16*3
		movq       mm2, mm1
		movq       mm5, mm4
		psrlq      mm2, 32
		psrlq      mm5, 32
		mov        [esp+36], edi
		mov        [esp+40], ecx
		mov        [esp+44], eax
		movd       dword ptr [esp+48], mm6
		movd       esi, mm0
		movd       eax, mm1
		movd       ebx, mm2
;-------------------------------------------------------------------
; 縦方向ループ
yuv422_to_yuy2_next_line:
;-------------------------------------------------------------------
; 横方向ループ
yuv422_to_yuy2_next_16_pixel:
;-------------------------------------------------------------------
; 変換コア
		movq       xmm0, qword ptr [esi]
		movq       xmm1, qword ptr [esi+8]
		movd       xmm4, dword ptr [eax]
		movd       xmm5, dword ptr [eax+4]
		movd       xmm6, dword ptr [ebx]
		movd       xmm7, dword ptr [ebx+4]
		lea        esi, [esi+16]
		lea        eax, [eax+8]
		lea        ebx, [ebx+8]
		punpcklbw  xmm4, xmm6
		punpcklbw  xmm5, xmm7
		punpcklbw  xmm0, xmm4
		punpcklbw  xmm1, xmm5
		movdqu     [edi], xmm0
		movdqu     [edi+16], xmm1
		lea        edi, [edi+32]
;-------------------------------------------------------------------
; 横方向ループ終端チェック
		dec        ecx
		jnz        yuv422_to_yuy2_next_16_pixel
;-------------------------------------------------------------------
; 端数処理
		mov        ecx, [esp+44]
		test       ecx, ecx
		jz         yuv422_to_yuy2_line_end
; 端数処理コア
		movq       xmm0, qword ptr [esi]
		movq       xmm1, qword ptr [esi+8]
		movd       xmm4, dword ptr [eax]
		movd       xmm5, dword ptr [eax+4]
		movd       xmm6, dword ptr [ebx]
		movd       xmm7, dword ptr [ebx+4]
		punpcklbw  xmm4, xmm6
		punpcklbw  xmm5, xmm7
		punpcklbw  xmm0, xmm4
		punpcklbw  xmm1, xmm5
		movdqa     [esp+4], xmm0
		movdqa     [esp+20], xmm1
		lea        esi, [esp+4]
		rep movsb
;-------------------------------------------------------------------
; 縦方向ループ終端チェック
yuv422_to_yuy2_line_end:
		mov        ecx, [esp+48]
		movd       esi, mm0
		movd       eax, mm1
		movd       ebx, mm2
		add        esi, ecx
		add        eax, ecx
		add        ebx, ecx
		mov        ecx, [esp+52]
		mov        edi, [esp+36]
		movq       mm0, mm3
		movq       mm1, mm4
		movq       mm2, mm5
		movd       mm3, esi
		movd       mm4, eax
		movd       mm5, ebx
		movd       esi, mm0
		movd       eax, mm1
		movd       ebx, mm2
		add        edi, ecx
		mov        ecx, [esp+40]
		mov        [esp+36], edi
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
_yuv422_to_yuy2_sse2@16 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; yuy2_convert_sse2 - YUY2 データの変換行列変更
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;-------------------------------------------------------------------
PUBLIC              C _yuy2_convert_sse2@16
;      void __stdcall  yuy2_convert_sse2(
; [esp + 4] = unsigned char             *yuy2
; [esp + 8] = int                        step
; [esp +12] = int                        height
; [esp +16] = YUY2_CONVERSION_PARAMETER *prm
; )
_yuy2_convert_sse2@16 PROC
;
;-------------------------------------------------------------------
; 使用するローカル変数
; 
; [esp+16] uv&vv
; [esp+32] work[16]
; [esp+48] abs(step)/16
; [esp+52] p
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
		sub        esp, 56
		and        esp, 0fffffff0h
;-------------------------------------------------------------------
; 変数のセットアップ
		mov        esi, [ebp+24+4]
		mov        ecx, [ebp+24+8]
		mov        ebx, [ebp+24+12]
		mov        eax, [ebp+24+16]
		movq       xmm5, qword ptr [eax]
		movq       xmm4, qword ptr [eax+8]
		movq       xmm3, qword ptr [eax+16]
		mov        eax, ecx ; abs(step) phase-1
		pcmpeqw    xmm2, xmm2
		pxor       xmm6, xmm6
		pxor       xmm7, xmm7
		sar        ecx, 31  ; abs(step) phase-2
		pshufd     xmm3, xmm3, 01000100b
		pshufd     xmm4, xmm4, 01000100b
		pshufd     xmm5, xmm5, 01000100b
		psrad      xmm3, 2
		psrad      xmm4, 2
		psrad      xmm5, 2
		xor        eax, ecx ; abs(step) phase-3
		psubw      xmm6, xmm2
		psubd      xmm7, xmm2
		sub        eax, ecx ; abs(step) phase-4
		packssdw   xmm3, xmm3
		packssdw   xmm4, xmm4
		packssdw   xmm5, xmm5
		mov        ecx, eax ; copy abs(step)
		psllw      xmm6, 7        ; 128x8
		pslld      xmm7, 12       ; 4096x4
		movdqa     [esp+16], xmm3 ; vu&vv
		shr        ecx, 4   ; abs(step)/16
		and        eax, 15  ; abs(step)%16
		mov        [esp+48], ecx
;-------------------------------------------------------------------
; 縦方向ループ
yuy2_convert_sse2_v_head:
		test       ecx,ecx
		jz         yuy2_convert_sse2_h_tail
yuy2_convert_sse2_h_head:
		movdqu     xmm0, [esi]
		movdqu     xmm3, [esi]
		psrlw      xmm0, 8
		psllw      xmm3, 8
		psubw      xmm0, xmm6
		psrlw      xmm3, 8
		movdqa     xmm1, xmm0
		movdqa     xmm2, xmm0
		pmaddwd    xmm0, xmm5
		pmaddwd    xmm1, xmm4
		pmaddwd    xmm2, [esp+16]
		paddd      xmm0, xmm7
		paddd      xmm1, xmm7
		paddd      xmm2, xmm7
		psrad      xmm0, 14
		psrad      xmm1, 14
		psrad      xmm2, 14
		packssdw   xmm0, xmm0 ; YD_YC_YB_YA_YD_YC_YB_YA
		packssdw   xmm1, xmm1 ; VD_VC_VB_VA_VD_VC_VB_VA
		packssdw   xmm2, xmm2 ; UD_UC_UB_UA_UD_UC_UB_UA
		punpcklwd  xmm0, xmm0 ; YD_YD_YC_YC_YB_YB_YA_YA
		punpcklwd  xmm1, xmm2 ; VD_UD_VC_UC_VB_UB_VA_UA
		paddw      xmm0, xmm3 ; YH_YG_YF_YE_YD_YC_YB_YA
		paddw      xmm1, xmm6 ; UV+128
		packuswb   xmm0, xmm0 ; YD_YC_YB_YA_YD_YC_YB_YA
		packuswb   xmm1, xmm1 ; VB_UB_VA_UA_VB_UB_VA_UA
		punpcklbw  xmm0, xmm1 ; VB_YD_UB_YC_VA_YB_UA_YA
		movdqu     [esi], xmm0
		add        esi, 16
		dec        ecx
		jnz        yuy2_convert_sse2_h_head
yuy2_convert_sse2_h_tail:
		test       eax,eax
		jz         yuy2_convert_sse2_h_last
		mov        [esp+52], esi
		lea        edi, [esp+32]
		mov        ecx, eax
		rep movsb
		movdqu     xmm0, [esp+32]
		movdqu     xmm3, [esi+32]
		psrlw      xmm0, 8
		psllw      xmm3, 8
		psubw      xmm0, xmm6
		psrlw      xmm3, 8
		movdqa     xmm1, xmm0
		movdqa     xmm2, xmm0
		pmaddwd    xmm0, xmm5
		pmaddwd    xmm1, xmm4
		pmaddwd    xmm2, [esp+16]
		paddd      xmm0, xmm7
		paddd      xmm1, xmm7
		paddd      xmm2, xmm7
		psrad      xmm0, 14
		psrad      xmm1, 14
		psrad      xmm2, 14
		packssdw   xmm0, xmm0 ; YD_YC_YB_YA_YD_YC_YB_YA
		packssdw   xmm1, xmm1 ; VD_VC_VB_VA_VD_VC_VB_VA
		packssdw   xmm2, xmm2 ; UD_UC_UB_UA_UD_UC_UB_UA
		punpcklwd  xmm0, xmm0 ; YD_YD_YC_YC_YB_YB_YA_YA
		punpcklwd  xmm1, xmm2 ; VD_UD_VC_UC_VB_UB_VA_UA
		paddw      xmm0, xmm3 ; YH_YG_YF_YE_YD_YC_YB_YA
		paddw      xmm1, xmm6 ; UV+128
		packuswb   xmm1, xmm1 ; VB_UB_VA_UA_VB_UB_VA_UA
		packuswb   xmm0, xmm0 ; YD_YC_YB_YA_YD_YC_YB_YA
		punpcklbw  xmm0, xmm1 ; VB_YD_UB_YC_VA_YB_UA_YA
		movdqu     [esp+32], xmm0
		lea        esi, [esp+32]
		mov        edi, [esp+52]
		mov        ecx, eax
		rep        movsb
yuy2_convert_sse2_h_last:
		mov        ecx, [esp+48]
		mov        esi, [ebp+24+4]
		add        esi, [ebp+24+8]
		mov        [ebp+24+4], esi
		dec        ebx
		jnz        yuy2_convert_sse2_v_head
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
_yuy2_convert_sse2@16 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; _TEXT64 セグメントの終了
END
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

