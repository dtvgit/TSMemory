;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; お約束
.586
.mmx
.model flat
_TEXT64 segment page public use32 'CODE'
		align 8
;-------------------------------------------------------------------
;IEEE test conditions: -L = -256, +H = 256, sign = 1, #iters = 10000
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
;   0.0095   0.0121   0.0110   0.0105   0.0129   0.0088   0.0093   0.0082
;   0.0080   0.0103   0.0073   0.0115   0.0108   0.0080   0.0113   0.0091
;   0.0096   0.0113   0.0075   0.0133   0.0098   0.0093   0.0122   0.0097
;   0.0073   0.0133   0.0087   0.0131   0.0113   0.0113   0.0140   0.0105
;   0.0089   0.0122   0.0115   0.0110   0.0139   0.0092   0.0149   0.0089
;   0.0086   0.0121   0.0076   0.0119   0.0105   0.0085   0.0100   0.0095
;   0.0084   0.0132   0.0066   0.0113   0.0105   0.0100   0.0098   0.0074
;   0.0099   0.0107   0.0108   0.0105   0.0115   0.0074   0.0143   0.0078
;Worst pmse = 0.014900  (meets spec limit 0.06)
;Overall mse = 0.010314  (meets spec limit 0.02)
;
;Mean errors:
;  -0.0007  -0.0009   0.0000  -0.0017   0.0003  -0.0002   0.0003  -0.0002
;  -0.0008   0.0017  -0.0013   0.0009   0.0010   0.0004  -0.0001   0.0001
;  -0.0006  -0.0013  -0.0019  -0.0005   0.0006  -0.0005   0.0012  -0.0001
;   0.0001   0.0003  -0.0003  -0.0017   0.0023   0.0011  -0.0010  -0.0017
;  -0.0007   0.0010   0.0009   0.0016  -0.0009   0.0008   0.0001   0.0009
;   0.0002   0.0005   0.0012  -0.0015   0.0001  -0.0013   0.0000  -0.0015
;  -0.0006   0.0002   0.0008   0.0001   0.0011   0.0004  -0.0008   0.0000
;   0.0013  -0.0003   0.0004   0.0021  -0.0013   0.0010  -0.0005   0.0000
;Worst mean error = 0.002300  (meets spec limit 0.015)
;Overall mean error = 0.000002  (meets spec limit 0.0015)
;
;0 elements of IDCT(0) were not zero
;-------------------------------------------------------------------
;-------------------------------------------------------------------
; idct_llm_mmx - 方針
; 
; 32 bit 精度で 2 並列計算（16bit では精度がとれないので）
; RAW -> COL 間での通過ビットは +3
; 固定小数点定数は 13 ビット精度
; AP-922 は madd 8 回、add 8 回、もう少し何とかしたい
; idct 中にアクセスするメモリ領域は
;  2 * 64 - block
;  4 * 64 - work
;  + α
; だけなので、メモリ IO でキャッシュヒットを期待できる……はず
;-------------------------------------------------------------------
; 定数
e_coeff1     dq 0d6311151115129cfh ; 
o_coeff1     dq 025a108d408d41924h ; (o1_o2 & s[5]_s[1])
o_coeff2     dq 0d39ee6de25a1d39eh ; (o1_o2 & s[7]_s[3])
o_coeff3     dq 0d39e25a119242c62h ; (o3_o4 & s[5]_s[1])
o_coeff4     dq 0e6def72c08d425a1h ; (o3_o4 & s[7]_s[3])
half_10bit   dq 00000020000000200h
half_19bit   dq 00004000000040000h
p0_298631336 dq 00000098e0000098eh
p2_053119869 dq 0000041b3000041b3h
m0_899976223 dq 00000e3340000e334h
m2_562915447 dq 00000adfd0000adfdh
p3_072711026 dq 00000625300006253h
p1_501321110 dq 00000300a0000300ah
p1_175875602 dq 0000025a1000025a1h
m1_961570560 dq 00000c13b0000c13bh
m0_390180644 dq 00000f3840000f384h
p0_765366865 dq 00000187d0000187dh
m1_847759065 dq 00000c4e00000c4e0h
p0_541196100 dq 00000115100001151h
;-------------------------------------------------------------------
PUBLIC              C _idct_llm_mmx@4
;      void __stdcall  idct_llm_mmx(
; [esp + 4] = short *block,
; )
_idct_llm_mmx@4 PROC
;-------------------------------------------------------------------
; 使用レジスタ
; esi - 入力
; edi - 出力
; ecx - ループカウンタ
; eax - スタック調整
; ebx - 作業領域
; total 20 bytes
;-------------------------------------------------------------------
; ローカル変数
; tmp[4]        8 byte * 4      [esp+ 4] ～ 
; work[64]      4 byte * 64     [esp+36] ～ 
; total 288 bytes
;-------------------------------------------------------------------
; レジスタ退避
		push       esi
		push       edi
		push       ecx
		push       eax
		push       ebx
;-------------------------------------------------------------------
; 引数からのデータ受け取り
		mov        esi, [esp+20+4]
;-------------------------------------------------------------------
; ローカル変数領域確保
		mov        eax, esp
		and        eax, 7
		sub        esp, eax
		sub        esp, 292
; 64 bit 境界で 288 + 4 bytes 確保
;-------------------------------------------------------------------
; 入出力パラメータ設定
		lea        edi, [esp+36]
		mov        ecx, 8
;-------------------------------------------------------------------
; IDCT_RAW
idct_llm_mmx_raw_loop:
		dec        ecx
;-------------------------------------------------------------------
; idct_llm_mmx_raw - 計算内容
;
; e1 = ( (s[0] + s[4]) << 13) + ( s[2] * 1.306562965 + s[6] * 0.541196100)
; e2 = ( (s[0] - s[4]) << 13) + ( s[2] * 0.541196100 - s[6] * 1.306562965)
; e3 = ( (s[0] - s[4]) << 13) - ( s[2] * 0.541196100 - s[6] * 1.306562965)
; e4 = ( (s[0] + s[4]) << 13) - ( s[2] * 1.306562965 + s[6] * 0.541196100)
;
; d631_1151_1151_29cf (-1.306562965)_( 0.541196100)_( 0.541196100)_( 1.306562965)
;
; o1 = s[7] * (-1.387039845) + s[3] * (-0.785694958) + s[5] * ( 1.175875602) + s[1] * ( 0.275899379)
; o2 = s[7] * ( 1.175875602) + s[3] * (-1.387039845) + s[5] * ( 0.275899379) + s[1] * ( 0.785694958)
; o3 = s[7] * (-0.785694958) + s[3] * (-0.275899379) + s[5] * (-1.387039845) + s[1] * ( 1.175875602)
; o4 = s[7] * ( 0.275899379) + s[3] * ( 1.175875602) + s[5] * ( 0.785694958) + s[1] * ( 1.387039845)
;
; 25a1_08d4_08d4_1924 ( 1.175875602)_( 0.275899379)_( 0.275899379)_( 0.785694958) : (o1_o2 & s[5]_s[1])
; d39e_e6de_25a1_d39e (-1.387039845)_(-0.785694958)_( 1.175875602)_(-1.387039845) : (o1_o2 & s[7]_s[3])
; d39e_25a1_1924_2c62 (-1.387039845)_( 1.175875602)_( 0.785694958)_( 1.387039845) : (o3_o4 & s[5]_s[1])
; e6de_f72c_08d4_25a1 (-0.785694958)_(-0.275899379)_( 0.275899379)_( 1.175875602) : (o3_o4 & s[7]_s[3])
;
; w[0] = e1 + o4
; w[1] = e2 + o3
; w[2] = e3 + o2
; w[3] = e4 + o1
; w[4] = e4 - o1
; w[5] = e3 - o2
; w[6] = e2 - o3
; w[7] = e1 - o4
;-------------------------------------------------------------------
; AC 係数をチェックして、全て 0 ならスキップする（DC 係数をコピー）
; 量子化の影響で、高周波成分では AC 係数が 0 になることが多いため
; 多少高速化できる（チェックが増えても）
;
		movq       mm6, [esi]         ; s3_s2_s1_s0
		movq       mm7, [esi+8]       ; s7_s6_s5_s4
		movq       mm4, mm6
		psrlq      mm4, 16
		por        mm4, mm7
		movq       mm5, mm4
		psrlq      mm5, 32
		por        mm4, mm5
		movd       ebx, mm4
		test       ebx, ebx

		jnz        idct_llm_mmx_raw_calc

		pslld      mm6, 16
		psrad      mm6, 13
		punpckldq  mm6, mm6
		movq       [edi   ], mm6
		movq       [edi+ 8], mm6
		movq       [edi+16], mm6
		movq       [edi+24], mm6

		jmp        idct_llm_mmx_raw_last

idct_llm_mmx_raw_calc:
;-------------------------------------------------------------------
; 第１段 e1_e2, e3_e4 形式を作る
; madd 1 個 で計算する（ただし、シフト演算等は増える）
;
; 必要なもの
; s[6]_s[2]_s[6]_s[2]
; s[0]-s[4]_s[0]+s[4]
;
		movq       mm0, mm6
		movq       mm1, mm7
		movq       mm2, mm6
		movq       mm3, mm7
		paddw      mm1, mm6           ; s[0] + s[4]
		psubw      mm0, mm7           ; s[0] - s[4]
		punpckhwd  mm2, mm3           ; s[7]_s[3]_s[6]_s[2]
		punpckldq  mm1, mm0           ; 
		movq       mm4, mm2           ; 後で奇数項の計算に使用
		punpckldq  mm2, mm2           ; s[6]_s[2]_s[6]_s[2]
		pslld      mm1, 16            ; 
		psrad      mm1, 3             ; s[0]-s[4]_s[0]+s[4]
; 使用レジスタ mm1, mm2
; 計算して、e3_e1, e4_e2 を作り e2_e1, e4_e3 形式に変換する
		pmaddwd    mm2, e_coeff1      ; (-1.306562965)_( 0.541196100)_( 0.541196100)_( 1.306562965) をかける
		movq       mm0, mm2           ;
		paddd      mm0, mm1           ; e2_e1
		psubd      mm1, mm2           ; e3_e4
		movq       mm2, mm1
		psrlq      mm2, 32            ; ___e3
		punpckldq  mm2, mm1           ; e4_e3
; 使用レジスタ mm0, mm2 
; madd 1 回 add 2 回 sub 2 回 unpck 4.5 回
;-------------------------------------------------------------------
; 第２段 o3_o4, o1_o2 形式を作る
; muladd 4 個で計算する
; 
; 必要なもの
; s[5]_s[1]_s[5]_s[1]
; s[7]_s[3]_s[7]_s[3]
;
		punpcklwd  mm6, mm7           ; s[5]_s[1]_s[4]_s[0]
		punpckhdq  mm4, mm4           ; s[7]_s[3]_s[7]_s[3]
		punpckhdq  mm6, mm6           ; s[5]_s[1]_s[5]_s[1]
		movq       mm7, mm4           ; s[7]_s[3]_s[7]_s[3]
		movq       mm5, mm6           ; s[5]_s[1]_s[5]_s[1]
		pmaddwd    mm6, o_coeff3      ; 
		pmaddwd    mm7, o_coeff4      ;
		pmaddwd    mm4, o_coeff2      ;
		pmaddwd    mm5, o_coeff1      ;
		paddd      mm6, mm7           ; o3_o4
		paddd      mm4, mm5           ; o1_o2
; madd 4 回 add 2 回 unpck 3.5 回
;-------------------------------------------------------------------
; 第３段 o1_o2, o3_o4, e2_e1, e4_e3 を結合して出力する
;
; 現在のレジスタ使用状況
; mm0 - e2_e1
; mm2 - e4_e3
; mm4 - o1_o2
; mm6 - o3_o4
;
		movq       mm1, mm0
		movq       mm3, mm2
		paddd      mm0, mm6           ; w[1]_w[0]
		paddd      mm2, mm4           ; w[3]_w[2]
		psubd      mm1, mm6           ; w[6]_w[7]
		psubd      mm3, mm4           ; w[4]_w[5]
		movq       mm5, mm1           ; w[6]_w[7]
		movq       mm7, mm3           ; w[4]_w[5]
		psllq      mm7, 32            ; w[5]_____
		psllq      mm5, 32            ; w[7]_____
		punpckhdq  mm3, mm7           ; w[5]_w[4]
		punpckhdq  mm1, mm5           ; w[7]_w[6]
		paddd      mm0, half_10bit
		paddd      mm1, half_10bit
		paddd      mm2, half_10bit
		paddd      mm3, half_10bit
		psrad      mm0, 10
		psrad      mm1, 10
		psrad      mm2, 10
		psrad      mm3, 10
		movq       [edi   ], mm0
		movq       [edi+ 8], mm2
		movq       [edi+16], mm3
		movq       [edi+24], mm1
;add 6, sub 2, psxl 4, psxa 4,
;-------------------------------------------------------------------
; 終端チェック
idct_llm_mmx_raw_last:
		lea        esi, [esi+16]
		lea        edi, [edi+32]
		test       ecx, ecx
		jnz        idct_llm_mmx_raw_loop
;-------------------------------------------------------------------
; 入出力パラメータ修正

		mov        edi, esi
		sub        edi, 128
		lea        esi, [esp+36]
		mov        ecx, 4
;-------------------------------------------------------------------
; IDCT_COL
idct_llm_mmx_col_loop:
		dec        ecx
;-------------------------------------------------------------------
; 第１段 奇数項を計算する
;
;		w0 = w[7*8];
;		w1 = w[5*8];
;		w2 = w[3*8];
;		w3 = w[1*8];
;
;		z1 = w0 + w3;
;		z2 = w1 + w2;
;		z3 = w0 + w2;
;		z4 = w1 + w3;
;		z5 = (z3 + z4) * FIX_1_175875602;
;
;		w0 *= FIX_0_298631336;
;		w1 *= FIX_2_053119869;
;		w2 *= FIX_3_072711026;
;		w3 *= FIX_1_501321110;
;		z1 *= (- FIX_0_899976223);
;		z2 *= (- FIX_2_562915447);
;		z3 *= (- FIX_1_961570560);
;		Z4 *= (- FIX_0_390180644);
;
;		z3 += z5;
;		z4 += z5;
;
;		w0 += z1 + z3;
;		w1 += z2 + z4;
;		w2 += z2 + z3;
;		w3 += z1 + z4;
;
		movq       mm0, [esi+7*32]
		movq       mm1, [esi+5*32]
		movq       mm2, [esi+3*32]
		movq       mm3, [esi+1*32]

		movq       mm4, mm0
		movq       mm5, mm1
		paddd      mm4, mm3           ; z1
		paddd      mm5, mm2           ; z2
		movq       mm6, mm0
		movq       mm7, mm1
		paddd      mm6, mm2           ; z3
		paddd      mm7, mm3           ; z4
		pmaddwd    mm0, p0_298631336
		pmaddwd    mm1, p2_053119869
		pmaddwd    mm4, m0_899976223
		pmaddwd    mm5, m2_562915447
		pmaddwd    mm2, p3_072711026
		pmaddwd    mm3, p1_501321110
		paddd      mm0, mm4
		paddd      mm1, mm5
		paddd      mm2, mm5
		paddd      mm3, mm4
		movq       mm5, mm6           ; z5
		paddd      mm5, mm7
		pmaddwd    mm5, p1_175875602
		pmaddwd    mm6, m1_961570560
		pmaddwd    mm7, m0_390180644
		paddd      mm6, mm5
		paddd      mm7, mm5
		paddd      mm0, mm6
		paddd      mm1, mm7
		paddd      mm2, mm6
		paddd      mm3, mm7
		movq       [esp+ 4], mm0
		movq       [esp+12], mm1
		movq       [esp+20], mm2
		movq       [esp+28], mm3
;-------------------------------------------------------------------
; 第２段 偶数項を計算する
;
;		z2 = w[2*8];
;		z3 = w[6*8];
;
;		z1 = (z2+z3) * FIX_0_541196100;
;		w2 = z1 + (z3 * (- FIX_1_847759065));
;		w3 = z1 + (z2 * FIX_0_765366865);
;
;		w0 = (w[0*8] + w[4*8]) << 13;
;		w1 = (w[0*8] - w[4*8]) << 13;
;
;		w4 = w0 + w3;
;		w7 = w0 - w3;
;		w5 = w1 + w2;
;		w6 = w1 - w2;
;
		movq       mm0, [esi]         ;
		movq       mm2, [esi+32*2]    ;
		movq       mm4, [esi+32*4]    ;
		movq       mm6, [esi+32*6]    ;
		movq       mm3, mm2           ; z1
		movq       mm5, mm0           ; 
		paddd      mm0, mm4           ; 
		psubd      mm5, mm4
		paddd      mm3, mm6
		pslld      mm0, 13            ; w0
		pslld      mm5, 13            ; w1
		pmaddwd    mm2, p0_765366865
		pmaddwd    mm6, m1_847759065
		pmaddwd    mm3, p0_541196100
		paddd      mm2, mm3           ; w3
		paddd      mm6, mm3           ; w2
		movq       mm7, mm0           ; w0
		movq       mm3, mm5           ; w1
		paddd      mm0, mm2           ; w4
		paddd      mm3, mm6           ; w5
		psubd      mm7, mm2           ; w7
		psubd      mm5, mm6           ; w6
		movq       mm1, mm0
		movq       mm2, mm3
		movq       mm4, mm5
		movq       mm6, mm7
;-------------------------------------------------------------------
; 第３段 奇数項と偶数項から最終出力を計算して元の block に戻す
;
;		(w4+w3) >> 20;
;		(w5+w2) >> 20;
;		(w6+w1) >> 20;
;		(w7+w0) >> 20;
;		(w7-w0) >> 20;
;		(w6-w1) >> 20;
;		(w5-w2) >> 20;
;		(w4-w3) >> 20;		
;
		paddd      mm6, [esp+ 4]      ; d[3]
		paddd      mm4, [esp+12]      ; d[2]
		paddd      mm2, [esp+20]      ; d[1]
		paddd      mm0, [esp+28]      ; d[0]
		psubd      mm7, [esp+ 4]      ; d[4]
		psubd      mm5, [esp+12]      ; d[5]
		psubd      mm3, [esp+20]      ; d[6]
		psubd      mm1, [esp+28]      ; d[7]
		paddd      mm6, half_19bit
		paddd      mm4, half_19bit
		paddd      mm2, half_19bit
		paddd      mm0, half_19bit
		paddd      mm7, half_19bit
		paddd      mm5, half_19bit
		paddd      mm3, half_19bit
		paddd      mm1, half_19bit
		psrad      mm6, 19
		psrad      mm4, 19
		psrad      mm2, 19
		psrad      mm0, 19
		psrad      mm7, 19
		psrad      mm5, 19
		psrad      mm3, 19
		psrad      mm1, 19
		packssdw   mm0, mm7           ; d[4]_d[0]
		packssdw   mm2, mm5           ; d[5]_d[1]
		packssdw   mm4, mm3           ; d[6]_d[2]
		packssdw   mm6, mm1           ; d[7]_d[3]
		movd       dword ptr [edi     ], mm0
		movd       dword ptr [edi+16*1], mm2
		movd       dword ptr [edi+16*2], mm4
		movd       dword ptr [edi+16*3], mm6
		psrlq      mm0, 32
		psrlq      mm2, 32
		psrlq      mm4, 32
		psrlq      mm6, 32
		movd       dword ptr [edi+16*4], mm0
		movd       dword ptr [edi+16*5], mm2
		movd       dword ptr [edi+16*6], mm4
		movd       dword ptr [edi+16*7], mm6
;-------------------------------------------------------------------
; 終端チェック
		lea        esi, [esi+8]
		lea        edi, [edi+4]
		test       ecx, ecx
		jnz        idct_llm_mmx_col_loop
;-------------------------------------------------------------------
; 後始末
		add        esp, 292
		add        esp, eax

		pop        ebx
		pop        eax
		pop        ecx
		pop        edi
		pop        esi

		ret        4
;-------------------------------------------------------------------
_idct_llm_mmx@4 ENDP
;-------------------------------------------------------------------
; 終了

END
