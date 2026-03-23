#include "td_tdx.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
    TD_TDX_PAGE_BYTES = 4096
};

typedef struct {
    uint64_t r10;
    uint64_t r11;
} td_tdcall_args_t;

static td_tdcall_status_t td_tdx_raw_tdcall(td_tdx_runtime_t *runtime, td_tdcall_leaf_t leaf, td_tdcall_args_t *args) {
    (void)leaf;
    (void)args;

    /*
     * Keep the retry-loop contract in one place even when the development
     * environment is not an actual TDX guest.
     */
    if (runtime == NULL || runtime->enabled != TD_TDX_ON) {
        return TD_TDCALL_SUCCESS;
    }

    runtime->emulated = 1;
    return TD_TDCALL_SUCCESS;
}

static int td_tdx_call_with_retry(td_tdx_runtime_t *runtime, td_tdcall_leaf_t leaf, td_tdcall_args_t *args, char *err, size_t err_len) {
    for (;;) {
        td_tdcall_status_t status = td_tdx_raw_tdcall(runtime, leaf, args);

        if (status == TD_TDCALL_SUCCESS) {
            return 0;
        }
        if (status == TD_TDCALL_BUSY || status == TD_TDCALL_INTERRUPTED) {
            continue;
        }

        td_format_error(err, err_len, "tdcall leaf=%u failed with status=%u", (unsigned int)leaf, (unsigned int)status);
        return -1;
    }
}

static int td_tdx_operate_pages(td_tdx_runtime_t *runtime, void *base, size_t bytes, td_tdcall_leaf_t leaf, char *err, size_t err_len) {
    uintptr_t cursor;
    uintptr_t end;

    if (runtime == NULL || runtime->enabled != TD_TDX_ON) {
        return 0;
    }
    if (base == NULL || bytes == 0) {
        return 0;
    }
    if ((((uintptr_t)base) % TD_TDX_PAGE_BYTES) != 0 || (bytes % TD_TDX_PAGE_BYTES) != 0) {
        td_format_error(err, err_len, "tdx shared region must be 4KB aligned");
        return -1;
    }
    if (runtime->emulated) {
        td_tdcall_args_t args;

        memset(&args, 0, sizeof(args));
        args.r10 = (uint64_t)bytes;
        args.r11 = (uint64_t)(uintptr_t)base;
        return td_tdx_call_with_retry(runtime, leaf, &args, err, err_len);
    }

    cursor = (uintptr_t)base;
    end = cursor + bytes;
    while (cursor < end) {
        td_tdcall_args_t args;

        memset(&args, 0, sizeof(args));
        args.r11 = (uint64_t)cursor;
        if (td_tdx_call_with_retry(runtime, leaf, &args, err, err_len) != 0) {
            return -1;
        }
        cursor += TD_TDX_PAGE_BYTES;
    }
    return 0;
}

int td_tdx_runtime_init(td_tdx_runtime_t *runtime, td_mode_t mode, td_tdx_mode_t tdx, char *err, size_t err_len) {
    memset(runtime, 0, sizeof(*runtime));
    runtime->enabled = tdx;
#ifndef TD_TDX_REAL_TDCALL
    runtime->emulated = tdx == TD_TDX_ON ? 1 : 0;
#endif

    if (mode == TD_MODE_CN && tdx == TD_TDX_ON) {
        td_format_error(err, err_len, "cn must run native; set tdx: off");
        return -1;
    }
    if (mode == TD_MODE_MN && tdx != TD_TDX_ON) {
        td_format_error(err, err_len, "mn must run inside TDX; set tdx: on");
        return -1;
    }
    return 0;
}

int td_tdx_accept_shared_memory(td_tdx_runtime_t *runtime, void *base, size_t bytes, char *err, size_t err_len) {
    return td_tdx_operate_pages(runtime, base, bytes, TD_TDCALL_LEAF_PAGE_ACCEPT, err, err_len);
}

int td_tdx_release_shared_memory(td_tdx_runtime_t *runtime, void *base, size_t bytes, char *err, size_t err_len) {
    return td_tdx_operate_pages(runtime, base, bytes, TD_TDCALL_LEAF_PAGE_RELEASE, err, err_len);
}

const char *td_tdx_runtime_name(const td_tdx_runtime_t *runtime) {
    if (runtime == NULL || runtime->enabled != TD_TDX_ON) {
        return "native";
    }
    return runtime->emulated ? "tdx-emulated" : "tdx";
}
