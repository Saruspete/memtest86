/* test.c - MemTest-86  Version 3.4
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady
 * ----------------------------------------------------
 * MemTest86+ V5 Specific code (GPL V2.0)
 * By Samuel DEMEULEMEESTER, sdemeule@memtest.org
 * http://www.canardpc.com - http://www.memtest.org
 * Thanks to Passmark for calculate_chunk() and various comments !
 */

#include "test.h"
#include "config.h"
#include "stdint.h"
#include "cpuid.h"
#include "smp.h"
#include "io.h"

extern struct cpu_ident cpu_id;
extern volatile int    mstr_cpu;
extern volatile int    run_cpus;
extern volatile int    test;
extern volatile int segs, bail;
extern int test_ticks, nticks;
extern struct tseq tseq[];
extern void update_err_counts(void);
extern void print_err_counts(void);
void rand_seed( unsigned int seed1, unsigned int seed2, int me);
ulong rand(int me);
void poll_errors();

// NOTE(jcoiner):
//  Defining 'STATIC' to empty string results in crashes. (It should
//  work fine, of course.) I suspect relocation problems in reloc.c.
//  When we declare these routines static, we use relative addresses
//  for them instead of looking up their addresses in (supposedly
//  relocated) global elf tables, which avoids the crashes.

#define STATIC static
//#define STATIC

#define PREFER_C 0

static const void* const nullptr = 0x0;

// Writes *start and *end with the VA range to test.
//
// me - this threads CPU number
// j - index into v->map for current segment we are testing
// align - number of bytes to align each block to
STATIC void calculate_chunk(ulong** start, ulong** end, int me,
                            int j, int makeMultipleOf) {
    ulong chunk;

    // If we are only running 1 CPU then test the whole block
    if (run_cpus == 1) {
        *start = vv->map[j].start;
        *end = vv->map[j].end;
    } else {

        // Divide the current segment by the number of CPUs
        chunk = (ulong)vv->map[j].end-(ulong)vv->map[j].start;
        chunk /= run_cpus;
		
        // Round down to the nearest desired bitlength multiple
        chunk = (chunk + (makeMultipleOf-1)) &  ~(makeMultipleOf-1);

        // Figure out chunk boundaries
        *start = (ulong*)((ulong)vv->map[j].start+(chunk*me));
        /* Set end addrs for the highest CPU num to the
         * end of the segment for rounding errors */
        /* Also rounds down to boundary if needed, may miss some ram but
           better than crashing or producing false errors. */
        /* This rounding probably will never happen as the segments should
           be in 4096 bytes pages if I understand correctly. */
        if (me == mstr_cpu) {
            *end = (ulong*)(vv->map[j].end);
        } else {
            *end = (ulong*)((ulong)(*start) + chunk);
            (*end)--;
        }
    }
}

/* Call segment_fn() for each up-to-SPINSZ segment between
 * 'start' and 'end'.
 */
void foreach_segment
(ulong* start, ulong* end,
 int me, const void* ctx, segment_fn func) {

    ASSERT(start < end);

    // Confirm 'start' points to an even dword, and 'end'
    // should point to an odd dword
    ASSERT(0   == (((ulong)start) & 0x7));
    ASSERT(0x4 == (((ulong)end)   & 0x7));

    // 'end' may be exactly 0xfffffffc, right at the 4GB boundary.
    //
    // To avoid overflow in our loop tests and length calculations,
    // use dword indices (the '_dw' vars) to avoid overflows.
    ulong start_dw = ((ulong)start) >> 2;
    ulong   end_dw = ((ulong)  end) >> 2;

    // end is always xxxxxffc, but increment end_dw to an
    // address beyond the segment for easier boundary calculations.
    ++end_dw;

    ulong seg_dw     = start_dw;
    ulong seg_end_dw = start_dw;

    int done = 0;
    do {
        do_tick(me);
        { BAILR }

        // ensure no overflow
        ASSERT((seg_end_dw + SPINSZ_DWORDS) > seg_end_dw);
        seg_end_dw += SPINSZ_DWORDS;

        if (seg_end_dw >= end_dw) {
            seg_end_dw = end_dw;
            done++;
        }
        if (seg_dw == seg_end_dw) {
            break;
        }

        ASSERT(((ulong)seg_end_dw) <= 0x40000000);
        ASSERT(seg_end_dw > seg_dw);
        ulong seg_len_dw = seg_end_dw - seg_dw;

        func((ulong*)(seg_dw << 2), seg_len_dw, ctx);

        seg_dw = seg_end_dw;
    } while (!done);
}

/* Calls segment_fn() for each segment in vv->map.
 *
 * Does not slice by CPU number, so it covers the entire memory.
 * Contrast to sliced_foreach_segment().
 */
STATIC void unsliced_foreach_segment
(const void* ctx, int me, segment_fn func) {
    int j;
    for (j=0; j<segs; j++) {
        foreach_segment(vv->map[j].start,
                        vv->map[j].end,
                        me, ctx, func);
    }
}

/* Calls segment_fn() for each segment to be tested by CPU 'me'.
 *
 * In multicore mode, slices the segments by 'me' (the CPU ordinal
 * number) so that each call will cover only 1/Nth of memory.
 */
STATIC void sliced_foreach_segment
(const void *ctx, int me, segment_fn func) {
    int j;
    ulong *start, *end;  // VAs
    ulong* prev_end = 0;
    for (j=0; j<segs; j++) {
        calculate_chunk(&start, &end, me, j, 64);

        // Ensure no overlap among chunks
        ASSERT(end > start);
        if (prev_end > 0) {
            ASSERT(prev_end < start);
        }
        prev_end = end;

        foreach_segment(start, end, me, ctx, func);
    }
}

STATIC void addr_tst1_seg(ulong* restrict buf,
                          ulong len_dw, const void* unused) {
    // Within each segment:
    //  - choose a low dword offset 'off'
    //  - write pat to *off
    //  - write ~pat to addresses that are above off by
    //    1, 2, 4, ... dwords up to the top of the segment. None
    //    should alias to the original dword.
    //  - write ~pat to addresses that are below off by
    //    1, 2, 4, etc dwords, down to the start of the segment. None
    //    should alias to the original dword. If adding a given offset
    //    doesn't produce a single bit address flip (because it produced
    //    a carry) subtracting the same offset should give a single bit flip.
    //  - repeat this, moving off ahead in increments of 1MB;
    //    this covers address bits within physical memory banks, we hope?

    ulong pat;
    int k;

    for (pat=0x5555aaaa, k=0; k<2; k++) {
        hprint(LINE_PAT, COL_PAT, pat);

        for (ulong off_dw = 0; off_dw < len_dw; off_dw += (1 << 18)) {
            buf[off_dw] = pat;
            pat = ~pat;

            for (ulong more_off_dw = 1; off_dw + more_off_dw < len_dw;
                 more_off_dw = more_off_dw << 1) {
                ASSERT(more_off_dw);  // it should never get to zero
                buf[off_dw + more_off_dw] = pat;
                ulong bad;
                if ((bad = buf[off_dw]) != ~pat) {
                    ad_err1(buf + off_dw,
                            buf + off_dw + more_off_dw,
                            bad, ~pat);
                    break;
                }
            }
            for (ulong more_off_dw = 1; off_dw > more_off_dw;
                 more_off_dw = more_off_dw << 1) {
                ASSERT(more_off_dw);  // it should never get to zero
                buf[off_dw - more_off_dw] = pat;
                ulong bad;
                if ((bad = buf[off_dw]) != ~pat) {
                    ad_err1(buf + off_dw,
                            buf + off_dw - more_off_dw,
                            bad, ~pat);
                    break;
                }
            }
        }
    }
}

/*
 * Memory address test, walking ones
 */
void addr_tst1(int me)
{
    unsliced_foreach_segment(nullptr, me, addr_tst1_seg);
}

STATIC void addr_tst2_init_segment(ulong* p,
                                   ulong len_dw, const void* unused) {
    ulong* pe = p + (len_dw - 1);

    /* Original C code replaced with hand tuned assembly code
     *			for (; p <= pe; p++) {
     *				*p = (ulong)p;
     *			}
     */
    asm __volatile__ (
                      "jmp L91\n\t"
                      ".p2align 4,,7\n\t"
                      "L90:\n\t"
                      "addl $4,%%edi\n\t"
                      "L91:\n\t"
                      "movl %%edi,(%%edi)\n\t"
                      "cmpl %%edx,%%edi\n\t"
                      "jb L90\n\t"
                      : : "D" (p), "d" (pe)
                      );
}

STATIC void addr_tst2_check_segment(ulong* p,
                                    ulong len_dw, const void* unused) {
    ulong* pe = p + (len_dw - 1);

    /* Original C code replaced with hand tuned assembly code
     *			for (; p <= pe; p++) {
     *				if((bad = *p) != (ulong)p) {
     *					ad_err2((ulong)p, bad);
     *				}
     *			}
     */
    asm __volatile__
        (
         "jmp L95\n\t"
         ".p2align 4,,7\n\t"
         "L99:\n\t"
         "addl $4,%%edi\n\t"
         "L95:\n\t"
         "movl (%%edi),%%ecx\n\t"
         "cmpl %%edi,%%ecx\n\t"
         "jne L97\n\t"
         "L96:\n\t"
         "cmpl %%edx,%%edi\n\t"
         "jb L99\n\t"
         "jmp L98\n\t"

         "L97:\n\t"
         "pushl %%edx\n\t"
         "pushl %%ecx\n\t"
         "pushl %%edi\n\t"
         "call ad_err2\n\t"
         "popl %%edi\n\t"
         "popl %%ecx\n\t"
         "popl %%edx\n\t"
         "jmp L96\n\t"

         "L98:\n\t"
         : : "D" (p), "d" (pe)
         : "ecx"
         );
}

/*
 * Memory address test, own address
 */
void addr_tst2(int me)
{
    cprint(LINE_PAT, COL_PAT, "address ");

    /* Write each address with its own address */
    unsliced_foreach_segment(nullptr, me, addr_tst2_init_segment);
    { BAILR }

    /* Each address should have its own address */
    unsliced_foreach_segment(nullptr, me, addr_tst2_check_segment);
}

typedef struct {
    int me;
    ulong xorVal;    
} movinvr_ctx;

STATIC void movinvr_init(ulong* p,
                         ulong len_dw, const void* vctx) {
    ulong* pe = p + (len_dw - 1);
    const movinvr_ctx* ctx = (const movinvr_ctx*)vctx;
    /* Original C code replaced with hand tuned assembly code */
    /*
      for (; p <= pe; p++) {
      *p = rand(me);
      }
    */

    asm __volatile__
        (
         "jmp L200\n\t"
         ".p2align 4,,7\n\t"
         "L201:\n\t"
         "addl $4,%%edi\n\t"
         "L200:\n\t"
         "pushl %%ecx\n\t"
         "call rand\n\t"
         "popl %%ecx\n\t"
         "movl %%eax,(%%edi)\n\t"
         "cmpl %%ebx,%%edi\n\t"
         "jb L201\n\t"
         : : "D" (p), "b" (pe), "c" (ctx->me)
         : "eax"
         );
}

STATIC void movinvr_body(ulong* p, ulong len_dw, const void* vctx) {
    ulong* pe = p + (len_dw - 1);
    const movinvr_ctx* ctx = (const movinvr_ctx*)vctx;

    /* Original C code replaced with hand tuned assembly code */
				
    /*for (; p <= pe; p++) {
      num = rand(me);
      if (i) {
      num = ~num;
      }
      if ((bad=*p) != num) {
      mt86_error((ulong*)p, num, bad);
      }
      *p = ~num;
      }*/

    asm __volatile__
        (
         "pushl %%ebp\n\t"

         // Skip first increment
         "jmp L26\n\t"
         ".p2align 4,,7\n\t"

         // increment 4 bytes (32-bits)
         "L27:\n\t"
         "addl $4,%%edi\n\t"

         // Check this byte
         "L26:\n\t"

         // Get next random number, pass in me(edx), random value returned in num(eax)
         // num = rand(me);
         // cdecl call maintains all registers except eax, ecx, and edx
         // We maintain edx with a push and pop here using it also as an input
         // we don't need the current eax value and want it to change to the return value
         // we overwrite ecx shortly after this discarding its current value
         "pushl %%edx\n\t" // Push function inputs onto stack
         "call rand\n\t"
         "popl %%edx\n\t" // Remove function inputs from stack

         // XOR the random number with xorVal(ebx), which is either 0xffffffff or 0 depending on the outer loop
         // if (i) { num = ~num; }
         "xorl %%ebx,%%eax\n\t"

         // Move the current value of the current position p(edi) into bad(ecx)
         // (bad=*p)
         "movl (%%edi),%%ecx\n\t"

         // Compare bad(ecx) to num(eax)
         "cmpl %%eax,%%ecx\n\t"

         // If not equal jump the error case
         "jne L23\n\t"

         // Set a new value or not num(eax) at the current position p(edi)
         // *p = ~num;
         "L25:\n\t"
         "movl $0xffffffff,%%ebp\n\t"
         "xorl %%ebp,%%eax\n\t"
         "movl %%eax,(%%edi)\n\t"

         // Loop until current position p(edi) equals the end position pe(esi)
         "cmpl %%esi,%%edi\n\t"
         "jb L27\n\t"
         "jmp L24\n"

         // Error case
         "L23:\n\t"
         // Must manually maintain eax, ecx, and edx as part of cdecl call convention
         "pushl %%edx\n\t"
         "pushl %%ecx\n\t" // Next three pushes are functions input
         "pushl %%eax\n\t"
         "pushl %%edi\n\t"
         "call mt86_error\n\t"
         "popl %%edi\n\t" // Remove function inputs from stack and restore register values
         "popl %%eax\n\t"
         "popl %%ecx\n\t"
         "popl %%edx\n\t"
         "jmp L25\n" 

         "L24:\n\t"
         "popl %%ebp\n\t"
         :: "D" (p), "S" (pe), "b" (ctx->xorVal),
          "d" (ctx->me)
         : "eax", "ecx"
         );
}

/*
 * Test all of memory using a "half moving inversions" algorithm using random
 * numbers and their complement as the data pattern. Since we are not able to
 * produce random numbers in reverse order testing is only done in the forward
 * direction.
 */
void movinvr(int me)
{
    int i, seed1, seed2;

    movinvr_ctx ctx;
    ctx.me = me;
    ctx.xorVal = 0;

    /* Initialize memory with initial sequence of random numbers.  */
    if (cpu_id.fid.bits.rdtsc) {
        asm __volatile__ ("rdtsc":"=a" (seed1),"=d" (seed2));
    } else {
        seed1 = 521288629 + vv->pass;
        seed2 = 362436069 - vv->pass;
    }

    /* Display the current seed */
    if (mstr_cpu == me) hprint(LINE_PAT, COL_PAT, seed1);
    rand_seed(seed1, seed2, me);

    sliced_foreach_segment(&ctx, me, movinvr_init);
    { BAILR }

    /* Do moving inversions test. Check for initial pattern and then
     * write the complement for each memory location.
     */
    for (i=0; i<2; i++) {
        rand_seed(seed1, seed2, me);

        if (i) {
            ctx.xorVal = 0xffffffff;
        } else {
            ctx.xorVal = 0;
        }

        sliced_foreach_segment(&ctx, me, movinvr_body);
        { BAILR }
    }
}

typedef struct {
    ulong p1;
    ulong p2;
} movinv1_ctx;

STATIC void movinv1_init(ulong* start,
                         ulong len_dw, const void* vctx) {
    const movinv1_ctx* ctx = (const movinv1_ctx*)vctx;

    ulong p1 = ctx->p1;
    ulong* p = start;

    asm __volatile__
        (
         "rep\n\t"
         "stosl\n\t"
         : : "c" (len_dw), "D" (p), "a" (p1)
         );
}

STATIC void movinv1_bottom_up(ulong* start,
                              ulong len_dw, const void* vctx) {
    const movinv1_ctx* ctx = (const movinv1_ctx*)vctx;
    ulong p1 = ctx->p1;
    ulong p2 = ctx->p2;
    ulong* p = start;
    ulong* pe = p + (len_dw - 1);

    // Original C code replaced with hand tuned assembly code 
    // seems broken
    /*for (; p <= pe; p++) {
      if ((bad=*p) != p1) {
      mt86_error((ulong*)p, p1, bad);
      }
      *p = p2;
      }*/

    asm __volatile__
        (
         "jmp L2\n\t"
         ".p2align 4,,7\n\t"
         "L0:\n\t"
         "addl $4,%%edi\n\t"
         "L2:\n\t"
         "movl (%%edi),%%ecx\n\t"
         "cmpl %%eax,%%ecx\n\t"
         "jne L3\n\t"
         "L5:\n\t"
         "movl %%ebx,(%%edi)\n\t"
         "cmpl %%edx,%%edi\n\t"
         "jb L0\n\t"
         "jmp L4\n"

         "L3:\n\t"
         "pushl %%edx\n\t"
         "pushl %%ebx\n\t"
         "pushl %%ecx\n\t"
         "pushl %%eax\n\t"
         "pushl %%edi\n\t"
         "call mt86_error\n\t"
         "popl %%edi\n\t"
         "popl %%eax\n\t"
         "popl %%ecx\n\t"
         "popl %%ebx\n\t"
         "popl %%edx\n\t"
         "jmp L5\n"

         "L4:\n\t"
         :: "a" (p1), "D" (p), "d" (pe), "b" (p2)
         : "ecx"
         );
}

STATIC void movinv1_top_down(ulong* start,
                             ulong len_dw, const void* vctx) {
    const movinv1_ctx* ctx = (const movinv1_ctx*)vctx;
    ulong p1 = ctx->p1;
    ulong p2 = ctx->p2;
    ulong* p = start + (len_dw - 1);
    ulong* pe = start;

    //Original C code replaced with hand tuned assembly code
    // seems broken
    /*do {
      if ((bad=*p) != p2) {
      mt86_error((ulong*)p, p2, bad);
      }
      *p = p1;
      } while (--p >= pe);*/

    asm __volatile__
        (
         "jmp L9\n\t"
         ".p2align 4,,7\n\t"
         "L11:\n\t"
         "subl $4, %%edi\n\t"
         "L9:\n\t"
         "movl (%%edi),%%ecx\n\t"
         "cmpl %%ebx,%%ecx\n\t"
         "jne L6\n\t"
         "L10:\n\t"
         "movl %%eax,(%%edi)\n\t"
         "cmpl %%edi, %%edx\n\t"
         "jne L11\n\t"
         "jmp L7\n\t"

         "L6:\n\t"
         "pushl %%edx\n\t"
         "pushl %%eax\n\t"
         "pushl %%ecx\n\t"
         "pushl %%ebx\n\t"
         "pushl %%edi\n\t"
         "call mt86_error\n\t"
         "popl %%edi\n\t"
         "popl %%ebx\n\t"
         "popl %%ecx\n\t"
         "popl %%eax\n\t"
         "popl %%edx\n\t"
         "jmp L10\n"

         "L7:\n\t"
         :: "a" (p1), "D" (p), "d" (pe), "b" (p2)
         : "ecx"
         );
}

/*
 * Test all of memory using a "moving inversions" algorithm using the
 * pattern in p1 and its complement in p2.
 */
void movinv1 (int iter, ulong p1, ulong p2, int me)
{
    int i;

    /* Display the current pattern */
    if (mstr_cpu == me) hprint(LINE_PAT, COL_PAT, p1);

    movinv1_ctx ctx;
    ctx.p1 = p1;
    ctx.p2 = p2;
    sliced_foreach_segment(&ctx, me, movinv1_init);
    { BAILR }

    /* Do moving inversions test. Check for initial pattern and then
     * write the complement for each memory location. Test from bottom
     * up and then from the top down.  */
    for (i=0; i<iter; i++) {
        sliced_foreach_segment(&ctx, me, movinv1_bottom_up);
        { BAILR }

        // NOTE(jcoiner):
        // For the top-down pass, the original 5.01 code iterated over
        // 'segs' in from n-1 down to 0, and then within each mapped segment,
        // it would form the SPINSZ windows from the top down -- thus forming
        // a different set of windows than the bottom-up pass, when the segment
        // is not an integer number of windows.
        //
        // My guess is that this buys us very little additional coverage, that
        // the value in going top-down happens at the word or cache-line level
        // and that there's little to be gained from reversing the direction of
        // the outer loops. So I'm leaving a 'direction' bit off of the
        // foreach_segment() routines for now.
        sliced_foreach_segment(&ctx, me, movinv1_top_down);
        { BAILR }
    }
}

typedef struct {
    ulong p1;
    ulong lb;
    ulong hb;
    int sval;
    int off;
} movinv32_ctx;

STATIC void movinv32_init(ulong* restrict buf,
                          ulong len_dw, const void* vctx) {
    const movinv32_ctx* restrict ctx = (const movinv32_ctx*)vctx;

    ulong* p = buf;
    ulong* pe = buf + (len_dw - 1);

    int k = ctx->off;
    ulong pat = ctx->p1;
    ulong lb = ctx->lb;
    int sval = ctx->sval;

    /* Original C code replaced with hand tuned assembly code
     *			while (p <= pe) {
     *				*p = pat;
     *				if (++k >= 32) {
     *					pat = lb;
     *					k = 0;
     *				} else {
     *					pat = pat << 1;
     *					pat |= sval;
     *				}
     *				p++;
     *			}
     */
    asm __volatile__
        (
         "jmp L20\n\t"
         ".p2align 4,,7\n\t"
         "L923:\n\t"
         "addl $4,%%edi\n\t"
         "L20:\n\t"
         "movl %%ecx,(%%edi)\n\t"
         "addl $1,%%ebx\n\t"
         "cmpl $32,%%ebx\n\t"
         "jne L21\n\t"
         "movl %%esi,%%ecx\n\t"
         "xorl %%ebx,%%ebx\n\t"
         "jmp L22\n"
         "L21:\n\t"
         "shll $1,%%ecx\n\t"
         "orl %%eax,%%ecx\n\t"
         "L22:\n\t"
         "cmpl %%edx,%%edi\n\t"
         "jb L923\n\t"
         :: "D" (p),"d" (pe),"b" (k),"c" (pat),
           "a" (sval), "S" (lb)
         );
}

STATIC void movinv32_bottom_up(ulong* restrict buf, ulong len_dw,
                               const void* vctx) {
    const movinv32_ctx* restrict ctx = (const movinv32_ctx*)vctx;

    ulong* p = buf;
    ulong* pe = buf + (len_dw - 1);

    int k = ctx->off;
    ulong pat = ctx->p1;
    ulong lb = ctx->lb;
    int sval = ctx->sval;

    /* Original C code replaced with hand tuned assembly code
     *				while (1) {
     *					if ((bad=*p) != pat) {
     *						mt86_error((ulong*)p, pat, bad);
     *					}
     *					*p = ~pat;
     *					if (p >= pe) break;
     *					p++;
     *
     *					if (++k >= 32) {
     *						pat = lb;
     *						k = 0;
     *					} else {
     *						pat = pat << 1;
     *						pat |= sval;
     *					}
     *				}
     */
    asm __volatile__
        (
         "pushl %%ebp\n\t"
         "jmp L30\n\t"
         ".p2align 4,,7\n\t"
         "L930:\n\t"
         "addl $4,%%edi\n\t"
         "L30:\n\t"
         "movl (%%edi),%%ebp\n\t"
         "cmpl %%ecx,%%ebp\n\t"
         "jne L34\n\t"

         "L35:\n\t"
         "notl %%ecx\n\t"
         "movl %%ecx,(%%edi)\n\t"
         "notl %%ecx\n\t"
         "incl %%ebx\n\t"
         "cmpl $32,%%ebx\n\t"
         "jne L31\n\t"
         "movl %%esi,%%ecx\n\t"
         "xorl %%ebx,%%ebx\n\t"
         "jmp L32\n"
         "L31:\n\t"
         "shll $1,%%ecx\n\t"
         "orl %%eax,%%ecx\n\t"
         "L32:\n\t"
         "cmpl %%edx,%%edi\n\t"
         "jb L930\n\t"
         "jmp L33\n\t"

         "L34:\n\t"
         "pushl %%esi\n\t"
         "pushl %%eax\n\t"
         "pushl %%ebx\n\t"
         "pushl %%edx\n\t"
         "pushl %%ebp\n\t"
         "pushl %%ecx\n\t"
         "pushl %%edi\n\t"
         "call mt86_error\n\t"
         "popl %%edi\n\t"
         "popl %%ecx\n\t"
         "popl %%ebp\n\t"
         "popl %%edx\n\t"
         "popl %%ebx\n\t"
         "popl %%eax\n\t"
         "popl %%esi\n\t"
         "jmp L35\n"

         "L33:\n\t"
         "popl %%ebp\n\t"
         : "=b" (k),"=c" (pat)
         : "D" (p),"d" (pe),"b" (k),"c" (pat),
           "a" (sval), "S" (lb)
         );
}

STATIC void movinv32_top_down(ulong* restrict buf,
                              ulong len_dw, const void* vctx) {
    const movinv32_ctx* restrict ctx = (const movinv32_ctx*)vctx;

    ulong* pe = buf;
    ulong* p = buf + (len_dw - 1);

    int k = ctx->off;
    ulong pat = ctx->p1;
    ulong hb = ctx->hb;
    int sval = ctx->sval;
    ulong p3 = (ulong)sval << 31;

    // Advance 'k' and 'pat' to where they would have been
    // at the end of the corresponding bottom_up segment.
    //
    // The '-1' is because we didn't advance 'k' or 'pat'
    // on the final bottom_up loop, so they're off by one...
    ulong mod_len = (len_dw - 1) % 32;
    for (int i = 0; i < mod_len; i++) {
        if (++k >= 32) {
            pat = ctx->lb;
            k = 0;
        } else {
            pat = pat << 1;
            pat |= sval;
        }
    }

    // Increment 'k' only because the code below has an off-by-one
    // interpretation of 'k' relative to the bottom_up routine.
    // There it ranges from 0:31, and here it ranges from 1:32.
    k++;

    /* Original C code replaced with hand tuned assembly code */
#if PREFER_C
    ulong bad;
    while(1) {
        if ((bad=*p) != ~pat) {
            mt86_error((ulong*)p, ~pat, bad);
        }
        *p = pat;
        if (p <= pe) break;
        p--;

        if (--k <= 0) {
            k = 32;
            pat = hb;
        } else {
            pat = pat >> 1;
            pat |= p3;
        }
    };
#else
    asm __volatile__
        (
         "pushl %%ebp\n\t"
         "jmp L40\n\t"
         ".p2align 4,,7\n\t"
         "L49:\n\t"
         "subl $4,%%edi\n\t"
         "L40:\n\t"
         "movl (%%edi),%%ebp\n\t"
         "notl %%ecx\n\t"
         "cmpl %%ecx,%%ebp\n\t"
         "jne L44\n\t"

         "L45:\n\t"
         "notl %%ecx\n\t"
         "movl %%ecx,(%%edi)\n\t"
         "decl %%ebx\n\t"
         "cmpl $0,%%ebx\n\t"
         "jg L41\n\t"
         "movl %%esi,%%ecx\n\t"
         "movl $32,%%ebx\n\t"
         "jmp L42\n"
         "L41:\n\t"
         "shrl $1,%%ecx\n\t"
         "orl %%eax,%%ecx\n\t"
         "L42:\n\t"
         "cmpl %%edx,%%edi\n\t"
         "ja L49\n\t"
         "jmp L43\n\t"

         "L44:\n\t"
         "pushl %%esi\n\t"
         "pushl %%eax\n\t"
         "pushl %%ebx\n\t"
         "pushl %%edx\n\t"
         "pushl %%ebp\n\t"
         "pushl %%ecx\n\t"
         "pushl %%edi\n\t"
         "call mt86_error\n\t"
         "popl %%edi\n\t"
         "popl %%ecx\n\t"
         "popl %%ebp\n\t"
         "popl %%edx\n\t"
         "popl %%ebx\n\t"
         "popl %%eax\n\t"
         "popl %%esi\n\t"
         "jmp L45\n"

         "L43:\n\t"
         "popl %%ebp\n\t"
         : : "D" (p),"d" (pe),"b" (k),"c" (pat),
           "a" (p3), "S" (hb)
         );
#endif
}

void movinv32(int iter, ulong p1, ulong lb, ulong hb, int sval, int off,int me)
{
    // First callsite:
    //  - p1 has 1 bit set (somewhere)
    //  - lb = 1 ("low bit")
    //  - hb = 0x80000000 ("high bit")
    //  - sval = 0
    //  - 'off' indicates the position of the set bit in p1
    //
    // Second callsite is the same, but inverted:
    //  - p1 has 1 bit clear (somewhere)
    //  - lb = 0xfffffffe
    //  - hb = 0x7fffffff
    //  - sval = 1
    //  - 'off' indicates the position of the cleared bit in p1

    movinv32_ctx ctx;
    ctx.p1 = p1;
    ctx.lb = lb;
    ctx.hb = hb;
    ctx.sval = sval;
    ctx.off = off;

    /* Display the current pattern */
    if (mstr_cpu == me) hprint(LINE_PAT, COL_PAT, p1);

    sliced_foreach_segment(&ctx, me, movinv32_init);
    { BAILR }

    /* Do moving inversions test. Check for initial pattern and then
     * write the complement for each memory location. Test from bottom
     * up and then from the top down.  */
    for (int i=0; i<iter; i++) {
        sliced_foreach_segment(&ctx, me, movinv32_bottom_up);
        { BAILR }

        sliced_foreach_segment(&ctx, me, movinv32_top_down);
        { BAILR }
    }
}

typedef struct {
    int offset;
    ulong p1;
    ulong p2;
} modtst_ctx;

STATIC void modtst_sparse_writes(ulong* restrict start,
                                 ulong len_dw, const void* vctx) {
    const modtst_ctx* restrict ctx = (const modtst_ctx*)vctx;
    ulong p1 = ctx->p1;
    ulong offset = ctx->offset;

#if PREFER_C
    for (ulong i = offset; i < len_dw; i += MOD_SZ) {
        start[i] = p1;
    }
#else
    ulong* p = start + offset;
    ulong* pe = start + len_dw;
    asm __volatile__
        (
         "jmp L60\n\t"
         ".p2align 4,,7\n\t"

         "L60:\n\t"
         "movl %%eax,(%%edi)\n\t"
         "addl $80,%%edi\n\t"
         "cmpl %%edx,%%edi\n\t"
         "jb L60\n\t"
         :: "D" (p), "d" (pe), "a" (p1)
         );
#endif
}

STATIC void modtst_dense_writes(ulong* restrict start, ulong len_dw,
                                const void* vctx) {
    const modtst_ctx* restrict ctx = (const modtst_ctx*)vctx;
    ulong p2 = ctx->p2;
    ulong offset = ctx->offset;

    ASSERT(offset < MOD_SZ);

    ulong k = 0;
#if PREFER_C
    for (ulong i = 0; i < len_dw; i++) {
        if (k != offset) {
            start[i] = p2;
        }
        if (++k >= MOD_SZ) {
            k = 0;
        }
    }
#else
    ulong* pe = start + (len_dw - 1);
    asm __volatile__
        (
         "jmp L50\n\t"
         ".p2align 4,,7\n\t"

         "L54:\n\t"
         "addl $4,%%edi\n\t"
         "L50:\n\t"
         "cmpl %%ebx,%%ecx\n\t"
         "je L52\n\t"
         "movl %%eax,(%%edi)\n\t"
         "L52:\n\t"
         "incl %%ebx\n\t"
         "cmpl $19,%%ebx\n\t"
         "jle L53\n\t"
         "xorl %%ebx,%%ebx\n\t"
         "L53:\n\t"
         "cmpl %%edx,%%edi\n\t"
         "jb L54\n\t"
         : : "D" (start), "d" (pe), "a" (p2),
           "b" (k), "c" (offset)
         );
#endif
}

STATIC void modtst_check(ulong* restrict start,
                         ulong len_dw, const void* vctx) {
    const modtst_ctx* restrict ctx = (const modtst_ctx*)vctx;
    ulong p1 = ctx->p1;
    ulong offset = ctx->offset;

    ASSERT(offset < MOD_SZ);

#if PREFER_C
    ulong bad;
    for (ulong i = offset; i < len_dw; i += MOD_SZ) {
        if ((bad = start[i]) != p1)
            mt86_error(start + i, p1, bad);
    }
#else
    ulong* p = start + offset;
    ulong* pe = start + len_dw;
    asm __volatile__
        (
         "jmp L70\n\t"
         ".p2align 4,,7\n\t"

         "L70:\n\t"
         "movl (%%edi),%%ecx\n\t"
         "cmpl %%eax,%%ecx\n\t"
         "jne L71\n\t"
         "L72:\n\t"
         "addl $80,%%edi\n\t"
         "cmpl %%edx,%%edi\n\t"
         "jb L70\n\t"
         "jmp L73\n\t"

         "L71:\n\t"
         "pushl %%edx\n\t"
         "pushl %%ecx\n\t"
         "pushl %%eax\n\t"
         "pushl %%edi\n\t"
         "call mt86_error\n\t"
         "popl %%edi\n\t"
         "popl %%eax\n\t"
         "popl %%ecx\n\t"
         "popl %%edx\n\t"
         "jmp L72\n"

         "L73:\n\t"
         : : "D" (p), "d" (pe), "a" (p1)
         : "ecx"
         );
#endif
}

/*
 * Test all of memory using modulo X access pattern.
 */
void modtst(int offset, int iter, ulong p1, ulong p2, int me)
{
    modtst_ctx ctx;
    ctx.offset = offset;
    ctx.p1 = p1;
    ctx.p2 = p2;

    /* Display the current pattern */
    if (mstr_cpu == me) {
        hprint(LINE_PAT, COL_PAT-2, p1);
        cprint(LINE_PAT, COL_PAT+6, "-");
        dprint(LINE_PAT, COL_PAT+7, offset, 2, 1);
    }

    /* Write every nth location with pattern */
    sliced_foreach_segment(&ctx, me, modtst_sparse_writes);
    { BAILR }

    /* Write the rest of memory "iter" times with the pattern complement */
    for (ulong i=0; i<iter; i++) {
        sliced_foreach_segment(&ctx, me, modtst_dense_writes);
        { BAILR }
    }

    /* Now check every nth location */
    sliced_foreach_segment(&ctx, me, modtst_check);
}

#if PREFER_C

STATIC void movsl(ulong* dest,
           ulong* src,
           ulong size_in_dwords) {
    /* Logically equivalent to:

    for (ulong i = 0; i < size_in_dwords; i++)
        dest[i] = src[i];

    However: the movsl instruction does the entire loop
    in one instruction -- this is probably how 'memcpy'
    is implemented -- so hardware makes it very fast.

    Even in PREFER_C mode, we want the brute force of movsl!
    */
    asm __volatile__
        (
         "cld\n"
         "jmp L1189\n\t"

         ".p2align 4,,7\n\t"
         "L1189:\n\t"

         "movl %1,%%edi\n\t" // dest
         "movl %0,%%esi\n\t" // src
         "movl %2,%%ecx\n\t" // len in dwords
         "rep\n\t"
         "movsl\n\t"

         :: "g" (src), "g" (dest), "g" (size_in_dwords)
         : "edi", "esi", "ecx"
         );
}
#endif  // PREFER_C

STATIC ulong block_move_normalize_len_dw(ulong len_dw) {
    // The block_move test works with sets of 64-byte blocks,
    // so ensure our total length is a multiple of 64.
    //
    // In fact, since we divide the region in half, and each half-region
    // is a set of 64-byte blocks, the full region should be a multiple of 128
    // bytes.
    //
    // Note that there's no requirement for the start address of the region to
    // be 64-byte aligned, it can be any dword.
    ulong result = (len_dw >> 5) << 5;
    ASSERT(result > 0);
    return result;
}

STATIC void block_move_init(ulong* restrict buf,
                            ulong len_dw, const void* unused_ctx) {
    len_dw = block_move_normalize_len_dw(len_dw);

    // Compute 'len' in units of 64-byte chunks:
    ulong len = len_dw >> 4;

    // We only need to initialize len/2, since we'll just copy
    // the first half onto the second half in the move step.
    len = len >> 1;

    ulong base_val = 1;
#if PREFER_C
    while(len > 0) {
        ulong neg_val = ~base_val;

        // Set a block of 64 bytes   //   first block DWORDS are:
        buf[0] = base_val;             //   0x00000001
        buf[1] = base_val;             //   0x00000001
        buf[2] = base_val;             //   0x00000001
        buf[3] = base_val;             //   0x00000001
        buf[4] = neg_val;              //   0xfffffffe
        buf[5] = neg_val;              //   0xfffffffe
        buf[6] = base_val;             //   0x00000001
        buf[7] = base_val;             //   0x00000001
        buf[8] = base_val;             //   0x00000001
        buf[9] = base_val;             //   0x00000001
        buf[10] = neg_val;             //   0xfffffffe
        buf[11] = neg_val;             //   0xfffffffe
        buf[12] = base_val;            //   0x00000001
        buf[13] = base_val;            //   0x00000001
        buf[14] = neg_val;             //   0xfffffffe
        buf[15] = neg_val;             //   0xfffffffe

        buf += 16;  // advance to next 64-byte block
        len--;

        // Rotate the bit left, including an all-zero state.
        // It can't hurt to have a periodicity of 33 instead of
        // a power of two.
        if (base_val == 0) {
            base_val = 1;
        } else if (base_val & 0x80000000) {
            base_val = 0;
        } else {
            base_val = base_val << 1;
        }
    }
#else
    asm __volatile__
        (
         "jmp L100\n\t"

         ".p2align 4,,7\n\t"
         "L100:\n\t"

         // First loop eax is 0x00000001, edx is 0xfffffffe
         "movl %%eax, %%edx\n\t"
         "notl %%edx\n\t"

         // Set a block of 64-bytes	// First loop DWORDS are 
         "movl %%eax,0(%%edi)\n\t"	// 0x00000001
         "movl %%eax,4(%%edi)\n\t"	// 0x00000001
         "movl %%eax,8(%%edi)\n\t"	// 0x00000001
         "movl %%eax,12(%%edi)\n\t"	// 0x00000001
         "movl %%edx,16(%%edi)\n\t"	// 0xfffffffe
         "movl %%edx,20(%%edi)\n\t"	// 0xfffffffe
         "movl %%eax,24(%%edi)\n\t"	// 0x00000001
         "movl %%eax,28(%%edi)\n\t"	// 0x00000001
         "movl %%eax,32(%%edi)\n\t"	// 0x00000001
         "movl %%eax,36(%%edi)\n\t"	// 0x00000001
         "movl %%edx,40(%%edi)\n\t"	// 0xfffffffe
         "movl %%edx,44(%%edi)\n\t"	// 0xfffffffe
         "movl %%eax,48(%%edi)\n\t"	// 0x00000001
         "movl %%eax,52(%%edi)\n\t"	// 0x00000001
         "movl %%edx,56(%%edi)\n\t"	// 0xfffffffe
         "movl %%edx,60(%%edi)\n\t"	// 0xfffffffe

         // rotate left with carry, 
         // second loop eax is		 0x00000002
         // second loop edx is (~eax) 0xfffffffd
         "rcll $1, %%eax\n\t"		
			
         // Move current position forward 64-bytes (to start of next block)
         "leal 64(%%edi), %%edi\n\t"

         // Loop until end
         "decl %%ecx\n\t"
         "jnz  L100\n\t"

         : : "D" (buf), "c" (len), "a" (base_val)
         : "edx"
         );
#endif
}

typedef struct {
    int iter;
    int me;
} block_move_ctx;

STATIC void block_move_move(ulong* restrict buf,
                            ulong len_dw, const void* vctx) {
    const block_move_ctx* restrict ctx = (const block_move_ctx*)vctx;
    ulong iter = ctx->iter;
    int me = ctx->me;

    len_dw = block_move_normalize_len_dw(len_dw);

    /* Now move the data around 
     * First move the data up half of the segment size we are testing
     * Then move the data to the original location + 32 bytes
     */
    ulong half_len_dw = len_dw / 2; // Half the size of this block in DWORDS
    ASSERT(half_len_dw > 8);

    ulong* mid = buf + half_len_dw;    // VA at mid-point of this block.
    for (int i=0; i<iter; i++) {
        if (i > 0) {
            // foreach_segment() called this before the 0th iteration,
            // so don't tick twice in quick succession.
            do_tick(me);
        }
        { BAILR }

#if PREFER_C
        // Move first half to 2nd half:
        movsl(/*dest=*/ mid, /*src=*/ buf, half_len_dw);

        // Move the second half, less the last 8 dwords
        // to the first half plus an offset of 8 dwords.
        movsl(/*dest=*/ buf + 8, /*src=*/ mid, half_len_dw - 8);

        // Finally, move the last 8 dwords of the 2nd half
        // to the first 8 dwords of the first half.
        movsl(/*dest=*/ mid + half_len_dw - 8, /*src=*/ buf, 8);
#else
        asm __volatile__
            (
             "cld\n"
             "jmp L110\n\t"

             ".p2align 4,,7\n\t"
             "L110:\n\t"

             //
             // At the end of all this 
             // - the second half equals the inital value of the first half
             // - the first half is right shifted 32-bytes (with wrapping)
             //

             // Move first half to second half
             "movl %1,%%edi\n\t" // Destination 'mid' (mid point)
             "movl %0,%%esi\n\t" // Source, 'buf' (start point)
             "movl %2,%%ecx\n\t" // Length, 'half_len_dw' (size of a half in DWORDS)
             "rep\n\t"
             "movsl\n\t"

             // Move the second half, less the last 32-bytes. To the first half, offset plus 32-bytes
             "movl %0,%%edi\n\t"
             "addl $32,%%edi\n\t"   // Destination 'buf' plus 32 bytes
             "movl %1,%%esi\n\t"    // Source, 'mid'
             "movl %2,%%ecx\n\t"
             "subl $8,%%ecx\n\t"    // Length, 'half_len_dw'
             "rep\n\t"
             "movsl\n\t"

             // Move last 8 DWORDS (32-bytes) of the second half to the start of the first half
             "movl %0,%%edi\n\t"    // Destination 'buf'
             // Source, 8 DWORDS from the end of the second half, left over by the last rep/movsl
             "movl $8,%%ecx\n\t"    // Length, 8 DWORDS (32-bytes)
             "rep\n\t"
             "movsl\n\t"

             :: "g" (buf), "g" (mid), "g" (half_len_dw)
             : "edi", "esi", "ecx"
             );
#endif        
    }
}

STATIC void block_move_check(ulong* restrict buf,
                             ulong len_dw, const void* unused_ctx) {
    len_dw = block_move_normalize_len_dw(len_dw);

    /* Now check the data.
     * This is rather crude, we just check that the
     * adjacent words are the same.
     */
#if PREFER_C
    for (ulong i = 0; i < len_dw; i = i + 2) {
        if (buf[i] != buf[i+1]) {
            mt86_error(buf+i, buf[i], buf[i+1]);
        }
    }
#else
    ulong* pe = buf + (len_dw - 2);
    asm __volatile__
        (
         "jmp L120\n\t"

         ".p2align 4,,7\n\t"
         "L124:\n\t"
         "addl $8,%%edi\n\t" // Next QWORD
         "L120:\n\t"

         // Compare adjacent DWORDS
         "movl (%%edi),%%ecx\n\t"
         "cmpl 4(%%edi),%%ecx\n\t"
         "jnz L121\n\t" // Print error if they don't match

         // Loop until end of block
         "L122:\n\t"
         "cmpl %%edx,%%edi\n\t"
         "jb L124\n"
         "jmp L123\n\t"

         "L121:\n\t"
         // eax not used so we don't need to save it as per cdecl
         // ecx is used but not restored, however we don't need it's value anymore after this point
         "pushl %%edx\n\t"
         "pushl 4(%%edi)\n\t"
         "pushl %%ecx\n\t"
         "pushl %%edi\n\t"
         "call mt86_error\n\t"
         "popl %%edi\n\t"
         "addl $8,%%esp\n\t"
         "popl %%edx\n\t"
         "jmp L122\n"
         "L123:\n\t"
         :: "D" (buf), "d" (pe)
         : "ecx"
         );
#endif
}

/*
 * Test memory using block moves 
 * Adapted from Robert Redelmeier's burnBX test
 */
void block_move(int iter, int me)
{
    cprint(LINE_PAT, COL_PAT-2, "          ");

    block_move_ctx ctx;
    ctx.iter = iter;
    ctx.me = me;

    /* Initialize memory with the initial pattern.  */
    sliced_foreach_segment(&ctx, me, block_move_init);
    { BAILR }
    s_barrier();

    /* Now move the data around */
    sliced_foreach_segment(&ctx, me, block_move_move);
    { BAILR }
    s_barrier();

    /* And check it. */
    sliced_foreach_segment(&ctx, me, block_move_check);
}

typedef struct {
    ulong pat;
} bit_fade_ctx;

STATIC void bit_fade_fill_seg(ulong* restrict p,
                              ulong len_dw, const void* vctx) {
    const bit_fade_ctx* restrict ctx = (const bit_fade_ctx*)vctx;
    ulong pat = ctx->pat;

    for (ulong i = 0; i < len_dw; i++) {
        p[i] = pat;
    }
}

/*
 * Test memory for bit fade, fill memory with pattern.
 */
void bit_fade_fill(ulong p1, int me)
{
    /* Display the current pattern */
    hprint(LINE_PAT, COL_PAT, p1);

    /* Initialize memory with the initial pattern.  */
    bit_fade_ctx ctx;
    ctx.pat = p1;
    unsliced_foreach_segment(&ctx, me, bit_fade_fill_seg);
}

STATIC void bit_fade_chk_seg(ulong* restrict p,
                             ulong len_dw, const void* vctx) {
    const bit_fade_ctx* restrict ctx = (const bit_fade_ctx*)vctx;
    ulong pat = ctx->pat;

    for (ulong i = 0; i < len_dw; i++) {
        ulong bad;
        if ((bad=p[i]) != pat) {
            mt86_error(p+i, pat, bad);
        }
    }
}

void bit_fade_chk(ulong p1, int me)
{
    bit_fade_ctx ctx;
    ctx.pat = p1;

    /* Make sure that nothing changed while sleeping */
    unsliced_foreach_segment(&ctx, me, bit_fade_chk_seg);
}

/* Sleep for N seconds */
void sleep(long n, int flag, int me,
           int sms /* interpret 'n' as milliseconds instead */)
{
    ulong sh, sl, l, h, t, ip=0;

    /* save the starting time */
    asm __volatile__(
                     "rdtsc":"=a" (sl),"=d" (sh));

    /* loop for n seconds */
    while (1) {
        asm __volatile__(
                         "rep ; nop\n\t"
                         "rdtsc":"=a" (l),"=d" (h));
        asm __volatile__ (
                          "subl %2,%0\n\t"
                          "sbbl %3,%1"
                          :"=a" (l), "=d" (h)
                          :"g" (sl), "g" (sh),
                           "0" (l), "1" (h));

        if (sms != 0) {
            t = h * ((unsigned)0xffffffff / vv->clks_msec);
            t += (l / vv->clks_msec);
        } else {
            t = h * ((unsigned)0xffffffff / vv->clks_msec) / 1000;
            t += (l / vv->clks_msec) / 1000;
        }

        /* Is the time up? */
        if (t >= n) {
            break;
        }

        /* Only display elapsed time if flag is set */
        if (flag == 0) {
            continue;
        }

        if (t != ip) {
            do_tick(me);
            { BAILR }
            ip = t;
        }
    }
}

void beep(unsigned int frequency)
{
#if 0
    // BOZO(jcoiner)
    // Removed this, we need to define outb_p() and inb_p()
    // before reintroducing it.
#else
    unsigned int count = 1193180 / frequency;

    // Switch on the speaker
    outb_p(inb_p(0x61)|3, 0x61);

    // Set command for counter 2, 2 byte write
    outb_p(0xB6, 0x43);

    // Select desired Hz
    outb_p(count & 0xff, 0x42);
    outb((count >> 8) & 0xff, 0x42);

    // Block for 100 microseconds
    sleep(100, 0, 0, 1);

    // Switch off the speaker
    outb(inb_p(0x61)&0xFC, 0x61);
#endif
}
