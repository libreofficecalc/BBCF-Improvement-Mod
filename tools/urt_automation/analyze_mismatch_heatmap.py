#!/usr/bin/env python3
import argparse
import glob
import os


def main():
    ap = argparse.ArgumentParser(description="Compute diff-density heatmap across mismatch pairs.")
    ap.add_argument("--blobs-dir", required=True)
    ap.add_argument("--limit", type=int, default=8)
    ap.add_argument("--chunk", type=int, default=4096)
    args = ap.parse_args()

    metas = sorted(glob.glob(os.path.join(args.blobs_dir, "mismatch_*_meta.txt")), key=os.path.getmtime, reverse=True)
    metas = metas[: args.limit]
    pairs = []
    for m in metas:
        base = m[: -len("_meta.txt")]
        s = base + "_seeded.bin"
        e = base + "_entry.bin"
        if os.path.isfile(s) and os.path.isfile(e):
            with open(s, "rb") as fs, open(e, "rb") as fe:
                pairs.append((os.path.basename(base), fs.read(), fe.read()))
    if not pairs:
        raise SystemExit("No valid pairs.")

    min_common = min(min(len(s), len(e)) for _, s, e in pairs)
    chunk = args.chunk
    n_chunks = (min_common + chunk - 1) // chunk

    print("=== URT Mismatch Heatmap ===")
    print("pairs=" + ",".join(name for name, _, _ in pairs))
    print(f"common_prefix={min_common}")
    print(f"chunk_size={chunk}")
    print(f"chunks={n_chunks}")

    rows = []
    for ci in range(n_chunks):
        a = ci * chunk
        b = min(min_common, a + chunk)
        total = (b - a) * len(pairs)
        if total == 0:
            continue
        diff = 0
        pair_diff_counts = []
        for _, s, e in pairs:
            d = sum(1 for i in range(a, b) if s[i] != e[i])
            pair_diff_counts.append(d)
            diff += d
        ratio = diff / total
        rows.append((ci, a, b, ratio, min(pair_diff_counts), max(pair_diff_counts)))

    top = sorted(rows, key=lambda x: x[3], reverse=True)[:24]
    low = sorted(rows, key=lambda x: x[3])[:24]

    print("top_diff_chunks:")
    for ci, a, b, r, mn, mx in top:
        print(f"  idx={ci} off=0x{a:X}-0x{b-1:X} ratio={r:.4f} per_pair_minmax={mn}/{mx}")

    print("low_diff_chunks:")
    for ci, a, b, r, mn, mx in low:
        print(f"  idx={ci} off=0x{a:X}-0x{b-1:X} ratio={r:.4f} per_pair_minmax={mn}/{mx}")


if __name__ == "__main__":
    main()
