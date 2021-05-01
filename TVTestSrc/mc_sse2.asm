;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; お約束
.586
.mmx
.xmm
.model flat
_TEXT64 segment page public use32 'CODE'
		align 16

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Half/Half, 1st）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w16_hh_1st_sse2@20
;      void __stdcall  prediction_w16_hh_1st_sse2(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_hh_1st_sse2@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; レジスタの退避
		push       esi
		push       edi
		push       ecx
		push       ebx
		push       eax
;-------------------------------------------------------------------
; 引数からデータを受け取っておく
		mov        esi, [esp+20+ 4]
		mov        edi, [esp+20+ 8]
		mov        eax, [esp+20+12]
		mov        ebx, [esp+20+16]
		mov        ecx, [esp+20+20]
;-------------------------------------------------------------------
; 下準備
		movdqu     xmm0, [esi]
		pcmpeqw    xmm6, xmm6
		movdqu     xmm1, [esi]
		pxor       xmm7, xmm7
		movdqu     xmm2, [esi+1]
		psllw      xmm6, 1
		movdqu     xmm3, [esi+1]
		punpcklbw  xmm0, xmm7
		add        esi, eax
		punpckhbw  xmm1, xmm7
		shr        ecx, 1
		punpcklbw  xmm2, xmm7
		punpckhbw  xmm3, xmm7
;-------------------------------------------------------------------
; loop
prediction_w16_hh_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		paddw      xmm0, xmm2
		movdqu     xmm4, [esi]
		paddw      xmm1, xmm3
		movdqu     xmm5, [esi]
		movdqu     xmm2, [esi+1]
		movdqu     xmm3, [esi+1]
		add        esi, eax
		punpcklbw  xmm4, xmm7
		psubw      xmm0, xmm6
		punpckhbw  xmm5, xmm7
		psubw      xmm1, xmm6
		punpcklbw  xmm2, xmm7
		paddw      xmm0, xmm4
		punpckhbw  xmm3, xmm7
		paddw      xmm1, xmm5
		paddw      xmm0, xmm2
		paddw      xmm1, xmm3
		psraw      xmm0, 2
		paddw      xmm4, xmm2
		psraw      xmm1, 2
		paddw      xmm5, xmm3
		packuswb   xmm0, xmm1
		movdqu     xmm2, [esi]
		movdqa     [edi], xmm0
		movdqu     xmm3, [esi]
		movdqu     xmm0, [esi+1]
		movdqu     xmm1, [esi+1]
		punpcklbw  xmm2, xmm7
		psubw      xmm4, xmm6
		punpckhbw  xmm3, xmm7
		psubw      xmm5, xmm6
		punpcklbw  xmm0, xmm7
		paddw      xmm4, xmm2
		punpckhbw  xmm1, xmm7
		paddw      xmm5, xmm3
		add        edi,  ebx
		paddw      xmm4, xmm0
		add        esi,  eax
		paddw      xmm5, xmm1
		psraw      xmm4, 2
		psraw      xmm5, 2
		packuswb   xmm4, xmm5
		movdqa     [edi], xmm4
		add        edi, ebx
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w16_hh_1st_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_hh_1st_sse2@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Half/Half, 2nd）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w16_hh_2nd_sse2@20
;      void __stdcall  prediction_w16_hh_2nd_sse2(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_hh_2nd_sse2@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; レジスタの退避
		push       esi
		push       edi
		push       ecx
		push       ebx
		push       eax
;-------------------------------------------------------------------
; 引数からデータを受け取っておく
		mov        esi, [esp+20+ 4]
		mov        edi, [esp+20+ 8]
		mov        eax, [esp+20+12]
		mov        ebx, [esp+20+16]
		mov        ecx, [esp+20+20]
;-------------------------------------------------------------------
; 下準備
		movdqu     xmm0, [esi]
		pcmpeqw    xmm6, xmm6
		movdqu     xmm1, [esi]
		pxor       xmm7, xmm7
		movdqu     xmm2, [esi+1]
		psllw      xmm6, 1
		movdqu     xmm3, [esi+1]
		punpcklbw  xmm0, xmm7
		add        esi, eax
		punpckhbw  xmm1, xmm7
		shr        ecx, 1
		punpcklbw  xmm2, xmm7
		punpckhbw  xmm3, xmm7
;-------------------------------------------------------------------
; loop
prediction_w16_hh_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		paddw      xmm0, xmm2
		movdqu     xmm4, [esi]
		paddw      xmm1, xmm3
		movdqu     xmm5, [esi]
		movdqu     xmm2, [esi+1]
		movdqu     xmm3, [esi+1]
		add        esi, eax
		punpcklbw  xmm4, xmm7
		psubw      xmm0, xmm6
		punpckhbw  xmm5, xmm7
		psubw      xmm1, xmm6
		punpcklbw  xmm2, xmm7
		paddw      xmm0, xmm4
		punpckhbw  xmm3, xmm7
		paddw      xmm1, xmm5
		paddw      xmm0, xmm2
		paddw      xmm1, xmm3
		psraw      xmm0, 2
		paddw      xmm4, xmm2
		psraw      xmm1, 2
		paddw      xmm5, xmm3
		packuswb   xmm0, xmm1
		movdqu     xmm2, [esi]
		pavgb      xmm0, [edi]
		movdqu     xmm3, [esi]
		movdqa     [edi], xmm0
		movdqu     xmm0, [esi+1]
		movdqu     xmm1, [esi+1]
		punpcklbw  xmm2, xmm7
		psubw      xmm4, xmm6
		punpckhbw  xmm3, xmm7
		psubw      xmm5, xmm6
		punpcklbw  xmm0, xmm7
		paddw      xmm4, xmm2
		punpckhbw  xmm1, xmm7
		paddw      xmm5, xmm3
		add        edi,  ebx
		paddw      xmm4, xmm0
		add        esi,  eax
		paddw      xmm5, xmm1
		psraw      xmm4, 2
		psraw      xmm5, 2
		packuswb   xmm4, xmm5
		pavgb      xmm4, [edi]
		movdqa     [edi], xmm4
		add        edi, ebx
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w16_hh_2nd_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_hh_2nd_sse2@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Full/Half, 1st）水平 Full, 垂直 Half
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w16_fh_1st_sse2@20
;      void __stdcall  prediction_w16_fh_1st_sse2(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_fh_1st_sse2@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; レジスタの退避
		push       esi
		push       edi
		push       ecx
		push       ebx
		push       eax
;-------------------------------------------------------------------
; 引数からデータを受け取っておく
		mov        esi, [esp+20+ 4]
		mov        edi, [esp+20+ 8]
		mov        eax, [esp+20+12]
		mov        ebx, [esp+20+16]
		mov        ecx, [esp+20+20]
;-------------------------------------------------------------------
; 下準備
		shr        ecx, 3
;-------------------------------------------------------------------
; loop
prediction_w16_fh_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movdqu     xmm0, [esi]
		add        esi, eax
		movdqu     xmm1, [esi]
		add        esi, eax
		movdqu     xmm2, [esi]
		add        esi, eax
		movdqu     xmm3, [esi]
		add        esi, eax
		movdqu     xmm4, [esi]
		add        esi, eax
		movdqu     xmm5, [esi]
		add        esi, eax
		pavgb      xmm0, xmm1
		movdqu     xmm6, [esi]
		pavgb      xmm1, xmm2
		add        esi, eax
		pavgb      xmm2, xmm3
		movdqu     xmm7, [esi]
		pavgb      xmm3, xmm4
		add        esi, eax
		pavgb      xmm4, xmm5
		movdqa     [edi], xmm0
		movdqu     xmm0, [esi]
		pavgb      xmm5, xmm6
		add        edi, ebx
		pavgb      xmm6, xmm7
		movdqa     [edi], xmm1
		pavgb      xmm7, xmm0
		add        edi, ebx
		movdqa     [edi], xmm2
		add        edi, ebx
		movdqa     [edi], xmm3
		add        edi, ebx
		movdqa     [edi], xmm4
		add        edi, ebx
		movdqa     [edi], xmm5
		add        edi, ebx
		movdqa     [edi], xmm6
		add        edi, ebx
		movdqa     [edi], xmm7
		add        edi, ebx
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w16_fh_1st_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_fh_1st_sse2@20 ENDP
;-------------------------------------------------------------------
; 終了


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Full/Half, 2nd）水平 Full, 垂直 Half
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w16_fh_2nd_sse2@20
;      void __stdcall  prediction_w16_fh_2nd_sse2(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_fh_2nd_sse2@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; レジスタの退避
		push       esi
		push       edi
		push       edx
		push       ecx
		push       ebx
		push       eax
;-------------------------------------------------------------------
; 引数からデータを受け取っておく
		mov        esi, [esp+24+ 4]
		mov        edi, [esp+24+ 8]
		mov        eax, [esp+24+12]
		mov        ebx, [esp+24+16]
		mov        ecx, [esp+24+20]
;-------------------------------------------------------------------
; 下準備
		shr        ecx, 3
		mov        edx, edi
;-------------------------------------------------------------------
; loop
prediction_w16_fh_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movdqu     xmm0, [esi]
		add        esi, eax
		movdqu     xmm1, [esi]
		add        esi, eax
		movdqu     xmm2, [esi]
		add        esi, eax
		movdqu     xmm3, [esi]
		add        esi, eax
		movdqu     xmm4, [esi]
		pavgb      xmm0, xmm1
		add        esi, eax
		movdqu     xmm5, [esi]
		pavgb      xmm1, xmm2
		add        esi, eax
		movdqu     xmm6, [esi]
		pavgb      xmm2, xmm3
		add        esi, eax
		movdqu     xmm7, [esi]
		pavgb      xmm3, xmm4
		add        esi, eax
		pavgb      xmm0, [edx]
		add        edx, ebx
		pavgb      xmm4, xmm5
		movdqa     [edi], xmm0
		movdqu     xmm0, [esi]
		add        edi, ebx
		pavgb      xmm1, [edx]
		add        edx, ebx
		pavgb      xmm5, xmm6
		movdqa     [edi], xmm1
		add        edi, ebx
		pavgb      xmm2, [edx]
		add        edx, ebx
		pavgb      xmm6, xmm7
		movdqa     [edi], xmm2
		add        edi, ebx
		pavgb      xmm3, [edx]
		add        edx, ebx
		pavgb      xmm7, xmm0
		movdqa     [edi], xmm3
		add        edi, ebx
		pavgb      xmm4, [edx]
		add        edx, ebx
		pavgb      xmm5, [edx]
		add        edx, ebx
		pavgb      xmm6, [edx]
		add        edx, ebx
		pavgb      xmm7, [edx]
		add        edx, ebx
		movdqa     [edi], xmm4
		add        edi, ebx
		movdqa     [edi], xmm5
		add        edi, ebx
		movdqa     [edi], xmm6
		add        edi, ebx
		movdqa     [edi], xmm7
		add        edi, ebx
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w16_fh_2nd_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_fh_2nd_sse2@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Half/Full, 1st）水平 Half, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w16_hf_1st_sse2@20
;      void __stdcall  prediction_w16_hf_1st_sse2(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_hf_1st_sse2@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; レジスタの退避
		push       esi
		push       edi
		push       ecx
		push       ebx
		push       eax
;-------------------------------------------------------------------
; 引数からデータを受け取っておく
		mov        esi, [esp+20+ 4]
		mov        edi, [esp+20+ 8]
		mov        eax, [esp+20+12]
		mov        ebx, [esp+20+16]
		mov        ecx, [esp+20+20]
;-------------------------------------------------------------------
; 下準備
		shr        ecx, 2
;-------------------------------------------------------------------
; loop
prediction_w16_hf_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movdqu     xmm0, [esi]
		movdqu     xmm1, [esi+1]
		add        esi, eax
		movdqu     xmm2, [esi]
		movdqu     xmm3, [esi+1]
		add        esi, eax
		pavgb      xmm0, xmm1
		movdqu     xmm4, [esi]
		movdqu     xmm5, [esi+1]
		add        esi, eax
		pavgb      xmm2, xmm3
		movdqu     xmm6, [esi]
		movdqu     xmm7, [esi+1]
		add        esi, eax
		pavgb      xmm4, xmm5
		movdqa     [edi], xmm0
		add        edi, ebx
		pavgb      xmm6, xmm7
		movdqa     [edi], xmm2
		add        edi, ebx
		movdqa     [edi], xmm4
		add        edi, ebx
		movdqa     [edi], xmm6
		add        edi, ebx
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w16_hf_1st_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_hf_1st_sse2@20 ENDP
;-------------------------------------------------------------------
; 終了


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Half/Full, 2nd）水平 Half, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w16_hf_2nd_sse2@20
;      void __stdcall  prediction_w16_hf_2nd_sse2(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_hf_2nd_sse2@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; レジスタの退避
		push       esi
		push       edi
		push       edx
		push       ecx
		push       ebx
		push       eax
;-------------------------------------------------------------------
; 引数からデータを受け取っておく
		mov        esi, [esp+24+ 4]
		mov        edi, [esp+24+ 8]
		mov        eax, [esp+24+12]
		mov        ebx, [esp+24+16]
		mov        ecx, [esp+24+20]
;-------------------------------------------------------------------
; 下準備
		shr        ecx, 2
		mov        edx, edi
;-------------------------------------------------------------------
; loop
prediction_w16_hf_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movdqu     xmm0, [esi]
		movdqu     xmm1, [esi+1]
		add        esi, eax
		movdqu     xmm2, [esi]
		movdqu     xmm3, [esi+1]
		add        esi, eax
		movdqu     xmm4, [esi]
		movdqu     xmm5, [esi+1]
		pavgb      xmm0, xmm1
		add        esi, eax
		movdqu     xmm6, [esi]
		movdqu     xmm7, [esi+1]
		pavgb      xmm2, xmm3
		add        esi, eax
		pavgb      xmm0, [edx]
		add        edx, ebx
		pavgb      xmm4, xmm5
		movdqa     [edi], xmm0
		add        edi, ebx
		pavgb      xmm2, [edx]
		add        edx, ebx
		pavgb      xmm6, xmm7
		movdqa     [edi], xmm2
		add        edi, ebx
		pavgb      xmm4, [edx]
		add        edx, ebx
		pavgb      xmm6, [edx]
		add        edx, ebx
		movdqa     [edi], xmm4
		add        edi, ebx
		movdqa     [edi], xmm6
		add        edi, ebx
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w16_hf_2nd_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_hf_2nd_sse2@20 ENDP
;-------------------------------------------------------------------
; 終了


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Full/Full, 2nd）水平 Full, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16 bit 精度で 4 並列演算
;-------------------------------------------------------------------
PUBLIC              C _prediction_w16_ff_2nd_sse2@20
;      void __stdcall  prediction_w16_ff_2nd_sse2(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_ff_2nd_sse2@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; レジスタの退避
		push       esi
		push       edi
		push       edx
		push       ecx
		push       ebx
		push       eax
;-------------------------------------------------------------------
; 引数からデータを受け取っておく
		mov        esi, [esp+24+ 4]
		mov        edi, [esp+24+ 8]
		mov        eax, [esp+24+12]
		mov        ebx, [esp+24+16]
		mov        ecx, [esp+24+20]
;-------------------------------------------------------------------
; 下準備
		shr        ecx, 3
		mov        edx, edi
;-------------------------------------------------------------------
; loop
prediction_w16_ff_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movdqu     xmm0, [esi]
		add        esi, eax
		movdqu     xmm1, [esi]
		add        esi, eax
		movdqu     xmm2, [esi]
		add        esi, eax
		movdqu     xmm3, [esi]
		add        esi, eax
		pavgb      xmm0, [edx]
		add        edx, ebx
		movdqu     xmm4, [esi]
		add        esi, eax
		pavgb      xmm1, [edx]
		add        edx, ebx
		movdqu     xmm5, [esi]
		add        esi, eax
		pavgb      xmm2, [edx]
		add        edx, ebx
		movdqu     xmm6, [esi]
		add        esi, eax
		pavgb      xmm3, [edx]
		add        edx, ebx
		movdqu     xmm7, [esi]
		add        esi, eax
		pavgb      xmm4, [edx]
		add        edx, ebx
		pavgb      xmm5, [edx]
		add        edx, ebx
		pavgb      xmm6, [edx]
		add        edx, ebx
		pavgb      xmm7, [edx]
		add        edx, ebx
		movdqa     [edi], xmm0
		add        edi, ebx
		movdqa     [edi], xmm1		
		add        edi, ebx
		movdqa     [edi], xmm2		
		add        edi, ebx
		movdqa     [edi], xmm3		
		add        edi, ebx
		movdqa     [edi], xmm4		
		add        edi, ebx
		movdqa     [edi], xmm5		
		add        edi, ebx
		movdqa     [edi], xmm6		
		add        edi, ebx
		movdqa     [edi], xmm7		
		add        edi, ebx
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w16_ff_2nd_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_ff_2nd_sse2@20 ENDP
;-------------------------------------------------------------------
; 終了

END