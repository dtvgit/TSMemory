;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; お約束
.586
.mmx
.xmm
.model flat
_TEXT64 segment page public use32 'CODE'
		align 16

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Half/Half, 2nd）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; 16bit 精度で４並列計算
; キャッシュにヒットする筈だから、２回メモリから読んでも充分速い？
; 
;-------------------------------------------------------------------
PUBLIC              C _prediction_w16_hh_2nd_sse@20
;      void __stdcall  prediction_w16_hh_2nd_sse(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_hh_2nd_sse@20 PROC
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
; 用途固定 sse レジスタ
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
		pcmpeqw    mm6, mm6
		pxor       mm7, mm7
		psllw      mm6, 1
;-------------------------------------------------------------------
; loop
prediction_w16_hh_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+1]
		movd       mm2, dword ptr [esi+eax]
		movd       mm3, dword ptr [esi+eax+1]
		movd       mm4, dword ptr [esi+4]
		movd       mm5, dword ptr [esi+5]
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		paddw      mm6, mm2
		paddw      mm1, mm3
		movd       mm2, dword ptr [esi+eax+4]
		movd       mm3, dword ptr [esi+eax+5]
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		paddw      mm4, mm2
		paddw      mm5, mm3
		psubw      mm0, mm6
		paddw      mm4, mm6
		psrlw      mm0, 2
		psrlw      mm4, 2
		packuswb   mm0, mm4
		pavgb      mm0, [edi]
		movq       [edi], mm6
; 0 - 7 finish
		movd       mm0, dword ptr [esi]
		movd       mm1, dword ptr [esi+1]
		movd       mm2, dword ptr [esi+eax]
		movd       mm3, dword ptr [esi+eax+1]
		movd       mm4, dword ptr [esi+4]
		movd       mm5, dword ptr [esi+5]
		punpcklbw  mm0, mm7
		punpcklbw  mm1, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		paddw      mm6, mm2
		paddw      mm1, mm3
		movd       mm2, dword ptr [esi+eax+4]
		movd       mm3, dword ptr [esi+eax+5]
		punpcklbw  mm4, mm7
		punpcklbw  mm5, mm7
		punpcklbw  mm2, mm7
		punpcklbw  mm3, mm7
		paddw      mm4, mm2
		paddw      mm5, mm3
		psubw      mm0, mm6
		paddw      mm4, mm6
		psrlw      mm0, 2
		psrlw      mm4, 2
		packuswb   mm0, mm4
		pavgb      mm0, [edi]
		movq       [edi], mm6
; 7 - 15 finish
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

_prediction_w16_hh_2nd_sse@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Full/Half, 1st）水平 Full, 垂直 Half
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
; pavgb 使って 8 並列処理
;-------------------------------------------------------------------
PUBLIC              C _prediction_w16_fh_1st_sse@20
;      void __stdcall  prediction_w16_fh_1st_sse(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_fh_1st_sse@20 PROC
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
		shr        ecx, 2
		movq       mm0, [esi]
		movq       mm1, [esi+8]
		movq       mm2, [esi+eax]
		movq       mm3, [esi+eax+8]
		dec        ecx
		lea        esi, [esi+eax*2]
;-------------------------------------------------------------------
; loop
prediction_w16_fh_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movq       mm4, [esi]
		movq       mm5, [esi+8]
		movq       mm6, [esi+eax]
		movq       mm7, [esi+eax+8]
		lea        esi, [esi+eax*2]
		pavgb      mm0, mm2
		pavgb      mm1, mm3
		pavgb      mm2, mm4
		pavgb      mm3, mm5
		movq       [edi], mm0
		movq       [edi+8], mm1
		movq       [edi+ebx], mm2
		movq       [edi+ebx+8], mm3
		lea        edi, [edi+ebx*2]
		movq       mm0, [esi]
		movq       mm1, [esi+8]
		movq       mm2, [esi+eax]
		movq       mm3, [esi+eax+8]
		lea        esi, [esi+eax*2]
		pavgb      mm4, mm6
		pavgb      mm5, mm7
		pavgb      mm6, mm0
		pavgb      mm7, mm1
		movq       [edi], mm4
		movq       [edi+8], mm5
		movq       [edi+ebx], mm6
		movq       [edi+ebx+8], mm7
		lea        edi, [edi+ebx*2]
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w16_fh_1st_loop
;-------------------------------------------------------------------
; 最後の４行出力
		movq       mm4, [esi]
		movq       mm5, [esi+8]
		movq       mm6, [esi+eax]
		movq       mm7, [esi+eax+8]
		lea        esi, [esi+eax*2]
		pavgb      mm0, mm2
		pavgb      mm1, mm3
		pavgb      mm2, mm4
		pavgb      mm3, mm5
		movq       [edi], mm0
		movq       [edi+8], mm1
		movq       [edi+ebx], mm2
		movq       [edi+ebx+8], mm3
		lea        edi, [edi+ebx*2]
		movq       mm0, [esi]
		movq       mm1, [esi+8]
		pavgb      mm4, mm6
		pavgb      mm5, mm7
		pavgb      mm6, mm0
		pavgb      mm7, mm1
		movq       [edi], mm4
		movq       [edi+8], mm5
		movq       [edi+ebx], mm6
		movq       [edi+ebx+8], mm7
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_fh_1st_sse@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Full/Half, 2nd）水平 Full, 垂直 Half
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w16_fh_2nd_sse@20
;      void __stdcall  prediction_w16_fh_2nd_sse(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_fh_2nd_sse@20 PROC
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
		shr        ecx, 2
		movq       mm0, [esi]
		movq       mm1, [esi+8]
		movq       mm2, [esi+eax]
		movq       mm3, [esi+eax+8]
                lea        esi, [esi+eax*2]
		dec        ecx
;-------------------------------------------------------------------
; loop
prediction_w16_fh_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movq       mm4, [esi]
		movq       mm5, [esi+8]
		movq       mm6, [esi+eax]
		movq       mm7, [esi+eax+8]
		lea        esi, [esi+eax*2]
		pavgb      mm0, mm2
		pavgb      mm1, mm3
		pavgb      mm2, mm4
		pavgb      mm3, mm5
		pavgb      mm0, [edi]
		pavgb      mm1, [edi+8]
		pavgb      mm2, [edi+ebx]
		pavgb      mm3, [edi+ebx+8]
		movq       [edi], mm0
		movq       [edi+8], mm1
		movq       [edi+ebx], mm2
		movq       [edi+ebx+8], mm3
		lea        edi, [edi+ebx*2]
		movq       mm0, [esi]
		movq       mm1, [esi+8]
		movq       mm2, [esi+eax]
		movq       mm3, [esi+eax+8]
		lea        esi, [esi+eax*2]
		pavgb      mm4, mm6
		pavgb      mm5, mm7
		pavgb      mm6, mm0
		pavgb      mm7, mm1
		pavgb      mm4, [edi]
		pavgb      mm5, [edi+8]
		pavgb      mm6, [edi+ebx]
		pavgb      mm7, [edi+ebx+8]
		movq       [edi], mm4
		movq       [edi+8], mm5
		movq       [edi+ebx], mm6
		movq       [edi+ebx+8], mm7
		lea        edi, [edi+ebx*2]
;-------------------------------------------------------------------
; ループ終端チェック
		test       ecx, ecx
		jnz        prediction_w16_fh_2nd_loop
;-------------------------------------------------------------------
; 最後の４行出力
		movq       mm4, [esi]
		movq       mm5, [esi+8]
		movq       mm6, [esi+eax]
		movq       mm7, [esi+eax+8]
		lea        esi, [esi+eax*2]
		pavgb      mm0, mm2
		pavgb      mm1, mm3
		pavgb      mm2, mm4
		pavgb      mm3, mm5
		pavgb      mm0, [edi]
		pavgb      mm1, [edi+8]
		pavgb      mm2, [edi+ebx]
		pavgb      mm3, [edi+ebx+8]
		movq       [edi], mm0
		movq       [edi+8], mm1
		movq       [edi+ebx], mm2
		movq       [edi+ebx+8], mm3
                lea        edi, [edi+ebx*2]
		movq       mm0, [esi]
		movq       mm1, [esi+8]
		pavgb      mm4, mm6
		pavgb      mm5, mm7
		pavgb      mm6, mm0
		pavgb      mm7, mm1
		pavgb      mm4, [edi]
		pavgb      mm5, [edi+8]
		pavgb      mm6, [edi+ebx]
		pavgb      mm7, [edi+ebx+8]
		movq       [edi], mm4
		movq       [edi+8], mm5
		movq       [edi+ebx], mm6
		movq       [edi+ebx+8], mm7
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w16_fh_2nd_sse@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Half/Full, 1st）水平 Half, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w16_hf_1st_sse@20
;      void __stdcall  prediction_w16_hf_1st_sse(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_hf_1st_sse@20 PROC
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
		shr        ecx, 2
;-------------------------------------------------------------------
; loop
prediction_w16_hf_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movq       mm0, [esi]
		movq       mm1, [esi+8]
		movq       mm2, [esi+eax]
		movq       mm3, [esi+eax+8]
		pavgb      mm0, [esi+1]
		pavgb      mm1, [esi+9]
		pavgb      mm2, [esi+eax+1]
		pavgb      mm3, [esi+eax+9]
		lea        esi, [esi+eax*2]
		movq       [edi], mm0
		movq       [edi+8], mm1
		movq       [edi+ebx], mm2
		movq       [edi+ebx+8], mm3
		lea        edi, [edi+ebx*2]
		movq       mm4, [esi]
		movq       mm5, [esi+8]
		movq       mm6, [esi+eax]
		movq       mm7, [esi+eax+8]
		pavgb      mm4, [esi+1]
		pavgb      mm5, [esi+9]
		pavgb      mm6, [esi+eax+1]
		pavgb      mm7, [esi+eax+9]
		lea        esi, [esi+eax*2]
		movq       [edi], mm4
		movq       [edi+8], mm5
		movq       [edi+ebx], mm6
		movq       [edi+ebx+8], mm7
		lea        edi, [edi+ebx*2]
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

_prediction_w16_hf_1st_sse@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Half/Full, 2nd）水平 Half, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w16_hf_2nd_sse@20
;      void __stdcall  prediction_w16_hf_2nd_sse(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_hf_2nd_sse@20 PROC
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
		shr        ecx, 2
;-------------------------------------------------------------------
; loop
prediction_w16_hf_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movq       mm0, [esi]
		movq       mm1, [esi+8]
		movq       mm2, [esi+eax]
		movq       mm3, [esi+eax+8]
		pavgb      mm0, [esi+1]
		pavgb      mm1, [esi+9]
		pavgb      mm2, [esi+eax+1]
		pavgb      mm3, [esi+eax+9]
		lea        esi, [esi+eax*2]
		pavgb      mm0, [edi]
		pavgb      mm1, [edi+8]
		pavgb      mm2, [edi+ebx]
		pavgb      mm3, [edi+ebx+8]
		movq       [edi], mm0
		movq       [edi+8], mm1
		movq       [edi+ebx], mm2
		movq       [edi+ebx+8], mm3
		lea        edi, [edi+ebx*2]
		movq       mm4, [esi]
		movq       mm5, [esi+8]
		movq       mm6, [esi+eax]
		movq       mm7, [esi+eax+8]
		pavgb      mm4, [esi+1]
		pavgb      mm5, [esi+9]
		pavgb      mm6, [esi+eax+1]
		pavgb      mm7, [esi+eax+9]
		lea        esi, [esi+eax*2]
		pavgb      mm4, [edi]
		pavgb      mm5, [edi+8]
		pavgb      mm6, [edi+ebx]
		pavgb      mm7, [edi+ebx+8]
		movq       [edi], mm4
		movq       [edi+8], mm5
		movq       [edi+ebx], mm6
		movq       [edi+ebx+8], mm7
		lea        edi, [edi+ebx*2]
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

_prediction_w16_hf_2nd_sse@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 16, Full/Full, 2nd）水平 Full, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w16_ff_2nd_sse@20
;      void __stdcall  prediction_w16_ff_2nd_sse(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w16_ff_2nd_sse@20 PROC
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
prediction_w16_ff_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movq       mm0, [esi]
		movq       mm1, [esi+8]
		movq       mm2, [esi+eax]
		movq       mm3, [esi+eax+8]
		lea        esi, [esi+eax*2]
		pavgb      mm0, [edi]
		pavgb      mm1, [edi+8]
		pavgb      mm2, [edi+ebx]
		pavgb      mm3, [edi+ebx+8]
		movq       [edi], mm0
		movq       [edi+8], mm1
		movq       [edi+ebx], mm2
		movq       [edi+ebx+8], mm3
		lea        edi, [edi+ebx*2]
		movq       mm4, [esi]
		movq       mm5, [esi+8]
		movq       mm6, [esi+eax]
		movq       mm7, [esi+eax+8]
		lea        esi, [esi+eax*2]
		pavgb      mm4, [edi]
		pavgb      mm5, [edi+8]
		pavgb      mm6, [edi+ebx]
		pavgb      mm7, [edi+ebx+8]
		movq       [edi], mm4
		movq       [edi+8], mm5
		movq       [edi+ebx], mm6
		movq       [edi+ebx+8], mm7
		lea        edi, [edi+ebx*2]
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

_prediction_w16_ff_2nd_sse@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Full/Half, 1st）水平 Full, 垂直 Half
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w8_fh_1st_sse@20
;      void __stdcall  prediction_w8_fh_1st_sse(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_fh_1st_sse@20 PROC
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
		movq       mm0, [esi]
		add        esi, eax
;-------------------------------------------------------------------
; loop
prediction_w8_fh_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movq       mm1, [esi]
		add        esi, eax
		movq       mm2, [esi]
		add        esi, eax
		movq       mm3, [esi]
		add        esi, eax
		movq       mm4, [esi]
                add        esi, eax
		pavgb      mm0, mm1
		pavgb      mm1, mm2
		pavgb      mm2, mm3
		pavgb      mm3, mm4
		movq       [edi], mm0
		add        edi, ebx
		movq       [edi], mm1
		add        edi, ebx
		movq       [edi], mm2
		add        edi, ebx
		movq       [edi], mm3
		add        edi, ebx
		movq       mm0, mm4
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

_prediction_w8_fh_1st_sse@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Full/Half, 2nd）水平 Full, 垂直 Half
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w8_fh_2nd_sse@20
;      void __stdcall  prediction_w8_fh_2nd_sse(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_fh_2nd_sse@20 PROC
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
		movq       mm0, [esi]
		add        esi, eax
;-------------------------------------------------------------------
; loop
prediction_w8_fh_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movq       mm1, [esi]
		add        esi, eax
		movq       mm2, [esi]
		add        esi, eax
		movq       mm3, [esi]
		add        esi, eax
		movq       mm4, [esi]
		add        esi, eax
		pavgb      mm0, mm1
		pavgb      mm1, mm2
		pavgb      mm2, mm3
		pavgb      mm3, mm4
		pavgb      mm0, [edi]
		pavgb      mm1, [edi+ebx]
		movq       [edi], mm0
		movq       [edi+ebx], mm1
		lea        edi, [edi+ebx*2]
		pavgb      mm2, [edi]
		pavgb      mm3, [edi+ebx]
		movq       [edi], mm2
		movq       [edi+ebx], mm3
		lea        edi, [edi+ebx*2]
		movq       mm0, mm4
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

_prediction_w8_fh_2nd_sse@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Half/Full, 1st）水平 Half, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w8_hf_1st_sse@20
;      void __stdcall  prediction_w8_hf_1st_sse(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_hf_1st_sse@20 PROC
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
prediction_w8_hf_1st_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movq       mm0, [esi]
		movq       mm1, [esi+eax]
		pavgb      mm0, [esi+1]
		pavgb      mm1, [esi+eax+1]
		lea        esi, [esi+eax*2]
		movq       mm2, [esi]
		movq       mm3, [esi+eax]
		pavgb      mm2, [esi+1]
		pavgb      mm3, [esi+eax+1]
		lea        esi, [esi+eax*2]
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
		jnz        prediction_w8_hf_1st_loop
;-------------------------------------------------------------------
; レジスタ復元など後始末
		pop        eax
		pop        ebx
		pop        ecx
		pop        edi
		pop        esi

		ret        20

_prediction_w8_hf_1st_sse@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Half/Full, 2nd）水平 Half, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUBLIC              C _prediction_w8_hf_2nd_sse@20
;      void __stdcall  prediction_w8_hf_2nd_sse(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_hf_2nd_sse@20 PROC
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
prediction_w8_hf_2nd_loop:
		dec        ecx
;-------------------------------------------------------------------
; core
		movq       mm0, [esi]
		movq       mm1, [esi+eax]
		pavgb      mm0, [esi+1]
		pavgb      mm1, [esi+eax+1]
		lea        esi, [esi+eax*2]
		movq       mm2, [esi]
		movq       mm3, [esi+eax]
		pavgb      mm2, [esi+1]
		pavgb      mm3, [esi+eax+1]
		lea        esi, [esi+eax*2]
		pavgb      mm0, [edi]
		pavgb      mm1, [edi+ebx]
		movq       [edi], mm0
		movq       [edi+ebx], mm1
		lea        edi, [edi+ebx*2]
		pavgb      mm2, [edi]
		pavgb      mm3, [edi+ebx]
		movq       [edi], mm2
		movq       [edi+ebx], mm3
		lea        edi, [edi+ebx*2]
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

_prediction_w8_hf_2nd_sse@20 ENDP
;-------------------------------------------------------------------
; 終了

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 動き補償（width 8, Full/Full, 2nd）水平 Full, 垂直 Full
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 基本方針
;-------------------------------------------------------------------
PUBLIC              C _prediction_w8_ff_2nd_sse@20
;      void __stdcall  prediction_w8_ff_2nd_sse(
; [esp+ 4] unsigned char *in,
; [esp+ 8] unsigned char *out,
; [esp+12] int            in_step,
; [esp+16] int            out_step,
; [esp+20] int            height
; )
_prediction_w8_ff_2nd_sse@20 PROC
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
prediction_w8_ff_2nd_loop:
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
		pavgb      mm0, [edi]
		pavgb      mm1, [edi+ebx]
		movq       [edi], mm0
		movq       [edi+ebx], mm1
		lea        edi, [edi+ebx*2]
		pavgb      mm2, [edi]
		pavgb      mm3, [edi+ebx]
		movq       [edi], mm2
		movq       [edi+ebx], mm3
		lea        edi, [edi+ebx*2]
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

_prediction_w8_ff_2nd_sse@20 ENDP
;-------------------------------------------------------------------
; 終了

END