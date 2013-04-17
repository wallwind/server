/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
#ident "Copyright (c) 2007-2012 Tokutek Inc.  All rights reserved."
#include "includes.h"
#include "test.h"

int num_entries;
bool flush_may_occur;
int expected_flushed_key;
bool check_flush;


//
// This test verifies that if partial eviction is expensive and
// does not estimate number of freed bytes to be greater than 0,
// then partial eviction is not called, and normal eviction
// is used. The verification is done ia an assert(false) in 
// pe_callback.
//


static void
flush (CACHEFILE f __attribute__((__unused__)),
       int UU(fd),
       CACHEKEY k  __attribute__((__unused__)),
       void *v     __attribute__((__unused__)),
       void** UU(dd),
       void *e     __attribute__((__unused__)),
       PAIR_ATTR s      __attribute__((__unused__)),
       PAIR_ATTR* new_size      __attribute__((__unused__)),
       bool w      __attribute__((__unused__)),
       bool keep   __attribute__((__unused__)),
       bool c      __attribute__((__unused__)),
        bool UU(is_clone)
       ) {
    /* Do nothing */
    if (check_flush && !keep) {
        if (verbose) { printf("FLUSH: %d write_me %d\n", (int)k.b, w); }
        assert(flush_may_occur);
        assert(!w);
        assert(expected_flushed_key == (int)k.b);
        expected_flushed_key--;
    }
}

static int
fetch (CACHEFILE f        __attribute__((__unused__)),
       int UU(fd),
       CACHEKEY k         __attribute__((__unused__)),
       uint32_t fullhash __attribute__((__unused__)),
       void **value       __attribute__((__unused__)),
       void** UU(dd),
       PAIR_ATTR *sizep        __attribute__((__unused__)),
       int  *dirtyp,
       void *extraargs    __attribute__((__unused__))
       ) {
    *dirtyp = 0;
    *value = NULL;
    *sizep = make_pair_attr(1);
    return 0;
}

static void 
pe_est_callback(
    void* UU(ftnode_pv),
    void* UU(dd), 
    long* bytes_freed_estimate, 
    enum partial_eviction_cost *cost, 
    void* UU(write_extraargs)
    )
{
    *bytes_freed_estimate = 0;
    *cost = PE_EXPENSIVE;
}

static int 
pe_callback (
    void *ftnode_pv __attribute__((__unused__)), 
    PAIR_ATTR bytes_to_free __attribute__((__unused__)), 
    PAIR_ATTR* bytes_freed, 
    void* extraargs __attribute__((__unused__))
    ) 
{
    assert(false);
    *bytes_freed = bytes_to_free;
    return 0;
}


static void
cachetable_test (void) {
    const int test_limit = 4;
    num_entries = 0;
    int r;
    CACHETABLE ct;
    r = toku_create_cachetable(&ct, test_limit, ZERO_LSN, NULL_LOGGER); assert(r == 0);
    char fname1[] = __SRCFILE__ "test1.dat";
    unlink(fname1);
    CACHEFILE f1;
    r = toku_cachetable_openf(&f1, ct, fname1, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO); assert(r == 0);

    void* v1;
    void* v2;
    long s1, s2;
    flush_may_occur = false;
    check_flush = true;
    CACHETABLE_WRITE_CALLBACK wc = def_write_callback(NULL);
    wc.flush_callback = flush;
    wc.pe_est_callback = pe_est_callback;
    wc.pe_callback = pe_callback;
    for (int i = 0; i < 100000; i++) {
      r = toku_cachetable_get_and_pin(f1, make_blocknum(1), 1, &v1, &s1, wc, fetch, def_pf_req_callback, def_pf_callback, true, NULL);
        r = toku_cachetable_unpin(f1, make_blocknum(1), 1, CACHETABLE_CLEAN, make_pair_attr(1));
    }
    for (int i = 0; i < 8; i++) {
      r = toku_cachetable_get_and_pin(f1, make_blocknum(2), 2, &v2, &s2, wc, fetch, def_pf_req_callback, def_pf_callback, true, NULL);
        r = toku_cachetable_unpin(f1, make_blocknum(2), 2, CACHETABLE_CLEAN, make_pair_attr(1));
    }
    for (int i = 0; i < 4; i++) {
      r = toku_cachetable_get_and_pin(f1, make_blocknum(3), 3, &v2, &s2, wc, fetch, def_pf_req_callback, def_pf_callback, true, NULL);
        r = toku_cachetable_unpin(f1, make_blocknum(3), 3, CACHETABLE_CLEAN, make_pair_attr(1));
    }
    for (int i = 0; i < 2; i++) {
      r = toku_cachetable_get_and_pin(f1, make_blocknum(4), 4, &v2, &s2, wc, fetch, def_pf_req_callback, def_pf_callback, true, NULL);
        r = toku_cachetable_unpin(f1, make_blocknum(4), 4, CACHETABLE_CLEAN, make_pair_attr(1));
    }
    flush_may_occur = true;
    expected_flushed_key = 4;
    r = toku_cachetable_put(f1, make_blocknum(5), 5, NULL, make_pair_attr(4), wc);
    flush_may_occur = true;
    expected_flushed_key = 5;
    r = toku_cachetable_unpin(f1, make_blocknum(5), 5, CACHETABLE_CLEAN, make_pair_attr(4));

    check_flush = false;
    r = toku_cachefile_close(&f1, 0, false, ZERO_LSN); assert(r == 0 );
    r = toku_cachetable_close(&ct); assert(r == 0 && ct == 0);
}

int
test_main(int argc, const char *argv[]) {
    default_parse_args(argc, argv);
    cachetable_test();
    return 0;
}