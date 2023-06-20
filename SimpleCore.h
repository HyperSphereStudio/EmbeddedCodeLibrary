#ifndef Simple_CORE_C_H
#define Simple_CORE_C_H

#include "stdint.h"
#include "math.h"

#define CAT0(x) _CAT0(x)
#define _CAT0(x) x
#define _CAT(x, y) x ## y
#define CAT(x, y) _CAT(x, y)
#define _CAT2(x, y, z) x ## y ## z
#define CAT2(x, y, z) _CAT2(x, y, z)
#define WaitFor(x) while(!(x))
#define true 1
#define false 0

//Apply A Mask To Source And Dest that Creates a Copy operation
#define ApplyDestMask(dst, src, mask, dest_delta) ((dst) & ~((mask) >> dest_delta)) | (((src) & (mask)) << dest_delta)
#define ApplyMask(dst, src, mask) ApplyDestMask(dst, src, mask, 0)

//Copy A Range of Bits From Src[src_offset:(src_offset+n)] To Desc[dst_offset:(dst_offset+n)]
#define CopyBitRange(dst, dst_offset, src, src_offset, n) ApplyDestMask(dst, src, ((1 << n) - 1) << src_offset, (dst_offset - src_offset))
#define SetBitRange(dst, dst_offset, src, src_offset, n) dst = CopyBitRange(dst, dst_offset, src, src_offset, n)


namespace Simple{
    struct Default;
}

#include "SimpleUtils.h"
#include "SimpleLoop.h"

#endif