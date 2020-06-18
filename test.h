/* test.h - MemTest-86  Version 3.4
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady
 */

#ifndef _TEST_H_
#define _TEST_H_
#define E88     0x00
#define E801    0x04
#define E820NR  0x08           /* # entries in E820MAP */
#define E820MAP 0x0c           /* our map */
#define E820MAX 127            /* number of entries in E820MAP */
#define E820ENTRY_SIZE 20
#define MEMINFO_SIZE (E820MAP + E820MAX * E820ENTRY_SIZE)

#ifndef __ASSEMBLY__

#define E820_RAM        1
#define E820_RESERVED   2
#define E820_ACPI       3 /* usable as RAM once ACPI tables have been read */
#define E820_NVS        4

struct e820entry {
    unsigned long long addr;        /* start of memory segment */
    unsigned long long size;        /* size of memory segment */
    unsigned long type;             /* type of memory segment */
};

struct mem_info_t {
    unsigned long e88_mem_k;         /* 0x00 */
    unsigned long e801_mem_k;        /* 0x04 */
    unsigned long e820_nr;           /* 0x08 */
    struct e820entry e820[E820MAX];  /* 0x0c */
                                    /* 0x28c */
};

typedef unsigned long ulong;
#define STACKSIZE_BYTES (8*1024)
#define MAX_MEM_PAGES   0x7FF00000      /* 8 TB; units are 4K pages */
#define WIN_SZ_PAGES    0x80000         /* 2 GB; units are 4K pages */
#define UNMAP_SZ_PAGES  (0x100000-WIN_SZ_PAGES)  /* Size of unmapped first segment */

#define SPINSZ_DWORDS	0x4000000	/* 256 MB; units are dwords (32-bit words) */
#define MOD_SZ		20
#define BAILOUT		if (bail) return(1);
#define BAILR		if (bail) return;

#define RES_START	0xa0000
#define RES_END		0x100000
#define SCREEN_ADR	0xb8000
#define SCREEN_END_ADR  (SCREEN_ADR + 80*25*2)

#define DMI_SEARCH_START  0x0000F000
#define DMI_SEARCH_LENGTH 0x000F0FFF
#define MAX_DMI_MEMDEVS 16

#define TITLE_WIDTH	 28
#define LINE_TITLE 		0
#define LINE_TST			3
#define LINE_RANGE		4
#define LINE_PAT      5
#define LINE_TIME			5
#define LINE_STATUS		8
#define LINE_INFO			9
#define LINE_HEADER	 12
#define LINE_SCROLL	 14
#define LINE_SPD 		 14
#define LINE_MSG		 22
#define LINE_CPU			7
#define LINE_RAM			8
#define LINE_DMI		 23
#define COL_INF1        15
#define COL_INF2        32
#define COL_INF3        51
#define COL_INF4        70
#define COL_MODE	15
#define COL_MID		30
#define COL_PAT		41
#define BAR_SIZE	(78-COL_MID-9)
#define COL_MSG		23
#define COL_TIME	67
#define COL_SPEC 41

#define POP_W	34
#define POP_H	15
#define POP_X	11
#define POP_Y	8
#define POP2_W  74
#define POP2_H  21
#define POP2_X  3
#define POP2_Y  2

/* CPU mode types */
#define CPM_ALL    1
#define CPM_RROBIN 2
#define CPM_SEQ    3

/* memspeed operations */
#define MS_COPY		1
#define MS_WRITE	2
#define MS_READ		3

#define SZ_MODE_BIOS		1
#define SZ_MODE_PROBE		2

#define getCx86(reg) ({ outb((reg), 0x22); inb(0x23); })

int mt86_memcmp(const void *s1, const void *s2, ulong count);
void *mt86_memmove(void *dest, const void *src, ulong n);
int mt86_strncmp(const char *s1, const char *s2, ulong n);
int mt86_strstr(char *str1, char *str2);
int mt86_strlen(char *string);
int query_linuxbios(void);
int query_pcbios(void);
int insertaddress(ulong);
void printpatn(void);
void printpatn(void);
void itoa(char s[], int n); 
void reverse(char *p);
void serial_console_setup(char *param);
void serial_echo_init(void);
void serial_echo_print(const char *s);
void ttyprint(int y, int x, const char *s);
void ttyprintc(int y, int x, char c);
void cprint(int y,int x, const char *s);
void cplace(int y,int x, const char s);
void hprint(int y,int x, ulong val);
void hprint2(int y,int x, ulong val, int len);
void hprint3(int y,int x, ulong val, int len);
void xprint(int y,int x,ulong val);
void aprint(int y,int x,ulong page);
void dprint(int y,int x,ulong val,int len, int right);
void movinv1(int iter, ulong p1, ulong p2, int cpu);
void movinvr(int cpu);
void movinv32(int iter, ulong p1, ulong lb, ulong mb, int sval, int off,
	int cpu);
void modtst(int off, int iter, ulong p1, ulong p2, int cpu);
#define ASSERT(n) do {                   \
    if (!(n)) {                          \
        assert_fail(__FILE__, __LINE__); \
    } } while(0)
void assert_fail(const char* file, int line_no);
void mt86_error(ulong* adr, ulong good, ulong bad);
void ad_err1(ulong *adr1, ulong *adr2, ulong good, ulong bad);
void ad_err2(ulong *adr, ulong bad);
void do_tick();
void init(void);
struct eregs;
void inter(struct eregs *trap_regs);
void set_cache(int val);
void check_input(void);
void footer(void);
void scroll(void);
void clear_scroll(void);
void popup(void);
void popdown(void);
void popclear(void);
void pop2up(void);
void pop2down(void);
void pop2clear(void);
void get_config(void);
void get_menu(void);
void get_printmode(void);
void addr_tst1(int cpu);
void addr_tst2(int cpu);
int getnum(ulong val);
void sleep(long sec, int flag, int cpu, int sms);
void block_move(int iter, int cpu);
void find_ticks(void);
void print_err(ulong *adr, ulong good, ulong bad, ulong xor);
void print_ecc_err(ulong page, ulong offset, int corrected, 
	unsigned short syndrome, int channel);
void mem_size(void);
void adj_mem(void);
ulong getval(int x, int y, int result_shift);
int get_key(void);
int ascii_to_keycode(int in);
void wait_keyup(void);
void print_hdr(void);
void restart(void);
void parity_err(ulong edi, ulong esi);
void start_config(void);
void clear_screen(void);
void paging_off(void);
void show_spd(void);
int map_page(unsigned long page);
void *mapping(unsigned long phys_page);   // get VA for a physical page
void *emapping(unsigned long phys_page);
int mt86_isdigit(char c);
ulong memspeed(ulong src, ulong len, int iter);
unsigned long page_of(void *ptr);
ulong correct_tsc(ulong el_org);
void bit_fade_fill(unsigned long n, int cpu);
void bit_fade_chk(unsigned long n, int cpu);
void find_ticks_for_pass(void);
void beep(unsigned int frequency);

// Expose foreach_segment here for self_test, otherwise
// it would be local to test.c:
typedef void(*segment_fn)(ulong* start,  // start address
                          ulong len_dw,  // length of segment in dwords
                          const void* ctx);  // any context data needed
void foreach_segment(ulong* start, ulong* end,
                     int me, const void* ctx, segment_fn func);


// In self-test, DEBUGF wraps libc's printf.
// In memtest standalone, printf will be a stub
// and 'vv->debugging' is false to avoid call overhead.
int printf(const char *format, ...);
#define DEBUGF(...) {            \
        if (vv->debugging) {     \
            printf(__VA_ARGS__); \
        }                        \
    }

#define PRINTMODE_SUMMARY   0
#define PRINTMODE_ADDRESSES 1
#define PRINTMODE_PATTERNS  2
#define PRINTMODE_NONE      3

#define BADRAM_MAXPATNS 10

struct pair {
       ulong adr;
       ulong mask;
};

static inline void cache_off(void)
{
    asm(
        "push %eax\n\t"
        "movl %cr0,%eax\n\t"
        "orl $0x40000000,%eax\n\t"  /* Set CD */
        "movl %eax,%cr0\n\t"
        "wbinvd\n\t"
        "pop  %eax\n\t");
}

static inline void cache_on(void)
{
    asm(
        "push %eax\n\t"
        "movl %cr0,%eax\n\t"
        "andl $0x9fffffff,%eax\n\t" /* Clear CD and NW */ 
        "movl %eax,%cr0\n\t"
        "pop  %eax\n\t");
}

struct mmap {
    ulong pbase_addr;
    ulong *start;  // VA of segment start
    ulong *end;    // VA of the last dword within the segment.
};

struct pmap {
    ulong start;   /* phys page number of RAM segment start,
                      in 4K pages. */
    ulong end;  // phys page number (past the end? or inclusive?)
};

struct tseq {
    short sel;     // enabled
    short cpu_sel; // cpu_sel == 0 indicates end of tseq[] array
    short pat;     // the test #
    short iter;    // # of iterations to run
    short errors;  // error count, updated as tests run
    char *msg;
};

struct xadr {
    ulong page;
    ulong offset;
};

struct err_info {
    struct xadr   low_addr;
    struct xadr   high_addr;
    unsigned long ebits;
    long          tbits;
    short         min_bits;
    short         max_bits;
    unsigned long maxl;
    unsigned long eadr;
    unsigned long exor;
    unsigned long cor_err;
    short         hdr_flag;
};



#define X86_FEATURE_PAE		(0*32+ 6) /* Physical Address Extensions */

#define MAX_MEM_SEGMENTS E820MAX

/* Define common variables across relocations of memtest86 */
struct vars {
    int pass;
    int msg_line;
    int ecount;
    int ecc_ecount;
    int msegs;  // number of entries in pmap[]
    int testsel;
    int scroll_start;
    int pass_ticks;
    int total_ticks;
    int pptr;
    int tptr;
    struct err_info erri;
    // PA ranges from e820 table:
    struct pmap pmap[MAX_MEM_SEGMENTS];
    // VA mappings:
    volatile struct mmap map[MAX_MEM_SEGMENTS];
    ulong plim_lower;  // phys page number
    ulong plim_upper;  // phys page number
    ulong clks_msec;
    ulong starth;
    ulong startl;
    ulong snaph;
    ulong snapl;
    int printmode;
    int numpatn;
    struct pair patn [BADRAM_MAXPATNS];
    ulong test_pages;
    ulong selected_pages;
    ulong reserved_pages;
    int check_temp;
    int fail_safe;
    int each_sec;
    int beepmode;
    int debugging;  // Set in selftest only
};

#define FIRMWARE_UNKNOWN   0
#define FIRMWARE_PCBIOS    1
#define FIRMWARE_LINUXBIOS 2

extern struct vars * const vv;
extern unsigned char _start[], _end[], startup_32[];
extern unsigned char _size, _pages;

extern struct mem_info_t mem_info;

#endif /* __ASSEMBLY__ */
#endif /* _TEST_H_ */
