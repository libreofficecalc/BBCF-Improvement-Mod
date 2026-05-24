#!/usr/bin/env python3
import argparse
import os
import struct
from collections import Counter


def fnv1a32(data: bytes) -> int:
    h = 0x811C9DC5
    for b in data:
        h ^= b
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h


def find_diff_runs(a: bytes, b: bytes):
    n = min(len(a), len(b))
    runs = []
    in_run = False
    start = 0
    diff_count = 0
    for i in range(n):
        if a[i] != b[i]:
            diff_count += 1
            if not in_run:
                in_run = True
                start = i
        elif in_run:
            runs.append((start, i - 1))
            in_run = False
    if in_run:
        runs.append((start, n - 1))
    return n, diff_count, runs


def tail_offset_candidates(blob: bytes, min_size: int, full_size: int):
    count = 0
    positions = []
    for i in range(0, len(blob) - 3, 4):
        v = struct.unpack_from("<I", blob, i)[0]
        if min_size <= v < full_size:
            count += 1
            if len(positions) < 24:
                positions.append((i, v))
    return count, positions


def dword_counter(blob: bytes, take=32):
    vals = []
    lim = min(len(blob) // 4, take)
    for i in range(lim):
        vals.append(struct.unpack_from("<I", blob, i * 4)[0])
    return vals


def main():
    parser = argparse.ArgumentParser(description="Analyze URT mismatch seeded/entry blobs.")
    parser.add_argument("--seeded", required=True, help="Path to seeded .bin")
    parser.add_argument("--entry", required=True, help="Path to entry .bin")
    args = parser.parse_args()

    with open(args.seeded, "rb") as f:
        seeded = f.read()
    with open(args.entry, "rb") as f:
        entry = f.read()

    min_size, diff_count, runs = find_diff_runs(seeded, entry)
    size_delta = len(entry) - len(seeded)
    run_preview = runs[:16]

    tail_only = b""
    if len(entry) > min_size:
        tail_only = entry[min_size:]

    cand_count, cand_pos = tail_offset_candidates(entry, min_size, len(entry))

    print("=== URT Snapshot Mismatch Analysis ===")
    print(f"seeded_path={os.path.abspath(args.seeded)}")
    print(f"entry_path={os.path.abspath(args.entry)}")
    print(f"seeded_size={len(seeded)}")
    print(f"entry_size={len(entry)}")
    print(f"size_delta={size_delta}")
    print(f"seeded_digest=0x{fnv1a32(seeded):08X}")
    print(f"entry_digest=0x{fnv1a32(entry):08X}")
    print(f"common_prefix_size={min_size}")
    print(f"diff_count_in_common_prefix={diff_count}")
    print(f"diff_run_count={len(runs)}")
    if runs:
        print(f"first_diff={runs[0][0]}")
        print(f"last_diff={runs[-1][1]}")
    else:
        print("first_diff=-1")
        print("last_diff=-1")
    print("diff_runs_preview=" + " ".join(f"[{a}..{b}]" for a, b in run_preview))

    print(f"entry_tail_size={len(tail_only)}")
    print(f"entry_tail_digest=0x{fnv1a32(tail_only):08X}")

    print("entry_first_dwords=" + " ".join(f"0x{v:08X}" for v in dword_counter(entry)))
    print("seeded_first_dwords=" + " ".join(f"0x{v:08X}" for v in dword_counter(seeded)))

    print(f"tail_range_offset_candidates_in_entry={cand_count}")
    if cand_pos:
        print(
            "tail_range_offset_preview="
            + " ".join(f"(off=0x{off:X},val=0x{val:X})" for off, val in cand_pos)
        )

    if len(tail_only) >= 4:
        dword_hist = Counter(struct.unpack_from("<I", tail_only, i)[0] for i in range(0, len(tail_only) - 3, 4))
        common = dword_hist.most_common(12)
        print("entry_tail_top_dwords=" + " ".join(f"(0x{k:08X}:{v})" for k, v in common))


if __name__ == "__main__":
    main()
