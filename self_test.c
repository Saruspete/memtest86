/* Usermode self-test.
 *
 * This is a unit test for memtest86+ itself,
 * not a standalone usermode memory tester. Sorry.
 *
 * It covers the main test routines in test.c, allows running them
 * in a debugger or adding printfs as needed.
 *
 * It does not cover:
 *  - SMP functionality. There's no fundamental obstacle
 *    to this, it's just not here yet.
 *  - Every .c file. Some of them do things that will be
 *    difficult to test in user mode (eg set page tables)
 *    without some refactoring.
 */

#ifdef NDEBUG
 #error "Someone disabled asserts, but we want asserts for the self test."
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stdint.h"
#include "cpuid.h"
#include "test.h"

/* Provide alternate versions of the globals */
volatile int run_cpus = 1;
volatile int bail = 0;
volatile int segs = 0;
struct vars variables;
struct vars* const vv = &variables;
// Self-test only supports single CPU (ordinal 0) for now:
volatile int mstr_cpu = 0;

void assert_fail(const char* file, int line_no) {
    printf("Failing assert at %s:%d\n", file, line_no);
    assert(0);
}

void do_tick(int me) {}
void hprint(int y, int x, ulong val) {}
void cprint(int y,int x, const char *s) {}
void dprint(int y,int x,ulong val,int len, int right) {}
void s_barrier() {}   // No SMP support in selftest yet

// Selftest doesn't have error injection yet, and thus, we
// never expect to detect any error. Fail an assert in these
// error-reporting routines:
void ad_err1(ulong *adr1, ulong *adr2, ulong good, ulong bad) {
    assert(0);
}
void ad_err2(ulong *adr, ulong bad) {
    assert(0);
}
void mt86_error(ulong* adr, ulong good, ulong bad) {
    assert(0);
}

typedef struct {
    ulong* va;
    ulong len_dw;
} foreach_chunk;

#define MAX_CHUNK 16

typedef struct {
    foreach_chunk chunks[MAX_CHUNK];
    int index;
} foreach_ctx;

void record_chunks(ulong* va, ulong len_dw, const void* vctx) {
    foreach_ctx* ctx = (foreach_ctx*)vctx;

    ctx->chunks[ctx->index].va = va;
    ctx->chunks[ctx->index].len_dw = len_dw;
    ctx->index++;
}

void foreach_tests() {
    foreach_ctx ctx;
    const int me = 0;

    // mapped segment is 3G to 4G exact
    memset(&ctx, 0, sizeof(ctx));
    foreach_segment((ulong*)0xc0000000,
                    (ulong*)0xfffffffc, me, &ctx, record_chunks);

    assert(ctx.index == 4);
    assert(ctx.chunks[0].va     == (ulong*)0xc0000000);
    assert(ctx.chunks[0].len_dw == SPINSZ_DWORDS);
    assert(ctx.chunks[1].va     == (ulong*)0xd0000000);
    assert(ctx.chunks[1].len_dw == SPINSZ_DWORDS);
    assert(ctx.chunks[2].va     == (ulong*)0xe0000000);
    assert(ctx.chunks[2].len_dw == SPINSZ_DWORDS);
    assert(ctx.chunks[3].va     == (ulong*)0xf0000000);
    assert(ctx.chunks[3].len_dw == SPINSZ_DWORDS);

    // mapped segment is 256 bytes, 128 byte aligned
    memset(&ctx, 0, sizeof(ctx));
    foreach_segment((ulong*)0xc0000080,
                    (ulong*)0xc000017c, me, &ctx, record_chunks);
    assert(ctx.index == 1);
    assert(ctx.chunks[0].va == (ulong*)0xc0000080);
    assert(ctx.chunks[0].len_dw == 64);

    // mapped segment starts a bit below 3.75G
    // and goes up to the 4G boundary
    memset(&ctx, 0, sizeof(ctx));
    foreach_segment((ulong*)0xeffff800,
                    (ulong*)0xfffffffc, me, &ctx, record_chunks);
    assert(ctx.index == 2);
    assert(ctx.chunks[0].va == (ulong*)0xeffff800);
    assert(ctx.chunks[0].len_dw == SPINSZ_DWORDS);
    assert(ctx.chunks[1].va == (ulong*)0xfffff800);
    assert(ctx.chunks[1].len_dw == 0x200);

    // mapped segment is 0 to 32M
    memset(&ctx, 0, sizeof(ctx));
    foreach_segment((ulong*)0x0,
                    (ulong*)0x1fffffc, me, &ctx, record_chunks);
    assert(ctx.index == 1);
    assert(ctx.chunks[0].va == (ulong*)0x0);
    assert(ctx.chunks[0].len_dw == 0x800000);
}

int main() {
    memset(&variables, 0, sizeof(variables));
    vv->debugging = 1;

    get_cpuid();

    // add a non-power-of-2 pad to the size, so things don't line
    // up too nicely. Chose 508 because it's not 512.
    const int kTestSizeDwords = SPINSZ_DWORDS * 2 + 508;

    // Allocate an extra cache line on each end, where we'll
    // write a sentinel pattern to detect overflow or underflow:
    const int kRawBufSizeDwords = kTestSizeDwords + 32;

    segs = 1;
    ulong raw_start = (ulong)malloc(kRawBufSizeDwords * sizeof(ulong));
    ulong raw_end = raw_start + kRawBufSizeDwords * sizeof(ulong);

    // align to a cache line:
    if (raw_start & 0x3f) {
        raw_start &= ~0x3f;
        raw_start += 0x40;
    }
    // align to a cache line:
    if (raw_end & 0x3f) {
        raw_end &= ~0x3f;
    }

    const int kSentinelBytes = 48;
    ulong start = raw_start + kSentinelBytes; // exclude low sentinel
    ulong end   = raw_end   - kSentinelBytes; // exclude high sentinel

    // setup sentinels
    memset((ulong*)raw_start, 'z', kSentinelBytes);
    memset((ulong*)end,       'z', kSentinelBytes);

    vv->map[0].start = (ulong*)start;
    vv->map[0].end = ((ulong*)end) - 1;  // map.end points to xxxxxfc

    const int iter = 2;
    const int me = 0;  // cpu ordinal

    foreach_tests();

    // TEST 0
    addr_tst1(me);

    // TEST 1, 2
    addr_tst2(me);

    // TEST 3, 4, 5, 6
    const ulong pat = 0x112211ee;
    movinv1(iter, pat, ~pat, 0);

    // TEST 7
    block_move(iter, me);

    // TEST 8
    movinv32(iter, 0x2, 0x1, 0x80000000, 0, 1, me);

    // TEST 9
    movinvr(me);

    // TEST 10
    modtst(2, 1, 0x5555aaaa, 0xaaaa5555, me);

    // TEST 11
    bit_fade_fill(0xdeadbeef, me);
    bit_fade_chk(0xdeadbeef, me);


    // Check sentinels, they should not have been overwritten. Do this last.
    for (int j=0; j<kSentinelBytes; j++) {
        assert(((char*)raw_start)[j] == 'z');
        assert(((char*)end)[j]       == 'z');
    }

    printf("All self-tests PASS.\n");

    return 0;
}
