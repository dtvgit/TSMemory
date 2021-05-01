;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; お約束
.586
.mmx
.model flat
_TEXT64 segment page public use32 'CODE'
		align 16


adjust_1 dq 00001000100010001h
adjust_2 dq 00002000200020002h
adjust_3 dq 00003000300030003h
adjust_6 dq 00006000600060006h

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Half/Half, 1st）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
; キャッシュにヒットする筈だから、２回メモリから読んでも充分速い？
; 
;-------------------------------------------------------------------
PUBLIC              C _prediction_w16_hh_1st_mmx@20
;      void __stdcall  prediction_w16_hh_1st_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_hh_1st_mmx@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; 使用するローカル変数
; なし
;-------------------------------------------------------------------
; 用途固定 MMX レジスタ
; mm7 - 0
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
; 係数作成
		pxor       mm7, mm7
		movq       mm6, adjust_2
;-------------------------------------------------------------------
; loop
prediction_w16_hh_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+eax]
		movd       mm3, dword ptr [esi+eax+4]
		movq       mm4, mm0
		movq       mm5, mm2
		punpckldq  mm4, mm1
		punpckldq  mm5, mm3
		psrlq      mm4, 8
		psrlq      mm5, 8
		punpcklbw  mm0, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		paddw      mm0, mm2
		paddw      mm0, mm4
		paddw      mm0, mm5
		paddw      mm0, mm6
		psrlw      mm0, 2
		packuswb   mm0, mm7
		movd       dword ptr [edi], mm0
; 0 - 3 finish
		movd       mm0, dword ptr [esi+8]
		movd       mm2, dword ptr [esi+eax+8]
		movq       mm4, mm1
		movq       mm5, mm3
		punpckldq  mm4, mm0
		punpckldq  mm5, mm2
		psrlq      mm4, 8
		psrlq      mm5, 8
		punpcklbw  mm1, mm7
		punpcklbw  mm3, mm7
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		paddw      mm1, mm3
		paddw      mm1, mm4
		paddw      mm1, mm5
		paddw      mm1, mm6
		psrlw      mm1, 2
		packuswb   mm1, mm7
		movd       dword ptr [edi+4], mm1
; 4 - 7 finish
		movd       mm1, dword ptr [esi+12]
		movd       mm3, dword ptr [esi+eax+12]
		movq       mm4, mm0
		movq       mm5, mm2
		punpckldq  mm4, mm1
		punpckldq  mm5, mm3
		psrlq      mm4, 8
		psrlq      mm5, 8
		punpcklbw  mm0, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		paddw      mm0, mm2
		paddw      mm0, mm4
		paddw      mm0, mm5
		paddw      mm0, mm6
		psrlw      mm0, 2
		packuswb   mm0, mm7
		movd       dword ptr [edi+8], mm0
; 8 - 11 finish
		movd       mm0, dword ptr [esi+16]
		movd       mm2, dword ptr [esi+eax+16]
		psllq      mm0, 32
		psllq      mm2, 32
		por        mm0, mm1
		por        mm2, mm3
		psrlq      mm0, 8
		psrlq      mm2, 8
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		paddw      mm0, mm1
		paddw      mm0, mm2
		paddw      mm0, mm3
		paddw      mm0, mm6
		psrlw      mm0, 2
		packuswb   mm0, mm7
		movd       dword ptr [edi+12], mm0
; 12 - 15 finish
		lea        edi, [edi+ebx]
		lea        esi, [esi+eax]
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

_prediction_w16_hh_1st_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Half/Half, 2nd）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
; キャッシュにヒットする筈だから、２回メモリから読んでも充分速い？
; 
;-------------------------------------------------------------------
PUBLIC              C _prediction_w16_hh_2nd_mmx@20
;      void __stdcall  prediction_w16_hh_2nd_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_hh_2nd_mmx@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; 使用するローカル変数
; なし
;-------------------------------------------------------------------
; 用途固定 MMX レジスタ
; mm7 - 0
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
; 係数作成
		pxor       mm7, mm7
;-------------------------------------------------------------------
; loop
prediction_w16_hh_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+eax]
		movd       mm3, dword ptr [esi+eax+4]
		movd       mm6, dword ptr [edi]
		movq       mm4, mm1
		movq       mm5, mm3
		psllq      mm4, 32
		psllq      mm5, 32
		por        mm4, mm0
		por        mm5, mm2
		psrlq      mm4, 8
		psrlq      mm5, 8
		punpcklbw  mm0, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		punpcklbw  mm6, mm7
		paddw      mm0, mm2
		paddw      mm0, mm4
		paddw      mm0, mm5
		paddw      mm0, adjust_6
		psrlw      mm0, 2
		paddw      mm0, mm6
		psrlw      mm0, 1
		packuswb   mm0, mm7
		movd       dword ptr [edi], mm0
; 0 - 3 finish
		movd       mm0, dword ptr [esi+8]
		movd       mm2, dword ptr [esi+eax+8]
		movd       mm6, dword ptr [edi+4]
		movq       mm4, mm0
		movq       mm5, mm2
		psllq      mm4, 32
		psllq      mm5, 32
		por        mm4, mm1
		por        mm5, mm3
		psrlq      mm4, 8
		psrlq      mm5, 8
		punpcklbw  mm1, mm7
		punpcklbw  mm3, mm7
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		punpcklbw  mm6, mm7
		paddw      mm1, mm3
		paddw      mm1, mm4
		paddw      mm1, mm5
		paddw      mm1, adjust_6
		psrlw      mm1, 2
		paddw      mm1, mm6
		psrlw      mm1, 1
		packuswb   mm1, mm7
		movd       dword ptr [edi+4], mm1
; 4 - 7 finish
		movd       mm1, dword ptr [esi+12]
		movd       mm3, dword ptr [esi+eax+12]
		movd       mm6, dword ptr [edi+8]
		movq       mm4, mm1
		movq       mm5, mm3
		psllq      mm4, 32
		psllq      mm5, 32
		por        mm4, mm0
		por        mm5, mm2
		psrlq      mm4, 8
		psrlq      mm5, 8
		punpcklbw  mm0, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		punpcklbw  mm6, mm7
		paddw      mm0, mm2
		paddw      mm0, mm4
		paddw      mm0, mm5
		paddw      mm0, adjust_6
		psrlw      mm0, 2
		paddw      mm0, mm6
		psrlw      mm0, 1
		packuswb   mm0, mm7
		movd       dword ptr [edi+8], mm0
; 8 - 11 finish
		movd       mm0, dword ptr [esi+16]
		movd       mm2, dword ptr [esi+eax+16]
		movd       mm6, dword ptr [edi+12]
		psllq      mm0, 32
		psllq      mm2, 32
		por        mm0, mm1
		por        mm2, mm3
		psrlq      mm0, 8
		psrlq      mm2, 8
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		punpcklbw  mm6, mm7
		paddw      mm0, mm1
		paddw      mm0, mm2
		paddw      mm0, mm3
		paddw      mm0, adjust_6
		psrlw      mm0, 2
		paddw      mm0, mm6
		psrlw      mm0, 1
		packuswb   mm0, mm7
		movd       dword ptr [edi+12], mm0
; 12 - 15 finish
		lea        edi, [edi+ebx]
		lea        esi, [esi+eax]
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

_prediction_w16_hh_2nd_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Full/Half, 1st）水平 Full, 垂直 Half
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
;-------------------------------------------------------------------
PUBLIC              C _prediction_w16_fh_1st_mmx@20
;      void __stdcall  prediction_w16_fh_1st_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_fh_1st_mmx@20 PROC
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
; 係数作成
		shr        ecx, 1
		pxor       mm7, mm7
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+8]
		movd       mm3, dword ptr [esi+12]
		lea        esi, [esi+eax]
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
;-------------------------------------------------------------------
; loop
prediction_w16_fh_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm4, dword ptr [esi]
		movd       mm5, dword ptr [esi+4]
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		paddw      mm0, mm4
		paddw      mm1, mm5
		paddw      mm0, adjust_1
		paddw      mm1, adjust_1
		psrlw      mm0, 1
		psrlw      mm1, 1
		packuswb   mm0, mm1
		movq       [edi], mm0

		pxor       mm1, mm1

		movd       mm6, dword ptr [esi+8]
		movd       mm7, dword ptr [esi+12]
		punpcklbw  mm6, mm1
		punpcklbw  mm7, mm1
		paddw      mm2, mm6
		paddw      mm3, mm7
		paddw      mm2, adjust_1
		paddw      mm3, adjust_1
		psrlw      mm2, 1
		psrlw      mm3, 1
		packuswb   mm2, mm3
		movq       [edi+8], mm2

		pxor       mm3, mm3

		lea        esi, [esi+eax] 
		lea        edi, [edi+ebx]

		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		punpcklbw  mm0, mm3
		punpcklbw  mm1, mm3
		paddw      mm4, mm0
		paddw      mm5, mm1
		paddw      mm4, adjust_1
		paddw      mm5, adjust_1
		psrlw      mm4, 1
		psrlw      mm5, 1
		packuswb   mm4, mm5
		movq       [edi], mm4

		pxor       mm5, mm5

		movd       mm2, dword ptr [esi+8]
		movd       mm3, dword ptr [esi+12]
		punpcklbw  mm2, mm5
		punpcklbw  mm3, mm5
		paddw      mm6, mm2
		paddw      mm7, mm3
		paddw      mm6, adjust_1
		paddw      mm7, adjust_1
		psrlw      mm6, 1
		psrlw      mm7, 1
		packuswb   mm6, mm7
		movq       [edi+8], mm6

		pxor       mm7, mm7

		lea        esi, [esi+eax]
		lea        edi, [edi+ebx]
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

_prediction_w16_fh_1st_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Full/Half, 2nd）水平 Full, 垂直 Half
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
;-------------------------------------------------------------------
PUBLIC              C _prediction_w16_fh_2nd_mmx@20
;      void __stdcall  prediction_w16_fh_2nd_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_fh_2nd_mmx@20 PROC
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
; 係数作成
		shr        ecx, 1
		pxor       mm7, mm7
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+8]
		movd       mm3, dword ptr [esi+12]
		lea        esi, [esi+eax]
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
;-------------------------------------------------------------------
; loop
prediction_w16_fh_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm4, dword ptr [esi]
		movd       mm5, dword ptr [esi+4]
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		paddw      mm0, mm4
		paddw      mm1, mm5
		paddw      mm0, adjust_3
		psrlw      mm0, 1
		movd       mm6, dword ptr [edi]
		punpcklbw  mm6, mm7
		paddw      mm0, mm6
		psrlw      mm0, 1
		paddw      mm1, adjust_3
		psrlw      mm1, 1
		movd       mm6, dword ptr [edi+4]
		punpcklbw  mm6, mm7
		paddw      mm1, mm6
		psrlw      mm1, 1
		packuswb   mm0, mm1
		movq       [edi], mm0

		pxor       mm1, mm1

		movd       mm6, dword ptr [esi+8]
		movd       mm7, dword ptr [esi+12]
		punpcklbw  mm6, mm1
		punpcklbw  mm7, mm1
		paddw      mm2, mm6
		paddw      mm3, mm7
		paddw      mm2, adjust_3
		psrlw      mm2, 1
		movd       mm0, dword ptr [edi+8]
		punpcklbw  mm0, mm1
		paddw      mm2, mm0
		psrlw      mm2, 1
		paddw      mm3, adjust_3
		psrlw      mm3, 1
		movd       mm0, dword ptr [edi+12]     
		punpcklbw  mm0, mm1
		paddw      mm3, mm0
		psrlw      mm3, 1
		packuswb   mm2, mm3
		movq       [edi+8], mm2

		pxor       mm3, mm3

		lea        esi, [esi+eax] 
		lea        edi, [edi+ebx]

		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		punpcklbw  mm0, mm3
		punpcklbw  mm1, mm3
		paddw      mm4, mm0
		paddw      mm5, mm1
		paddw      mm4, adjust_3
		psrlw      mm4, 1
		movd       mm2, dword ptr [edi]
		punpcklbw  mm2, mm3
		paddw      mm4, mm2
		psrlw      mm4, 1
		paddw      mm5, adjust_3
		psrlw      mm5, 1
		movd       mm2, dword ptr [edi+4]
		punpcklbw  mm2, mm3
		paddw      mm5, mm2
		psrlw      mm5, 1
		packuswb   mm4, mm5
		movq       [edi], mm4

		pxor       mm5, mm5

		movd       mm2, dword ptr [esi+8]
		movd       mm3, dword ptr [esi+12]
		punpcklbw  mm2, mm5
		punpcklbw  mm3, mm5
		paddw      mm6, mm2
		paddw      mm7, mm3
		paddw      mm6, adjust_3
		psrlw      mm6, 1
		movd       mm4, dword ptr [edi+8]
		punpcklbw  mm4, mm5
		paddw      mm6, mm4
		psrlw      mm6, 1
		paddw      mm7, adjust_3
		psrlw      mm7, 1
		movd       mm4, dword ptr [edi+12]
		punpcklbw  mm4, mm5
		paddw      mm7, mm4
		psrlw      mm7, 1
		packuswb   mm6, mm7
		movq       [edi+8], mm6

		pxor       mm7, mm7

		lea        esi, [esi+eax]
		lea        edi, [edi+ebx]
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w16_fh_2nd_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_fh_2nd_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Half/Full, 1st）水平 Half, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
;-------------------------------------------------------------------
PUBLIC              C _prediction_w16_hf_1st_mmx@20
;      void __stdcall  prediction_w16_hf_1st_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_hf_1st_mmx@20 PROC
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
; 係数作成
		pxor       mm7, mm7
;-------------------------------------------------------------------
; loop
prediction_w16_hf_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+8]
		movd       mm3, dword ptr [esi+12]
		movd       mm4, dword ptr [esi+16]
		movq       mm5, mm1
		movq       mm6, mm2
		psllq      mm5, 32
		psllq      mm6, 32
		por        mm5, mm0
		por        mm6, mm1
		psrlq      mm5, 8
		psrlq      mm6, 8
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm5, mm7
		punpcklbw  mm6, mm7
		paddw      mm0, mm5
		paddw      mm1, mm6
		paddw      mm0, adjust_1
		paddw      mm1, adjust_1
		psrlw      mm0, 1
		psrlw      mm1, 1
		packuswb   mm0, mm1
		movq       [edi], mm0
		movq       mm5, mm3
		psllq      mm4, 32
		psllq      mm5, 32
		por        mm4, mm3
		por        mm5, mm2
		psrlq      mm4, 8
		psrlq      mm5, 8
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		paddw      mm2, mm5
		paddw      mm3, mm4
		paddw      mm2, adjust_1
		paddw      mm3, adjust_1
		psrlw      mm2, 1
		psrlw      mm3, 1
		packuswb   mm2, mm3
		movq       [edi+8],mm2
		lea        esi, [esi+eax]
		lea        edi, [edi+ebx]		
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

_prediction_w16_hf_1st_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Half/Full, 2nd）水平 Half, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
;-------------------------------------------------------------------
PUBLIC              C _prediction_w16_hf_2nd_mmx@20
;      void __stdcall  prediction_w16_hf_2nd_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_hf_2nd_mmx@20 PROC
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
; 係数作成
		pxor       mm7, mm7
;-------------------------------------------------------------------
; loop
prediction_w16_hf_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+8]
		movd       mm3, dword ptr [esi+12]
		movd       mm4, dword ptr [esi+16]
		movq       mm5, mm1
		movq       mm6, mm2
		psllq      mm5, 32
		psllq      mm6, 32
		por        mm5, mm0
		por        mm6, mm1
		psrlq      mm5, 8
		psrlq      mm6, 8
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm5, mm7
		punpcklbw  mm6, mm7
		paddw      mm0, mm5
		paddw      mm1, mm6
		paddw      mm0, adjust_3
		paddw      mm1, adjust_3
		psrlw      mm0, 1
		psrlw      mm1, 1
		movd       mm5, dword ptr [edi]
		movd       mm6, dword ptr [edi+4]
		punpcklbw  mm5, mm7
		punpcklbw  mm6, mm7
		paddw      mm0, mm5
		paddw      mm1, mm6
		psrlw      mm0, 1
		psrlw      mm1, 1
		packuswb   mm0, mm1
		movq       [edi], mm0
		movq       mm5, mm3
		psllq      mm4, 32
		psllq      mm5, 32
		por        mm4, mm3
		por        mm5, mm2
		psrlq      mm4, 8
		psrlq      mm5, 8
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		paddw      mm2, mm5
		paddw      mm3, mm4
		paddw      mm2, adjust_3
		paddw      mm3, adjust_3
		psrlw      mm2, 1
		psrlw      mm3, 1
		movd       mm5, dword ptr [edi+8]
		movd       mm6, dword ptr [edi+12]
		punpcklbw  mm5, mm7
		punpcklbw  mm6, mm7
		paddw      mm2, mm5
		paddw      mm3, mm6
		psrlw      mm2, 1
		psrlw      mm3, 1
		packuswb   mm2, mm3
		movq       [edi+8],mm2
		lea        esi, [esi+eax]
		lea        edi, [edi+ebx]		
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w16_hf_2nd_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_hf_2nd_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Full/Full, 1st）水平 Full, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; MMX は使わない
;-------------------------------------------------------------------
PUBLIC              C _prediction_w16_ff_1st_mmx@20
;      void __stdcall  prediction_w16_ff_1st_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_ff_1st_mmx@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; edx - ループカウンタ
; ecx - rep カウンタ
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
		mov        edx, [esp+24+20]
;-------------------------------------------------------------------
;ループパラメータ作成
		sub        eax, 16
		sub        ebx, 16
;-------------------------------------------------------------------
; loop
prediction_w16_ff_1st_loop:
		mov        ecx,4
		dec        edx
;-------------------------------------------------------------------
; core
		rep movsd
		lea        esi, [esi+eax]
		lea        edi, [edi+ebx]
;-------------------------------------------------------------------
; ループ終端チェック
		test       edx, edx
		jnz        prediction_w16_ff_1st_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_ff_1st_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Full/Full, 2nd）水平 Full, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16 bit 精度で 4 並列演算
;-------------------------------------------------------------------
PUBLIC              C _prediction_w16_ff_2nd_mmx@20
;      void __stdcall  prediction_w16_ff_2nd_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_ff_2nd_mmx@20 PROC
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
;ループパラメータ作成
		pxor       mm7, mm7
;-------------------------------------------------------------------
; loop
prediction_w16_ff_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+8]
		movd       mm3, dword ptr [esi+12]
		movd       mm4, dword ptr [edi]
		movd       mm5, dword ptr [edi+4]
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		paddw      mm0, mm4
		paddw      mm1, mm5
		paddw      mm0, adjust_1
		paddw      mm1, adjust_1
		psrlw      mm0, 1
		psrlw      mm1, 1
		packuswb   mm0, mm1
		movq       [edi], mm0
		movd       mm4, dword ptr [edi+8]
		movd       mm5, dword ptr [edi+12]
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		paddw      mm2, mm4
		paddw      mm3, mm5
		paddw      mm2, adjust_1
		paddw      mm3, adjust_1
		psrlw      mm2, 1
		psrlw      mm3, 1
		packuswb   mm2, mm3
		movq       [edi+8], mm2
		lea        esi, [esi+eax]
		lea        edi, [edi+ebx]
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w16_ff_2nd_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_ff_2nd_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Half/Half, 1st）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
; 
;-------------------------------------------------------------------
PUBLIC              C _prediction_w8_hh_1st_mmx@20
;      void __stdcall  prediction_w8_hh_1st_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_hh_1st_mmx@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; 使用するローカル変数
; なし
;-------------------------------------------------------------------
; 用途固定 MMX レジスタ
; mm7 - 0
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
		shr        ecx, 1
		pxor       mm7, mm7
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+8]
		psllq      mm2, 32
		por        mm2, mm1
		psrlq      mm2, 8
		psllq      mm1, 32
		por        mm1, mm0
		psrlq      mm1, 8
		punpcklbw  mm0, mm7
		punpcklbw  mm2, mm7
		lea        esi, [esi+eax]
; mm0, 00030002_00010000
; mm1, 00070605_04030201
; mm2, 00080007_00060005
;-------------------------------------------------------------------
; loop
prediction_w8_hh_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm3, dword ptr [esi]
		movd       mm4, dword ptr [esi+4]
		movd       mm5, dword ptr [esi+8]
		psllq      mm5, 32
		por        mm5, mm4
		psrlq      mm5, 8
		psllq      mm4, 32
		por        mm4, mm3
		psrlq      mm4, 8
		movq       mm6, mm1
		punpcklbw  mm3, mm7
		punpcklbw  mm5, mm7
		punpcklbw  mm6, mm7
		paddw      mm0, mm3
		paddw      mm0, mm6
		movq       mm6, mm4
		punpcklbw  mm6, mm7
		paddw      mm0, mm6
		paddw      mm0, adjust_2
		psrlw      mm0, 2

		movq       mm6, mm4
		psrlq      mm1, 24
		psrlq      mm6, 24
		punpcklbw  mm1, mm7
		punpcklbw  mm6, mm7
		paddw      mm1, mm2
		paddw      mm1, mm5
		paddw      mm1, mm6
		paddw      mm1, adjust_2
		psrlw      mm1, 2

		packuswb   mm0, mm1
		movq       [edi], mm0

		lea        edi, [edi+ebx]
		lea        esi, [esi+eax]

		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+8]
		psllq      mm2, 32
		por        mm2, mm1
		psrlq      mm2, 8
		psllq      mm1, 32
		por        mm1, mm0
		psrlq      mm1, 8
		movq       mm6, mm4
		punpcklbw  mm0, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm6, mm7
		paddw      mm3, mm0
		paddw      mm3, mm6
		movq       mm6, mm1
		punpcklbw  mm6, mm7
		paddw      mm3, mm6
		paddw      mm3, adjust_2
		psrlw      mm3, 2

		movq       mm6, mm1
		psrlq      mm4, 24
		psrlq      mm6, 24
		punpcklbw  mm4, mm7
		punpcklbw  mm6, mm7
		paddw      mm4, mm2
		paddw      mm4, mm5
		paddw      mm4, mm6
		paddw      mm4, adjust_2
		psrlw      mm4, 2

		packuswb   mm3, mm4
		movq       [edi], mm3

		lea        edi, [edi+ebx]
		lea        esi, [esi+eax]
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w8_hh_1st_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w8_hh_1st_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Half/Half, 2nd）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
; 
;-------------------------------------------------------------------
PUBLIC              C _prediction_w8_hh_2nd_mmx@20
;      void __stdcall  prediction_w8_hh_2nd_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_hh_2nd_mmx@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; 使用するローカル変数
; なし
;-------------------------------------------------------------------
; 用途固定 MMX レジスタ
; mm7 - 0
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
		shr        ecx, 1
		pxor       mm7, mm7
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+8]
		psllq      mm2, 32
		por        mm2, mm1
		psrlq      mm2, 8
		psllq      mm1, 32
		por        mm1, mm0
		psrlq      mm1, 8
		punpcklbw  mm0, mm7
		punpcklbw  mm2, mm7
		lea        esi, [esi+eax]
;-------------------------------------------------------------------
; loop
prediction_w8_hh_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm3, dword ptr [esi]
		movd       mm4, dword ptr [esi+4]
		movd       mm5, dword ptr [esi+8]
		psllq      mm5, 32
		por        mm5, mm4
		psrlq      mm5, 8
		psllq      mm4, 32
		por        mm4, mm3
		psrlq      mm4, 8
		movq       mm6, mm1
		punpcklbw  mm3, mm7
		punpcklbw  mm5, mm7
		punpcklbw  mm6, mm7
		paddw      mm0, mm3
		paddw      mm0, mm6
		movq       mm6, mm4
		punpcklbw  mm6, mm7
		paddw      mm0, mm6
		paddw      mm0, adjust_6
		psrlw      mm0, 2
		movd       mm6, dword ptr [edi]
		punpcklbw  mm6, mm7
		paddw      mm0, mm6
		psrlw      mm0, 1

		movq       mm6, mm4
		psrlq      mm1, 24
		psrlq      mm6, 24
		punpcklbw  mm1, mm7
		punpcklbw  mm6, mm7
		paddw      mm1, mm2
		paddw      mm1, mm5
		paddw      mm1, mm6
		paddw      mm1, adjust_6
		psrlw      mm1, 2
		movd       mm6, dword ptr [edi+4]
		punpcklbw  mm6, mm7
		paddw      mm1, mm6
		psrlw      mm1, 1

		packuswb   mm0, mm1
		movq       [edi], mm0

		lea        edi, [edi+ebx]
		lea        esi, [esi+eax]

		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [esi+8]
		psllq      mm2, 32
		por        mm2, mm1
		psrlq      mm2, 8
		psllq      mm1, 32
		por        mm1, mm0
		psrlq      mm1, 8
		movq       mm6, mm4
		punpcklbw  mm0, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm6, mm7
		paddw      mm3, mm0
		paddw      mm3, mm6
		movq       mm6, mm1
		punpcklbw  mm6, mm7
		paddw      mm3, mm6
		paddw      mm3, adjust_6
		psrlw      mm3, 2
		movd       mm6, dword ptr [edi]
		punpcklbw  mm6, mm7
		paddw      mm3, mm6
		psrlw      mm3, 1

		movq       mm6, mm1
		psrlq      mm4, 24
		psrlq      mm6, 24
		punpcklbw  mm4, mm7
		punpcklbw  mm6, mm7
		paddw      mm4, mm2
		paddw      mm4, mm5
		paddw      mm4, mm6
		paddw      mm4, adjust_6
		psrlw      mm4, 2
		movd       mm6, dword ptr [edi+4]
		punpcklbw  mm6, mm7
		paddw      mm4, mm6
		psrlw      mm4, 1

		packuswb   mm3, mm4
		movq       [edi], mm3

		lea        edi, [edi+ebx]
		lea        esi, [esi+eax]
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w8_hh_2nd_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w8_hh_2nd_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Full/Half, 1st）水平 Full, 垂直 Half
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
; 
;-------------------------------------------------------------------
PUBLIC              C _prediction_w8_fh_1st_mmx@20
;      void __stdcall  prediction_w8_fh_1st_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_fh_1st_mmx@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; 使用するローカル変数
; なし
;-------------------------------------------------------------------
; 用途固定 MMX レジスタ
; mm7 - 0
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
		shr        ecx, 1
		pxor       mm7, mm7
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		lea        esi, [esi+eax]
;-------------------------------------------------------------------
; loop
prediction_w8_fh_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm2, dword ptr [esi]
		movd       mm3, dword ptr [esi+4]
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		paddw      mm0, mm2
		paddw      mm1, mm3
		paddw      mm0, adjust_1
		paddw      mm1, adjust_1
		psrlw      mm0, 1
		psrlw      mm1, 1
		packuswb   mm0, mm1
		movq       [edi], mm0

		lea        edi, [edi+ebx]
		lea        esi, [esi+eax]

		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		paddw      mm2, mm0
		paddw      mm3, mm1
		paddw      mm2, adjust_1
		paddw      mm3, adjust_1
		psrlw      mm2, 1
		psrlw      mm3, 1
		packuswb   mm2, mm3
		movq       [edi], mm2

		lea        edi, [edi+ebx]
		lea        esi, [esi+eax]

;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w8_fh_1st_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w8_fh_1st_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Full/Half, 2nd）水平 Full, 垂直 Half
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
; 
;-------------------------------------------------------------------
PUBLIC              C _prediction_w8_fh_2nd_mmx@20
;      void __stdcall  prediction_w8_fh_2nd_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_fh_2nd_mmx@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; 使用するローカル変数
; なし
;-------------------------------------------------------------------
; 用途固定 MMX レジスタ
; mm7 - 0
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
		shr        ecx, 1
		pxor       mm7, mm7
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		lea        esi, [esi+eax]
;-------------------------------------------------------------------
; loop
prediction_w8_fh_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm2, dword ptr [esi]
		movd       mm3, dword ptr [esi+4]
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		paddw      mm0, mm2
		paddw      mm1, mm3
		paddw      mm0, adjust_3
		paddw      mm1, adjust_3
		psrlw      mm0, 1
		psrlw      mm1, 1
		movd       mm4, dword ptr [edi]
		movd       mm5, dword ptr [edi+4]
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		paddw      mm0, mm4
		paddw      mm1, mm5
		psrlw      mm0, 1
		psrlw      mm1, 1
		packuswb   mm0, mm1
		movq       [edi], mm0

		lea        edi, [edi+ebx]
		lea        esi, [esi+eax]

		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		paddw      mm2, mm0
		paddw      mm3, mm1
		paddw      mm2, adjust_3
		paddw      mm3, adjust_3
		psrlw      mm2, 1
		psrlw      mm3, 1
		movd       mm4, dword ptr [edi]
		movd       mm5, dword ptr [edi+4]
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		paddw      mm2, mm4
		paddw      mm3, mm5
		psrlw      mm2, 1
		psrlw      mm3, 1
		packuswb   mm2, mm3
		movq       [edi], mm2

		lea        edi, [edi+ebx]
		lea        esi, [esi+eax]

;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w8_fh_2nd_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w8_fh_2nd_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Half/Full, 1st）水平 Half, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
; 
;-------------------------------------------------------------------
PUBLIC              C _prediction_w8_hf_1st_mmx@20
;      void __stdcall  prediction_w8_hf_1st_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_hf_1st_mmx@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; 使用するローカル変数
; なし
;-------------------------------------------------------------------
; 用途固定 MMX レジスタ
; mm7 - 0
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
		pxor       mm7, mm7
;-------------------------------------------------------------------
; loop
prediction_w8_hf_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm3, dword ptr [esi+8]
		movq       mm2, mm1
		psllq      mm1, 32
		psllq      mm3, 32
		por        mm1, mm0
		por        mm3, mm2
		psrlq      mm1, 8
		psrlq      mm3, 8
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		paddw      mm0, mm1
		paddw      mm2, mm3
		paddw      mm0, adjust_1
		paddw      mm2, adjust_1
		psrlw      mm0, 1
		psrlw      mm2, 1
		packuswb   mm0, mm2
		movq       [edi], mm0
		lea        edi, [edi+ebx]
		lea        esi, [esi+eax]
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w8_hf_1st_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w8_hf_1st_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Half/Full, 2nd）水平 Half, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
; 
;-------------------------------------------------------------------
PUBLIC              C _prediction_w8_hf_2nd_mmx@20
;      void __stdcall  prediction_w8_hf_2nd_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_hf_2nd_mmx@20 PROC
;-------------------------------------------------------------------
; 使用するレジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - 入力ステップ
; ebx - 出力ステップ
;-------------------------------------------------------------------
; 使用するローカル変数
; なし
;-------------------------------------------------------------------
; 用途固定 MMX レジスタ
; mm7 - 0
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
		pxor       mm7, mm7
		movq       mm6, adjust_3
;-------------------------------------------------------------------
; loop
prediction_w8_hf_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm3, dword ptr [esi+8]
		movq       mm2, mm1
		psllq      mm1, 32
		psllq      mm3, 32
		por        mm1, mm0
		por        mm3, mm2
		psrlq      mm1, 8
		psrlq      mm3, 8
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		paddw      mm0, mm1
		paddw      mm2, mm3
		paddw      mm0, mm6
		paddw      mm2, mm6
		psrlw      mm0, 1
		psrlw      mm2, 1
		movd       mm4, dword ptr [edi]
		movd       mm5, dword ptr [edi+4]
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		paddw      mm0, mm4
		paddw      mm2, mm5
		psrlw      mm0, 1
		psrlw      mm2, 1
		packuswb   mm0, mm2
		movq       [edi], mm0
		lea        edi, [edi+ebx]
		lea        esi, [esi+eax]
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w8_hf_2nd_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w8_hf_2nd_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Full/Full, 1st）水平 Full, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
;-------------------------------------------------------------------
PUBLIC              C _prediction_w8_ff_1st_mmx@20
;      void __stdcall  prediction_w8_ff_1st_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_ff_1st_mmx@20 PROC
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
;ループパラメータ作成
		shr        ecx, 2
;-------------------------------------------------------------------
; loop
prediction_w8_ff_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movq       mm0, [esi]
		add        esi, eax
		movq       mm1, [esi]
		add        esi, eax
		movq       mm2, [esi]
		add        esi, eax
		movq       mm3, [esi]
		add        esi, eax
		movq       [edi], mm0
		add        edi, ebx
		movq       [edi], mm1
		add        edi, ebx
		movq       [edi], mm2
		add        edi, ebx
		movq       [edi], mm3
		add        edi, ebx
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w8_ff_1st_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w8_ff_1st_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Full/Full, 2nd）水平 Full, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
;-------------------------------------------------------------------
PUBLIC              C _prediction_w8_ff_2nd_mmx@20
;      void __stdcall  prediction_w8_ff_2nd_mmx(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_ff_2nd_mmx@20 PROC
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
;ループパラメータ作成
		movq       mm4, adjust_1;
		pxor       mm6, mm6
		movq       mm5, mm4
		pxor       mm7, mm7
;-------------------------------------------------------------------
; loop
prediction_w8_ff_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+4]
		movd       mm2, dword ptr [edi]
		movd       mm3, dword ptr [edi+4]
		punpcklbw  mm0, mm6
		punpcklbw  mm1, mm7
		punpcklbw  mm2, mm6
		punpcklbw  mm3, mm7
		paddw      mm0, mm2
		paddw      mm1, mm3
		paddw      mm0, mm4
		paddw      mm1, mm5
		psrlw      mm0, 1
		psrlw      mm1, 1
		packuswb   mm0, mm1
		movq       [edi], mm0
		lea        esi, [esi+eax]
		lea        edi, [edi+ebx]
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w8_ff_2nd_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w8_ff_2nd_mmx@20 ENDP
;-------------------------------------------------------------------
; 終了

END