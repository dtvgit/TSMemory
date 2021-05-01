;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ����
.586
.mmx
.xmm
.model flat
_TEXT64 segment page public use32 'CODE'
		align 16


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; copy_i_block_to_frame_mmx - �C���g���u���b�N�̃t���[���ǉ�
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ��{���j
; short block[64] ���� unsigned char * �փR�s�[����ہA�N���b�s���O
; �e�[�u�����g���̂ł͂Ȃ��AMMX �̖O�a���Z�𗘗p����
;-------------------------------------------------------------------
; �C���^�[�t�F�[�X
PUBLIC              C _copy_i_block_to_frame_mmx@12
;      void __stdcall  copy_i_block_to_frame_mmx(
; [esp + 4] = unsigned short           *in,
; [esp + 8] = unsigned char            *out,
; [esp +12] = int                       step
; )
_copy_i_block_to_frame_mmx@12     PROC
;-------------------------------------------------------------------
; �g�p���郌�W�X�^
; edi - �o��
; esi - ����
; ecx - �X�e�b�v
; total 12 bytes
;-------------------------------------------------------------------
; ���W�X�^�̑ޔ�
		push       edi
		push       esi
		push       ecx
;-------------------------------------------------------------------
; ��������f�[�^���󂯎���Ă���
		mov        esi, [esp+12+ 4]
		mov        edi, [esp+12+ 8]
		mov        ecx, [esp+12+12]
;-------------------------------------------------------------------
; �R�A
		movq       mm0, [esi]
		movq       mm1, [esi+8]
		movq       mm2, [esi+16]
		movq       mm3, [esi+24]
		movq       mm4, [esi+32]
		movq       mm5, [esi+40]
		movq       mm6, [esi+48]
		movq       mm7, [esi+56]
		packuswb   mm0, mm1
		packuswb   mm2, mm3
		packuswb   mm4, mm5
		packuswb   mm6, mm7
		movq       [edi], mm0
		lea        edi, [edi+ecx]
		movq       [edi], mm2
		lea        edi, [edi+ecx]
		movq       [edi], mm4
		lea        edi, [edi+ecx]
		movq       [edi], mm6
		lea        edi, [edi+ecx]
; �㔼���R�s�[�I��
		movq       mm0, [esi+64]
		movq       mm1, [esi+72]
		movq       mm2, [esi+80]
		movq       mm3, [esi+88]
		movq       mm4, [esi+96]
		movq       mm5, [esi+104]
		movq       mm6, [esi+112]
		movq       mm7, [esi+120]
		packuswb   mm0, mm1
		packuswb   mm2, mm3
		packuswb   mm4, mm5
		packuswb   mm6, mm7
		movq       [edi], mm0
		lea        edi, [edi+ecx]
		movq       [edi], mm2
		lea        edi, [edi+ecx]
		movq       [edi], mm4
		lea        edi, [edi+ecx]
		movq       [edi], mm6
		lea        edi, [edi+ecx]
; �������R�s�[�I��
;-------------------------------------------------------------------
; ���W�X�^��������n��

		pop        ecx
		pop        esi
		pop        edi

		ret        12

_copy_i_block_to_frame_mmx@12 ENDP
;-------------------------------------------------------------------
; �I��



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; add_diff_to_frame_mmx - �����u���b�N�̃t���[���ǉ�
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ��{���j
; 16 bit ���x�� 4 ���񉉎Z�A�N���b�s���O�e�[�u���͎g�킸�A
; MMX �̖O�a���Z�𗘗p����
;-------------------------------------------------------------------
; �C���^�[�t�F�[�X
PUBLIC              C _add_diff_to_frame_mmx@12
;      void __stdcall  add_diff_to_frame_mmx(
; [esp + 4] = unsigned short           *in,
; [esp + 8] = unsigned char            *out,
; [esp +12] = int                       step
; )
_add_diff_to_frame_mmx@12     PROC
;-------------------------------------------------------------------
; �g�p���郌�W�X�^
; edi - �o��
; esi - ����
; ecx - �X�e�b�v
; total 12 bytes
;-------------------------------------------------------------------
; �p�r�Œ� MMX ���W�X�^
; mm7 - 0
;-------------------------------------------------------------------
; ���W�X�^�̑ޔ�
		push       edi
		push       esi
		push       ecx
;-------------------------------------------------------------------
; ��������f�[�^���󂯎���Ă���
		mov        esi, [esp+12+ 4]
		mov        edi, [esp+12+ 8]
		mov        ecx, [esp+12+12]
;-------------------------------------------------------------------
; �ϊ��W���̍쐬
		pxor       mm7, mm7
;-------------------------------------------------------------------
; �R�A�i���[�v�͎g��Ȃ��A���̒��x�̋K�͂Ȃ�W�J���������ǂ��͂��j

; 0-1 �s
                movd       mm0, dword ptr [edi]
                movd       mm1, dword ptr [edi+4]
                movd       mm2, dword ptr [edi+ecx]
                movd       mm3, dword ptr [edi+ecx+4]
                punpcklbw  mm0, mm7
                punpcklbw  mm1, mm7
                punpcklbw  mm2, mm7
                punpcklbw  mm3, mm7
; �t���[���Q�s�ǂݎ��

                paddw      mm0, [esi]
                paddw      mm1, [esi+8]
                paddw      mm2, [esi+16]
                paddw      mm3, [esi+24]
                packuswb   mm0, mm1
                packuswb   mm2, mm3
; �����u���b�N�𑫂���

                movq       [edi], mm0
                lea        edi, [edi+ecx]
                movq       [edi], mm2
                lea        edi, [edi+ecx]
; �����߂�

; 2-3 �s
                movd       mm0, dword ptr [edi]
                movd       mm1, dword ptr [edi+4]
                movd       mm2, dword ptr [edi+ecx]
                movd       mm3, dword ptr [edi+ecx+4]
                punpcklbw  mm0, mm7
                punpcklbw  mm1, mm7
                punpcklbw  mm2, mm7
                punpcklbw  mm3, mm7
; �t���[���Q�s�ǂݎ��

                paddw      mm0, [esi+32]
                paddw      mm1, [esi+40]
                paddw      mm2, [esi+48]
                paddw      mm3, [esi+56]
                packuswb   mm0, mm1
                packuswb   mm2, mm3
; �����u���b�N�𑫂���

                movq       [edi], mm0
                lea        edi, [edi+ecx]
                movq       [edi], mm2
                lea        edi, [edi+ecx]
; �����߂�

; 4-5 �s
                movd       mm0, dword ptr [edi]
                movd       mm1, dword ptr [edi+4]
                movd       mm2, dword ptr [edi+ecx]
                movd       mm3, dword ptr [edi+ecx+4]
                punpcklbw  mm0, mm7
                punpcklbw  mm1, mm7
                punpcklbw  mm2, mm7
                punpcklbw  mm3, mm7
; �t���[���Q�s�ǂݎ��

                paddw      mm0, [esi+64]
                paddw      mm1, [esi+72]
                paddw      mm2, [esi+80]
                paddw      mm3, [esi+88]
                packuswb   mm0, mm1
                packuswb   mm2, mm3
; �����u���b�N�𑫂���

                movq       [edi], mm0
                lea        edi, [edi+ecx]
                movq       [edi], mm2
                lea        edi, [edi+ecx]
; �����߂�

; 6-7 �s
                movd       mm0, dword ptr [edi]
                movd       mm1, dword ptr [edi+4]
                movd       mm2, dword ptr [edi+ecx]
                movd       mm3, dword ptr [edi+ecx+4]
                punpcklbw  mm0, mm7
                punpcklbw  mm1, mm7
                punpcklbw  mm2, mm7
                punpcklbw  mm3, mm7
; �t���[���Q�s�ǂݎ��

                paddw      mm0, [esi+96]
                paddw      mm1, [esi+104]
                paddw      mm2, [esi+112]
                paddw      mm3, [esi+120]
                packuswb   mm0, mm1
                packuswb   mm2, mm3
; �����u���b�N�𑫂���

                movq       [edi], mm0
                lea        edi, [edi+ecx]
                movq       [edi], mm2
                lea        edi, [edi+ecx]
; �����߂�

;-------------------------------------------------------------------
; ���W�X�^��������n��

                pop        ecx
                pop        esi
                pop        edi

                ret        12

_add_diff_to_frame_mmx@12 ENDP
;-------------------------------------------------------------------
; �I��


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; setup_qw_mmx - �ʎq���s��Ɨʎq���W������d�݃e�[�u�����쐬����
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; �C���^�[�t�F�[�X
PUBLIC              C _setup_qw_mmx@12
;      void __stdcall  setup_qw_mmx(
; [esp + 4] = unsigned short           *qw,
; [esp + 8] = unsigned short           *qm,
; [esp +12] = int                       q
; )
_setup_qw_mmx@12     PROC
;-------------------------------------------------------------------
; �g�p���郌�W�X�^
; edi - �o��
; esi - ����
; total 8 bytes
;-------------------------------------------------------------------
; ���W�X�^�̑ޔ�
                push       edi
                push       esi
;-------------------------------------------------------------------
; ��������f�[�^���󂯎���Ă���
                mov        edi, [esp+ 8+ 4]
                mov        esi, [esp+ 8+ 8]
;-------------------------------------------------------------------
; �ϊ��W���̍쐬
                movd       mm7, dword ptr [esp+ 8+12]
                punpcklwd  mm7, mm7;
                punpckldq  mm7, mm7;
                movq       mm0, [esi];
                movq       mm1, [esi+8];
                movq       mm2, [esi+16];
                movq       mm3, [esi+24];
                movq       mm4, [esi+32];
                movq       mm5, [esi+40];
                movq       mm6, [esi+48];
                lea        esi, [esi+56]; 28
                pmullw     mm0, mm7;
                pmullw     mm1, mm7;
                pmullw     mm2, mm7;
                pmullw     mm3, mm7;
                pmullw     mm4, mm7;
                pmullw     mm5, mm7;
                pmullw     mm6, mm7;
                movq       [edi], mm0;
                movq       [edi+8], mm1;
                movq       [edi+16], mm2;
                movq       [edi+24], mm3;
                movq       [edi+32], mm4;
                movq       [edi+40], mm5;
                movq       [edi+48], mm6;
                lea        edi, [edi+56];
                movq       mm0, [esi];
                movq       mm1, [esi+8];
                movq       mm2, [esi+16];
                movq       mm3, [esi+24];
                movq       mm4, [esi+32];
                movq       mm5, [esi+40];
                movq       mm6, [esi+48];
                lea        esi, [esi+56]; 56
                pmullw     mm0, mm7;
                pmullw     mm1, mm7;
                pmullw     mm2, mm7;
                pmullw     mm3, mm7;
                pmullw     mm4, mm7;
                pmullw     mm5, mm7;
                pmullw     mm6, mm7;
                movq       [edi], mm0;
                movq       [edi+8], mm1;
                movq       [edi+16], mm2;
                movq       [edi+24], mm3;
                movq       [edi+32], mm4;
                movq       [edi+40], mm5;
                movq       [edi+48], mm6;
                lea        edi, [edi+56];
                movq       mm0, [esi];
                movq       mm1, [esi+8];
                movq       mm2, [esi+16];
                movq       mm3, [esi+24];
                movq       mm4, [esi+32];
                movq       mm5, [esi+40];
                movq       mm6, [esi+48];
                lea        esi, [esi+56]; 84
                pmullw     mm0, mm7;
                pmullw     mm1, mm7;
                pmullw     mm2, mm7;
                pmullw     mm3, mm7;
                pmullw     mm4, mm7;
                pmullw     mm5, mm7;
                pmullw     mm6, mm7;
                movq       [edi], mm0;
                movq       [edi+8], mm1;
                movq       [edi+16], mm2;
                movq       [edi+24], mm3;
                movq       [edi+32], mm4;
                movq       [edi+40], mm5;
                movq       [edi+48], mm6;
                lea        edi, [edi+56];
                movq       mm0, [esi];
                movq       mm1, [esi+8];
                movq       mm2, [esi+16];
                movq       mm3, [esi+24];
                movq       mm4, [esi+32];
                movq       mm5, [esi+40];
                movq       mm6, [esi+48];
                lea        esi, [esi+56]; 112
                pmullw     mm0, mm7;
                pmullw     mm1, mm7;
                pmullw     mm2, mm7;
                pmullw     mm3, mm7;
                pmullw     mm4, mm7;
                pmullw     mm5, mm7;
                pmullw     mm6, mm7;
                movq       [edi], mm0;
                movq       [edi+8], mm1;
                movq       [edi+16], mm2;
                movq       [edi+24], mm3;
                movq       [edi+32], mm4;
                movq       [edi+40], mm5;
                movq       [edi+48], mm6;
                lea        edi, [edi+56];
                movq       mm0, [esi];
                movq       mm1, [esi+8];
                movq       mm2, [esi+16];
                movq       mm3, [esi+24];
                movq       mm4, [esi+32];
                movq       mm5, [esi+40];
                movq       mm6, [esi+48];
                lea        esi, [esi+56]; 140
                pmullw     mm0, mm7;
                pmullw     mm1, mm7;
                pmullw     mm2, mm7;
                pmullw     mm3, mm7;
                pmullw     mm4, mm7;
                pmullw     mm5, mm7;
                pmullw     mm6, mm7;
                movq       [edi], mm0;
                movq       [edi+8], mm1;
                movq       [edi+16], mm2;
                movq       [edi+24], mm3;
                movq       [edi+32], mm4;
                movq       [edi+40], mm5;
                movq       [edi+48], mm6;
                lea        edi, [edi+56];
                movq       mm0, [esi];
                movq       mm1, [esi+8];
                movq       mm2, [esi+16];
                movq       mm3, [esi+24];
                movq       mm4, [esi+32];
                movq       mm5, [esi+40];
                movq       mm6, [esi+48];
                lea        esi, [esi+56]; 168
                pmullw     mm0, mm7;
                pmullw     mm1, mm7;
                pmullw     mm2, mm7;
                pmullw     mm3, mm7;
                pmullw     mm4, mm7;
                pmullw     mm5, mm7;
                pmullw     mm6, mm7;
                movq       [edi], mm0;
                movq       [edi+8], mm1;
                movq       [edi+16], mm2;
                movq       [edi+24], mm3;
                movq       [edi+32], mm4;
                movq       [edi+40], mm5;
                movq       [edi+48], mm6;
                lea        edi, [edi+56];
                movq       mm0, [esi];
                movq       mm1, [esi+8];
                movq       mm2, [esi+16];
                movq       mm3, [esi+24];
                movq       mm4, [esi+32];
                movq       mm5, [esi+40];
                movq       mm6, [esi+48];
                lea        esi, [esi+56]; 196
                pmullw     mm0, mm7;
                pmullw     mm1, mm7;
                pmullw     mm2, mm7;
                pmullw     mm3, mm7;
                pmullw     mm4, mm7;
                pmullw     mm5, mm7;
                pmullw     mm6, mm7;
                movq       [edi], mm0;
                movq       [edi+8], mm1;
                movq       [edi+16], mm2;
                movq       [edi+24], mm3;
                movq       [edi+32], mm4;
                movq       [edi+40], mm5;
                movq       [edi+48], mm6;
                lea        edi, [edi+56];
                movq       mm0, [esi];
                movq       mm1, [esi+8];
                movq       mm2, [esi+16];
                movq       mm3, [esi+24];
                movq       mm4, [esi+32];
                movq       mm5, [esi+40];
                movq       mm6, [esi+48];
                lea        esi, [esi+56]; 224
                pmullw     mm0, mm7;
                pmullw     mm1, mm7;
                pmullw     mm2, mm7;
                pmullw     mm3, mm7;
                pmullw     mm4, mm7;
                pmullw     mm5, mm7;
                pmullw     mm6, mm7;
                movq       [edi], mm0;
                movq       [edi+8], mm1;
                movq       [edi+16], mm2;
                movq       [edi+24], mm3;
                movq       [edi+32], mm4;
                movq       [edi+40], mm5;
                movq       [edi+48], mm6;
                lea        edi, [edi+56];
                movq       mm0, [esi];
                movq       mm1, [esi+8];
                movq       mm2, [esi+16];
                movq       mm3, [esi+24];
                movq       mm4, [esi+32];
                movq       mm5, [esi+40];
                movq       mm6, [esi+48];
                lea        esi, [esi+56]; 252
                pmullw     mm0, mm7;
                pmullw     mm1, mm7;
                pmullw     mm2, mm7;
                pmullw     mm3, mm7;
                pmullw     mm4, mm7;
                pmullw     mm5, mm7;
                pmullw     mm6, mm7;
                movq       [edi], mm0;
                movq       [edi+8], mm1;
                movq       [edi+16], mm2;
                movq       [edi+24], mm3;
                movq       [edi+32], mm4;
                movq       [edi+40], mm5;
                movq       [edi+48], mm6;
                movq       mm0, [esi];
                pmullw     mm0, mm7;
                movq       [edi+56], mm0;
;-------------------------------------------------------------------
; ���W�X�^��������n��

                pop        esi
                pop        edi

                ret        12

_setup_qw_mmx@12 ENDP
;-------------------------------------------------------------------
; �I��


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; setup_qw_sse2 - �ʎq���s��Ɨʎq���W������d�݃e�[�u�����쐬����
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; �C���^�[�t�F�[�X
PUBLIC              C _setup_qw_sse2@12
;      void __stdcall  setup_qw_sse2(
; [esp + 4] = unsigned short           *qw,
; [esp + 8] = unsigned short           *qm,
; [esp +12] = int                       q
; )
_setup_qw_sse2@12     PROC
;-------------------------------------------------------------------
; �g�p���郌�W�X�^
; edi - �o��
; esi - ����
; total 8 bytes
;-------------------------------------------------------------------
; ���W�X�^�̑ޔ�
                push       edi
                push       esi
;-------------------------------------------------------------------
; ��������f�[�^���󂯎���Ă���
                mov        edi, [esp+ 8+ 4]
                mov        esi, [esp+ 8+ 8]
;-------------------------------------------------------------------
; �ϊ��W���̍쐬
                movd       xmm7, dword ptr [esp+ 8+12]
                punpcklwd  xmm7, xmm7;
                pshufd     xmm7, xmm7, 00000000b;
                movdqa     xmm0, [esi];
                movdqa     xmm1, [esi+16];
                movdqa     xmm2, [esi+32];
                movdqa     xmm3, [esi+48];
                movdqa     xmm4, [esi+64];
                movdqa     xmm5, [esi+80];
                movdqa     xmm6, [esi+96];
                lea        esi, [esi+112]; 56
                pmullw     xmm0, xmm7;
                pmullw     xmm1, xmm7;
                pmullw     xmm2, xmm7;
                pmullw     xmm3, xmm7;
                pmullw     xmm4, xmm7;
                pmullw     xmm5, xmm7;
                pmullw     xmm6, xmm7;
                movdqa     [edi], xmm0;
                movdqa     [edi+16], xmm1;
                movdqa     [edi+32], xmm2;
                movdqa     [edi+48], xmm3;
                movdqa     [edi+64], xmm4;
                movdqa     [edi+80], xmm5;
                movdqa     [edi+96], xmm6;
                lea        edi, [edi+112];
                movdqa     xmm0, [esi];
                movdqa     xmm1, [esi+16];
                movdqa     xmm2, [esi+32];
                movdqa     xmm3, [esi+48];
                movdqa     xmm4, [esi+64];
                movdqa     xmm5, [esi+80];
                movdqa     xmm6, [esi+96];
                lea        esi, [esi+112]; 112
                pmullw     xmm0, xmm7;
                pmullw     xmm1, xmm7;
                pmullw     xmm2, xmm7;
                pmullw     xmm3, xmm7;
                pmullw     xmm4, xmm7;
                pmullw     xmm5, xmm7;
                pmullw     xmm6, xmm7;
                movdqa     [edi], xmm0;
                movdqa     [edi+16], xmm1;
                movdqa     [edi+32], xmm2;
                movdqa     [edi+48], xmm3;
                movdqa     [edi+64], xmm4;
                movdqa     [edi+80], xmm5;
                movdqa     [edi+96], xmm6;
                lea        edi, [edi+112];
                movdqa     xmm0, [esi];
                movdqa     xmm1, [esi+16];
                movdqa     xmm2, [esi+32];
                movdqa     xmm3, [esi+48];
                movdqa     xmm4, [esi+64];
                movdqa     xmm5, [esi+80];
                movdqa     xmm6, [esi+96];
                lea        esi, [esi+112]; 168
                pmullw     xmm0, xmm7;
                pmullw     xmm1, xmm7;
                pmullw     xmm2, xmm7;
                pmullw     xmm3, xmm7;
                pmullw     xmm4, xmm7;
                pmullw     xmm5, xmm7;
                pmullw     xmm6, xmm7;
                movdqa     [edi], xmm0;
                movdqa     [edi+16], xmm1;
                movdqa     [edi+32], xmm2;
                movdqa     [edi+48], xmm3;
                movdqa     [edi+64], xmm4;
                movdqa     [edi+80], xmm5;
                movdqa     [edi+96], xmm6;
                lea        edi, [edi+112];
                movdqa     xmm0, [esi];
                movdqa     xmm1, [esi+16];
                movdqa     xmm2, [esi+32];
                movdqa     xmm3, [esi+48];
                movdqa     xmm4, [esi+64];
                movdqa     xmm5, [esi+80];
                movdqa     xmm6, [esi+96];
                pmullw     xmm0, xmm7;
                pmullw     xmm1, xmm7;
                pmullw     xmm2, xmm7;
                pmullw     xmm3, xmm7;
                pmullw     xmm4, xmm7;
                pmullw     xmm5, xmm7;
                pmullw     xmm6, xmm7;
                movdqa     [edi], xmm0;
                movdqa     [edi+16], xmm1;
                movdqa     [edi+32], xmm2;
                movdqa     [edi+48], xmm3;
                movdqa     [edi+64], xmm4;
                movdqa     [edi+80], xmm5;
                movdqa     [edi+96], xmm6;
                movdqa     xmm0, [esi+112];
                movdqa     xmm1, [esi+128];
                movdqa     xmm2, [esi+144];
                movdqa     xmm3, [esi+160];
                pmullw     xmm0, xmm7;
                pmullw     xmm1, xmm7;
                pmullw     xmm2, xmm7;
                pmullw     xmm3, xmm7;
                movdqa     [edi+112], xmm0;
                movdqa     [edi+128], xmm1;
                movdqa     [edi+144], xmm2;
                movdqa     [edi+160], xmm3;
;-------------------------------------------------------------------
; ���W�X�^��������n��

                pop        esi
                pop        edi

                ret        12

_setup_qw_sse2@12 ENDP
;-------------------------------------------------------------------
; �I��


END