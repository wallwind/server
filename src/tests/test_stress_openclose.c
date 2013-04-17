/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: expandtab:ts=8:sw=4:softtabstop=4:
#ident "Copyright (c) 2007 Tokutek Inc.  All rights reserved."
#ident "$Id: test_stress_openclose.c 44373 2012-06-08 20:33:40Z esmet $"

#include "stress_openclose.h"

int
test_main(int argc, char *const argv[]) {
    struct cli_args args = get_default_args();
    parse_stress_test_args(argc, argv, &args);
    // checkpointing is a part of the ref count, so do it often
    args.env_args.checkpointing_period = 5;
    // very small dbs, so verification scans are short and sweet
    args.num_elements = 1000;
    // it's okay for update to get DB_LOCK_NOTGRANTED, etc.
    args.crash_on_operation_failure = false;

    // just run the stress test, no crashing and recovery test
    stress_openclose_crash_at_end = false;
    stress_test_main(&args);
    return 0;
}