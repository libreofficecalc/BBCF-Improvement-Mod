#!/usr/bin/env python3
import argparse
import glob
import os
import struct
from collections import Counter, defaultdict


def u32(buf: bytes, off: int) -> int:
    return struct.unpack_from("<I", buf, off)[0]


def is_ptr_like(v: int) -> bool:
    return 0x10000000 <= v <= 0x7FFFFFFF and (v & 0x3) == 0


def s32(v: int) -> int:
    if v & 0x80000000:
        return -((~v + 1) & 0xFFFFFFFF)
    return v


def main():
    ap = argparse.ArgumentParser(description="Analyze pointer-like delta patterns across mismatch sets.")
    ap.add_argument("--blobs-dir", required=True)
    ap.add_argument("--limit", type=int, default=8)
    ap.add_argument("--scan-bytes", type=int, default=4096)
    args = ap.parse_args()

    metas = sorted(glob.glob(os.path.join(args.blobs_dir, "mismatch_*_meta.txt")), key=os.path.getmtime, reverse=True)
    metas = metas[: args.limit]
    pairs = []
    for meta in metas:
        base = meta[: -len("_meta.txt")]
        s = base + "_seeded.bin"
        e = base + "_entry.bin"
        if os.path.isfile(s) and os.path.isfile(e):
            with open(s, "rb") as fs, open(e, "rb") as fe:
                pairs.append((os.path.basename(base), fs.read(), fe.read()))
    if not pairs:
        raise SystemExit("No valid mismatch pairs found.")

    print("=== URT Pointer Delta Analysis ===")
    print("pairs=" + ",".join(name for name, _, _ in pairs))

    global_delta_hist = Counter()
    offset_delta_hist = defaultdict(Counter)
    same_ptr_count = 0
    ptr_pair_count = 0
    sample_out = []

    for name, seeded, entry in pairs:
        n = min(len(seeded), len(entry), args.scan_bytes)
        local_hist = Counter()
        for off in range(0, n - 3, 4):
            sv = u32(seeded, off)
            ev = u32(entry, off)
            if is_ptr_like(sv) and is_ptr_like(ev):
                ptr_pair_count += 1
                if sv == ev:
                    same_ptr_count += 1
                d = (ev - sv) & 0xFFFFFFFF
                global_delta_hist[d] += 1
                local_hist[d] += 1
                offset_delta_hist[off][d] += 1
                if len(sample_out) < 24 and sv != ev:
                    sample_out.append((name, off, sv, ev, d))
        top_local = local_hist.most_common(6)
        print(f"{name}: ptr_pairs={sum(local_hist.values())} top_deltas=" +
              " ".join(f"{s32(d)}({c})" for d, c in top_local))

    print(f"total_ptr_pairs={ptr_pair_count}")
    print(f"equal_ptr_pairs={same_ptr_count}")
    print(f"changed_ptr_pairs={ptr_pair_count - same_ptr_count}")
    print("global_top_deltas=" + " ".join(f"{s32(d)}({c})" for d, c in global_delta_hist.most_common(12)))

    stable_delta_offsets = []
    for off, hist in offset_delta_hist.items():
        if len(hist) == 1:
            d, c = next(iter(hist.items()))
            if c >= 3:
                stable_delta_offsets.append((off, d, c))
    stable_delta_offsets.sort(key=lambda x: x[0])
    print(f"stable_delta_offsets_count={len(stable_delta_offsets)}")
    if stable_delta_offsets:
        preview = stable_delta_offsets[:24]
        print("stable_delta_offsets_preview=" + " ".join(f"off=0x{o:X},d={s32(d)},n={c}" for o, d, c in preview))

    print("sample_changed_ptrs:")
    for name, off, sv, ev, d in sample_out:
        print(f"  {name} off=0x{off:X} seeded=0x{sv:08X} entry=0x{ev:08X} delta={s32(d)}")


if __name__ == "__main__":
    main()
