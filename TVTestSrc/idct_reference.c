/* Reference_IDCT.c, Inverse Discrete Fourier Transform, double precision          */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include <math.h>
#include "idct_clip_table.h"

#define IDCT_REFERENCE_C
#include "idct_reference.h"

/*  Perform IEEE 1180 reference (64-bit floating point, separable 8x1
 *  direct matrix multiply) Inverse Discrete Cosine Transform
*/
void __stdcall idct_reference(short *block);

/* cosine transform matrix for 8x1 IDCT */
static const double ref_dct_matrix[8][8] = {
	{    /* [0][0-7] */
		 3.5355339059327379e-001,  3.5355339059327379e-001,
		 3.5355339059327379e-001,  3.5355339059327379e-001,
		 3.5355339059327379e-001,  3.5355339059327379e-001,
		 3.5355339059327379e-001,  3.5355339059327379e-001,
	}, { /* [1][0-7] */
		 4.9039264020161522e-001,  4.1573480615127262e-001,
		 2.7778511650980114e-001,  9.7545161008064166e-002,
		-9.7545161008064096e-002, -2.7778511650980098e-001,
		-4.1573480615127267e-001, -4.9039264020161522e-001,
	}, { /* [2][0-7] */
		 4.6193976625564337e-001,  1.9134171618254492e-001,
		-1.9134171618254486e-001, -4.6193976625564337e-001,
		-4.6193976625564342e-001, -1.9134171618254517e-001,
		 1.9134171618254500e-001,  4.6193976625564326e-001,
	}, { /* [3][0-7] */
		 4.1573480615127262e-001, -9.7545161008064096e-002,
		-4.9039264020161522e-001, -2.7778511650980109e-001,
		 2.7778511650980092e-001,  4.9039264020161522e-001,
		 9.7545161008064388e-002, -4.1573480615127256e-001,
	}, { /* [4][0-7] */
		 3.5355339059327379e-001, -3.5355339059327373e-001,
		-3.5355339059327384e-001,  3.5355339059327368e-001,
		 3.5355339059327384e-001, -3.5355339059327334e-001,
		-3.5355339059327356e-001,  3.5355339059327329e-001,
	}, { /* [5][0-7] */
		 2.7778511650980114e-001, -4.9039264020161522e-001,
		 9.7545161008064152e-002,  4.1573480615127273e-001,
		-4.1573480615127256e-001, -9.7545161008064013e-002,
		 4.9039264020161533e-001, -2.7778511650980076e-001,
	}, { /* [6][0-7] */
		 1.9134171618254492e-001, -4.6193976625564342e-001,
		 4.6193976625564326e-001, -1.9134171618254495e-001,
		-1.9134171618254528e-001,  4.6193976625564337e-001,
		-4.6193976625564320e-001,  1.9134171618254478e-001,
	}, { /* [7][0-7] */
		 9.7545161008064166e-002, -2.7778511650980109e-001,
		 4.1573480615127273e-001, -4.9039264020161533e-001,
		 4.9039264020161522e-001, -4.1573480615127251e-001,
		 2.7778511650980076e-001, -9.7545161008064291e-002,
	},
};

void __stdcall idct_reference(short *block)
{
  int i, j, k, v;
  double partial_product;
  double tmp[64];

	__asm{
		finit;
	}

  for (i=0; i<8; i++)
    for (j=0; j<8; j++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= ref_dct_matrix[k][j]*block[8*i+k];

      tmp[8*i+j] = partial_product;
    }

  /* Transpose operation is integrated into address mapping by switching 
     loop order of i and j */

  for (j=0; j<8; j++)
    for (i=0; i<8; i++)
    {
      partial_product = 0.0;

      for (k=0; k<8; k++)
        partial_product+= ref_dct_matrix[k][i]*tmp[8*k+j];

      v = (int) floor(partial_product+0.5);
      block[8*i+j] = idct_clip_table[IDCT_CLIP_TABLE_OFFSET+v];
    }
}
