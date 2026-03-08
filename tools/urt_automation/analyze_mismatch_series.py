#!/usr/bin/env python3
import argparse
import glob
import os
import struct
from collections import Counter


def read_u32(buf: bytes, off: int) -> int:
    return struct.unpack_from("<I", buf, off)[0]


def is_ptr_like(v: int) -> bool:
    return 0x10000000 <= v <= 0x7FFFFFFF and (v & 0x3) == 0


def main():
    p = argparse.ArgumentParser(description="Analyze recent URT mismatch series for field stability.")
    p.add_argument("--blobs-dir", required=True, help="Path to BBCF_IM/URT_RE_BLOBS")
    p.add_argument("--limit", type=int, default=6, help="How many newest mismatch sets to inspect")
    p.add_argument("--header-bytes", type=int, default=512, help="Header range to analyze")
    args = p.parse_args()

    metas = sorted(glob.glob(os.path.join(args.blobs_dir, "mismatch_*_meta.txt")), key=os.path.getmtime, reverse=True)
    metas = metas[: args.limit]
    if not metas:
        raise SystemExit("No mismatch_*_meta.txt files found.")

    pairs = []
    for meta in metas:
        base = meta[: -len("_meta.txt")]
        seeded = base + "_seeded.bin"
        entry = base + "_entry.bin"
        if os.path.isfile(seeded) and os.path.isfile(entry):
            with open(seeded, "rb") as fs, open(entry, "rb") as fe:
                pairs.append((base, fs.read(), fe.read()))

    if not pairs:
        raise SystemExit("No valid seeded/entry pairs found.")

    seeded_sizes = [len(s) for _, s, _ in pairs]
    entry_sizes = [len(e) for _, _, e in pairs]
    deltas = [len(e) - len(s) for _, s, e in pairs]

    print("=== URT Mismatch Series Analysis ===")
    print(f"pairs_analyzed={len(pairs)}")
    print("ids=" + ",".join(os.path.basename(base).replace("mismatch_", "") for base, _, _ in pairs))
    print(f"seeded_size_set={sorted(set(seeded_sizes))}")
    print(f"entry_size_set={sorted(set(entry_sizes))}")
    print(f"delta_set={sorted(set(deltas))}")

    h = args.header_bytes
    slots = min(h // 4, min(min(len(s), len(e)) for _, s, e in pairs) // 4)
    changed_offsets = 0
    ptrlike_changed = 0
    scalar_changed = 0
    stable_same = 0

    examples = []
    for i in range(slots):
        off = i * 4
        vals = [(read_u32(s, off), read_u32(e, off)) for _, s, e in pairs]
        uniq_seed = {a for a, _ in vals}
        uniq_entry = {b for _, b in vals}
        all_same = len(uniq_seed) == 1 and len(uniq_entry) == 1 and next(iter(uniq_seed)) == next(iter(uniq_entry))
        any_diff = any(a != b for a, b in vals)
        if all_same:
            stable_same += 1
            continue
        if any_diff:
            changed_offsets += 1
            ptrish = any(is_ptr_like(a) or is_ptr_like(b) for a, b in vals)
            if ptrish:
                ptrlike_changed += 1
            else:
                scalar_changed += 1
            if len(examples) < 24:
                examples.append((off, vals))

    print(f"header_bytes={h}")
    print(f"header_dwords={slots}")
    print(f"header_stable_same_dwords={stable_same}")
    print(f"header_changed_dwords={changed_offsets}")
    print(f"header_changed_ptrlike_dwords={ptrlike_changed}")
    print(f"header_changed_scalar_dwords={scalar_changed}")

    print("changed_offset_examples:")
    for off, vals in examples:
        sig = " | ".join(f"s=0x{a:08X},e=0x{b:08X}" for a, b in vals)
        print(f"  off=0x{off:03X}: {sig}")

    tail_counts = Counter()
    for _, s, e in pairs:
        tail_counts[len(e) - len(s)] += 1
    print("tail_delta_hist=" + ",".join(f"{k}:{v}" for k, v in sorted(tail_counts.items())))


if __name__ == "__main__":
    main()
