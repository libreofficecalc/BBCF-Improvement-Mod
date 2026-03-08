#include "UnlimitedReplayTakeoverManager.h"

#include "Core/Settings.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/gamestates.h"

#include <Windows.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>

namespace {
const char* kDefaultProfileName = "default.urt";
const char* kProfileKind = "unlimited_replay_takeover_profile";
const char* kEntryMagic = "URTE";
const unsigned char kEntryVersionMajor = 1;
const unsigned char kEntryVersionMinor = 1;
const size_t kSnapshotBytesSize = sizeof(Snapshot);
const int kUrtDedicatedLiveSnapshotSlot = 9;
const int kPreviewFrameByteCount = 24;
const size_t kSnapshotHardLimit = 0xA10000;

SnapshotManager* GetCurrentSnapshotManager();

bool PathExists(const std::string& path) {
    const DWORD attrs = GetFileAttributesA(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES;
}

bool IsAbsolutePath(const std::string& path) {
    if (path.size() >= 2 && path[1] == ':') {
        return true;
    }
    if (!path.empty() && (path[0] == '/' || path[0] == '\\')) {
        return true;
    }
    return false;
}

std::string JoinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    const char tail = a.back();
    if (tail == '/' || tail == '\\') {
        return a + b;
    }
    return a + "/" + b;
}

void EnsureDirectoryRecursive(const std::string& dir) {
    if (dir.empty() || PathExists(dir)) {
        return;
    }

    std::string current;
    current.reserve(dir.size());
    for (size_t i = 0; i < dir.size(); ++i) {
        const char c = dir[i];
        current.push_back(c);
        if (c == '/' || c == '\\') {
            if (!current.empty() && !PathExists(current)) {
                CreateDirectoryA(current.c_str(), nullptr);
            }
        }
    }

    if (!PathExists(current)) {
        CreateDirectoryA(current.c_str(), nullptr);
    }
}

std::string Trim(const std::string& s) {
    const size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> Split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        out.push_back(item);
    }
    return out;
}

bool SafeSaveSnapshotNoBuffer(SnapshotApparatus* apparatus) {
    if (!apparatus) {
        LOG(1, "[URT][SAFE] SafeSaveSnapshotNoBuffer apparatus=null\n");
        return false;
    }
    bool ok = false;
    __try {
        ok = apparatus->save_snapshot(nullptr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ok = false;
        LOG(1, "[URT][SAFE] SafeSaveSnapshotNoBuffer exception code=0x%08X\n", GetExceptionCode());
    }
    LOG(1, "[URT][SAFE] SafeSaveSnapshotNoBuffer result=%d apparatus=%p\n", ok ? 1 : 0, apparatus);
    return ok;
}

bool SafeLoadSnapshot(SnapshotApparatus* apparatus, const void* snapshotBytes, size_t snapshotSize) {
    if (!apparatus || !snapshotBytes || snapshotSize == 0) {
        LOG(1, "[URT][SAFE] SafeLoadSnapshot invalid args apparatus=%p snapshotBytes=%p snapshotSize=%u\n",
            apparatus,
            snapshotBytes,
            static_cast<unsigned int>(snapshotSize));
        return false;
    }
    bool ok = false;
    __try {
        ok = apparatus->load_snapshot_sized(snapshotBytes, snapshotSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ok = false;
        LOG(1, "[URT][SAFE] SafeLoadSnapshot exception code=0x%08X\n", GetExceptionCode());
    }
    LOG(1, "[URT][SAFE] SafeLoadSnapshot result=%d apparatus=%p snapshotSize=%u\n",
        ok ? 1 : 0,
        apparatus,
        static_cast<unsigned int>(snapshotSize));
    return ok;
}

bool SafeLoadSnapshotIndex(SnapshotApparatus* apparatus, int snapshotIndex) {
    if (!apparatus || snapshotIndex < 0) {
        LOG(1, "[URT][SAFE] SafeLoadSnapshotIndex invalid args apparatus=%p snapshotIndex=%d\n", apparatus, snapshotIndex);
        return false;
    }
    bool ok = false;
    __try {
        ok = apparatus->load_snapshot_index(snapshotIndex);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ok = false;
        LOG(1, "[URT][SAFE] SafeLoadSnapshotIndex exception code=0x%08X index=%d\n", GetExceptionCode(), snapshotIndex);
    }
    LOG(1, "[URT][SAFE] SafeLoadSnapshotIndex result=%d apparatus=%p index=%d\n", ok ? 1 : 0, apparatus, snapshotIndex);
    return ok;
}

bool SafeLoadSnapshotNoBuffer(SnapshotApparatus* apparatus) {
    if (!apparatus) {
        LOG(1, "[URT][SAFE] SafeLoadSnapshotNoBuffer apparatus=null\n");
        return false;
    }
    bool ok = false;
    __try {
        ok = apparatus->load_snapshot(nullptr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ok = false;
        LOG(1, "[URT][SAFE] SafeLoadSnapshotNoBuffer exception code=0x%08X\n", GetExceptionCode());
    }
    LOG(1, "[URT][SAFE] SafeLoadSnapshotNoBuffer result=%d apparatus=%p\n", ok ? 1 : 0, apparatus);
    return ok;
}

uint32_t FastDigest32(const void* data, size_t size) {
    if (!data || size == 0) {
        return 0;
    }
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < size; ++i) {
        h ^= static_cast<uint32_t>(bytes[i]);
        h *= 16777619u;
    }
    return h;
}

bool IsPtrLikeSnapshotDword(uint32_t v) {
    return v >= 0x01000000u && v < 0x80000000u;
}

size_t ApplyProbeTransformPreservePointers(
    unsigned char* seededDst,
    const unsigned char* seededOriginal,
    const unsigned char* entrySrc,
    size_t size,
    size_t* outSanitizedMinusOneDwords,
    size_t* outReplacedMinusOneFromSeedDwords) {
    if (!seededDst || !seededOriginal || !entrySrc || size == 0) {
        return 0;
    }
    if (outSanitizedMinusOneDwords) {
        *outSanitizedMinusOneDwords = 0;
    }
    if (outReplacedMinusOneFromSeedDwords) {
        *outReplacedMinusOneFromSeedDwords = 0;
    }

    std::memcpy(seededDst, entrySrc, size);

    size_t restoredPtrCount = 0;
    const size_t dwordCount = size / 4;
    for (size_t i = 0; i < dwordCount; ++i) {
        const size_t off = i * 4;
        const uint32_t seededV = *reinterpret_cast<const uint32_t*>(seededOriginal + off);
        const uint32_t entryV = *reinterpret_cast<const uint32_t*>(entrySrc + off);
        if (IsPtrLikeSnapshotDword(seededV) && IsPtrLikeSnapshotDword(entryV)) {
            *reinterpret_cast<uint32_t*>(seededDst + off) = seededV;
            ++restoredPtrCount;
        }
    }

    if (size >= 0x1C + sizeof(uint32_t)) {
        *reinterpret_cast<uint32_t*>(seededDst + 0x14) = *reinterpret_cast<const uint32_t*>(seededOriginal + 0x14);
        *reinterpret_cast<uint32_t*>(seededDst + 0x18) = *reinterpret_cast<const uint32_t*>(seededOriginal + 0x18);
    }

    // Targeted RE probe windows near the failing memcpy source neighborhood.
    auto forceSeedWindow = [&](size_t begin, size_t end) {
        if (size <= begin) {
            return;
        }
        const size_t forceEnd = (std::min)(size, end);
        if (forceEnd > begin) {
            std::memcpy(seededDst + begin, seededOriginal + begin, forceEnd - begin);
        }
    };
    forceSeedWindow(0x1CCB0, 0x1CD20);
    // Expanded prelude window: selector/state immediately before source table.
    forceSeedWindow(0x1B000, 0x1CCB0);

    size_t replacedFromSeedMinusOne = 0;
    size_t sanitizedMinusOne = 0;
    const size_t dwordCountSan = size / 4;
    for (size_t i = 0; i < dwordCountSan; ++i) {
        const size_t off = i * 4;
        const uint32_t entryV = *reinterpret_cast<const uint32_t*>(entrySrc + off);
        if (entryV == 0xFFFFFFFFu) {
            const uint32_t seededV = *reinterpret_cast<const uint32_t*>(seededOriginal + off);
            if (seededV != 0xFFFFFFFFu) {
                *reinterpret_cast<uint32_t*>(seededDst + off) = seededV;
                ++replacedFromSeedMinusOne;
                continue;
            }
        }
        const uint32_t v = *reinterpret_cast<const uint32_t*>(seededDst + off);
        if (v == 0xFFFFFFFFu) {
            *reinterpret_cast<uint32_t*>(seededDst + off) = 0u;
            ++sanitizedMinusOne;
        }
    }
    if (outReplacedMinusOneFromSeedDwords) {
        *outReplacedMinusOneFromSeedDwords = replacedFromSeedMinusOne;
    }
    if (outSanitizedMinusOneDwords) {
        *outSanitizedMinusOneDwords = sanitizedMinusOne;
    }

    return restoredPtrCount;
}

void LogSnapshotDiffSummary(const void* aData, size_t aSize, const void* bData, size_t bSize, const char* tag) {
    if (!aData || !bData || aSize == 0 || bSize == 0) {
        LOG(1,
            "[URT][SEED][DIFF] %s invalid buffers a=%p aSize=%u b=%p bSize=%u\n",
            tag ? tag : "(null)",
            aData,
            static_cast<unsigned int>(aSize),
            bData,
            static_cast<unsigned int>(bSize));
        return;
    }
    const unsigned char* a = static_cast<const unsigned char*>(aData);
    const unsigned char* b = static_cast<const unsigned char*>(bData);
    const size_t minSize = (std::min)(aSize, bSize);
    size_t diffCount = 0;
    size_t firstDiff = static_cast<size_t>(-1);
    size_t lastDiff = 0;
    size_t runCount = 0;
    bool inRun = false;
    size_t runStart = 0;
    char runPreview[384] = {};
    size_t runPreviewLen = 0;

    for (size_t i = 0; i < minSize; ++i) {
        if (a[i] != b[i]) {
            ++diffCount;
            if (firstDiff == static_cast<size_t>(-1)) {
                firstDiff = i;
            }
            lastDiff = i;
            if (!inRun) {
                inRun = true;
                runStart = i;
            }
        } else if (inRun) {
            ++runCount;
            if (runCount <= 8 && runPreviewLen + 40 < sizeof(runPreview)) {
                const size_t runEnd = i - 1;
                const int written = std::snprintf(
                    runPreview + runPreviewLen,
                    sizeof(runPreview) - runPreviewLen,
                    "%s[%u..%u]",
                    runCount == 1 ? "" : " ",
                    static_cast<unsigned int>(runStart),
                    static_cast<unsigned int>(runEnd));
                if (written > 0) {
                    runPreviewLen += static_cast<size_t>(written);
                }
            }
            inRun = false;
        }
    }
    if (inRun) {
        ++runCount;
        if (runCount <= 8 && runPreviewLen + 40 < sizeof(runPreview)) {
            const size_t runEnd = minSize - 1;
            const int written = std::snprintf(
                runPreview + runPreviewLen,
                sizeof(runPreview) - runPreviewLen,
                "%s[%u..%u]",
                runCount == 1 ? "" : " ",
                static_cast<unsigned int>(runStart),
                static_cast<unsigned int>(runEnd));
            if (written > 0) {
                runPreviewLen += static_cast<size_t>(written);
            }
        }
    }

    LOG(1,
        "[URT][SEED][DIFF] %s min=%u diffCount=%u runCount=%u firstDiff=%u lastDiff=%u tailDelta=%d runs=%s\n",
        tag ? tag : "(null)",
        static_cast<unsigned int>(minSize),
        static_cast<unsigned int>(diffCount),
        static_cast<unsigned int>(runCount),
        firstDiff == static_cast<size_t>(-1) ? 0u : static_cast<unsigned int>(firstDiff),
        static_cast<unsigned int>(lastDiff),
        static_cast<int>(aSize) - static_cast<int>(bSize),
        runPreviewLen > 0 ? runPreview : "(none)");

    const size_t dwordCount = minSize / 4;
    int logged = 0;
    for (size_t i = 0; i < dwordCount && logged < 24; ++i) {
        const uint32_t av = *reinterpret_cast<const uint32_t*>(a + (i * 4));
        const uint32_t bv = *reinterpret_cast<const uint32_t*>(b + (i * 4));
        if (av == bv) {
            continue;
        }
        const bool aPtrLike = (av >= 0x01000000u && av < 0x80000000u);
        const bool bPtrLike = (bv >= 0x01000000u && bv < 0x80000000u);
        LOG(1,
            "[URT][SEED][DIFFDW] %s idx=%u off=0x%X seeded=0x%08X entry=0x%08X ptrLike(seed=%d,entry=%d)\n",
            tag ? tag : "(null)",
            static_cast<unsigned int>(i),
            static_cast<unsigned int>(i * 4),
            av,
            bv,
            aPtrLike ? 1 : 0,
            bPtrLike ? 1 : 0);
        ++logged;
    }
}

void DumpMismatchSnapshotsForRe(const unsigned char* seededBuf, size_t seededSize, const unsigned char* entryBuf, size_t entrySize) {
    if (!seededBuf || !entryBuf || seededSize == 0 || entrySize == 0) {
        return;
    }
    const std::string outDir = "BBCF_IM/URT_RE_BLOBS";
    EnsureDirectoryRecursive(outDir);
    const unsigned long long stamp = GetTickCount64();
    const std::string base = JoinPath(outDir, "mismatch_" + std::to_string(stamp));
    const std::string seededPath = base + "_seeded.bin";
    const std::string entryPath = base + "_entry.bin";
    const std::string metaPath = base + "_meta.txt";

    {
        std::ofstream out(seededPath, std::ios::binary | std::ios::trunc);
        if (out.good()) {
            out.write(reinterpret_cast<const char*>(seededBuf), static_cast<std::streamsize>(seededSize));
        }
    }
    {
        std::ofstream out(entryPath, std::ios::binary | std::ios::trunc);
        if (out.good()) {
            out.write(reinterpret_cast<const char*>(entryBuf), static_cast<std::streamsize>(entrySize));
        }
    }
    {
        std::ofstream out(metaPath, std::ios::binary | std::ios::trunc);
        if (out.good()) {
            out << "seeded_size=" << static_cast<unsigned int>(seededSize) << "\n";
            out << "entry_size=" << static_cast<unsigned int>(entrySize) << "\n";
            out << "seeded_digest=0x" << std::hex << FastDigest32(seededBuf, seededSize) << "\n";
            out << "entry_digest=0x" << std::hex << FastDigest32(entryBuf, entrySize) << "\n";
        }
    }
    LOG(1,
        "[URT][SEED] dumped mismatch blobs seeded='%s' entry='%s' meta='%s'\n",
        seededPath.c_str(),
        entryPath.c_str(),
        metaPath.c_str());
}

bool TryLoadSerializedSnapshotViaSeededSlot(
    SnapshotApparatus* apparatus,
    const void* snapshotBytes,
    size_t snapshotSize,
    int preferredSlot,
    bool* outHardIncompatible,
    bool* outUsedMismatchProbe) {
    if (outHardIncompatible) {
        *outHardIncompatible = false;
    }
    if (outUsedMismatchProbe) {
        *outUsedMismatchProbe = false;
    }
    if (!apparatus || !snapshotBytes || snapshotSize == 0 || snapshotSize > kSnapshotHardLimit) {
        LOG(1,
            "[URT][SEED] invalid args apparatus=%p snapshotBytes=%p snapshotSize=%u preferredSlot=%d\n",
            apparatus,
            snapshotBytes,
            static_cast<unsigned int>(snapshotSize),
            preferredSlot);
        return false;
    }
    SnapshotManager* snapManager = GetCurrentSnapshotManager();
    if (!snapManager) {
        LOG(1, "[URT][SEED] no snapshot manager\n");
        return false;
    }

    const unsigned int oldCount = apparatus->snapshot_count;
    if (preferredSlot >= 0) {
        apparatus->snapshot_count = static_cast<unsigned int>(preferredSlot % 10);
    }
    LOG(1,
        "[URT][SEED] seeding training slot oldCount=%u newCount=%u preferredSlot=%d\n",
        oldCount,
        apparatus->snapshot_count,
        preferredSlot);
    const bool seedOk = SafeSaveSnapshotNoBuffer(apparatus);
    if (!seedOk) {
        apparatus->snapshot_count = oldCount;
        LOG(1, "[URT][SEED] seed save failed\n");
        return false;
    }

    const int slotIndex = static_cast<int>((apparatus->snapshot_count + 9) % 10);
    auto& slot = snapManager->_saved_states_related_struct[slotIndex];
    if (!slot._ptr_buf_saved_frame) {
        LOG(1, "[URT][SEED] slot has null buffer slot=%d\n", slotIndex);
        return false;
    }

    const uint32_t entryDigest = FastDigest32(snapshotBytes, snapshotSize);
    const uint32_t slotDigestBefore = FastDigest32(slot._ptr_buf_saved_frame, snapshotSize);
    int seededSize = slot.field2_0x8;
    LOG(1,
        "[URT][SEED] slot=%d slotPtr=%p size_before=%d size_entry=%u digest_before=0x%08X digest_entry=0x%08X\n",
        slotIndex,
        slot._ptr_buf_saved_frame,
        seededSize,
        static_cast<unsigned int>(snapshotSize),
        slotDigestBefore,
        entryDigest);
    LogSnapshotDiffSummary(slot._ptr_buf_saved_frame, static_cast<size_t>((std::max)(seededSize, 0)),
        snapshotBytes, snapshotSize, "seeded-vs-entry(pre-copy)");
    if (seededSize != static_cast<int>(snapshotSize) && g_gameVals.pGameMode != nullptr) {
        const int oldGameMode = *g_gameVals.pGameMode;
        if (oldGameMode == GameMode_Training) {
            LOG(1,
                "[URT][SEED] size mismatch under training; trying replay-theater reseed oldMode=%d newMode=%d\n",
                oldGameMode,
                GameMode_ReplayTheater);
            *g_gameVals.pGameMode = GameMode_ReplayTheater;
            apparatus->snapshot_count = static_cast<unsigned int>(preferredSlot % 10);
            const bool replayModeSeedOk = SafeSaveSnapshotNoBuffer(apparatus);
            *g_gameVals.pGameMode = oldGameMode;
            LOG(1, "[URT][SEED] replay-theater reseed result=%d restoredMode=%d\n",
                replayModeSeedOk ? 1 : 0,
                oldGameMode);
            if (replayModeSeedOk) {
                seededSize = slot.field2_0x8;
                const uint32_t slotDigestReplayReseed = FastDigest32(slot._ptr_buf_saved_frame, snapshotSize);
                LOG(1,
                    "[URT][SEED] replay-theater reseed slot=%d size=%d digest=0x%08X targetSize=%u\n",
                    slotIndex,
                    seededSize,
                    slotDigestReplayReseed,
                    static_cast<unsigned int>(snapshotSize));
                LogSnapshotDiffSummary(
                    slot._ptr_buf_saved_frame,
                    static_cast<size_t>((std::max)(seededSize, 0)),
                    snapshotBytes,
                    snapshotSize,
                    "seeded-vs-entry(replay-mode-reseed)");
            }
        }
    }
    if (seededSize <= 0 || seededSize > static_cast<int>(kSnapshotHardLimit) || static_cast<size_t>(seededSize) != snapshotSize) {
        const bool allowMismatchProbe = Settings::settingsIni.urtReAllowSizeMismatchProbe;
        DumpMismatchSnapshotsForRe(
            reinterpret_cast<const unsigned char*>(slot._ptr_buf_saved_frame),
            static_cast<size_t>((std::max)(seededSize, 0)),
            reinterpret_cast<const unsigned char*>(snapshotBytes),
            snapshotSize);
        if (allowMismatchProbe && seededSize > 0) {
            if (outUsedMismatchProbe) {
                *outUsedMismatchProbe = true;
            }
            const size_t transplantSize = (std::min)(static_cast<size_t>(seededSize), snapshotSize);
            std::vector<unsigned char> seededBefore(transplantSize);
            std::memcpy(seededBefore.data(), slot._ptr_buf_saved_frame, transplantSize);
            LOG(1,
                "[URT][SEED] size-mismatch probe enabled: transform+transplant min bytes=%u into seeded_size=%d (analysis-only)\n",
                static_cast<unsigned int>(transplantSize),
                seededSize);
            size_t sanitizedMinusOneDwords = 0;
            size_t replacedMinusOneFromSeedDwords = 0;
            const size_t restoredPtrCount = ApplyProbeTransformPreservePointers(
                reinterpret_cast<unsigned char*>(slot._ptr_buf_saved_frame),
                seededBefore.data(),
                reinterpret_cast<const unsigned char*>(snapshotBytes),
                transplantSize,
                &sanitizedMinusOneDwords,
                &replacedMinusOneFromSeedDwords);
            LOG(1,
                "[URT][SEED] probe transform restored_ptr_dwords=%u replaced_minus1_from_seed_dwords=%u sanitized_minus1_dwords=%u forced_seed_windows=[0x1B000..0x1CCB0)+[0x1CCB0..0x1CD20) digest_before=0x%08X digest_after=0x%08X\n",
                static_cast<unsigned int>(restoredPtrCount),
                static_cast<unsigned int>(replacedMinusOneFromSeedDwords),
                static_cast<unsigned int>(sanitizedMinusOneDwords),
                FastDigest32(seededBefore.data(), transplantSize),
                FastDigest32(slot._ptr_buf_saved_frame, transplantSize));
            slot.field2_0x8 = seededSize;
            slot._framecount = g_gameVals.pFrameCount ? *g_gameVals.pFrameCount : 0;
            apparatus->last_saved_snapshot_size = seededSize;
            LogSnapshotDiffSummary(
                slot._ptr_buf_saved_frame,
                static_cast<size_t>(seededSize),
                snapshotBytes,
                snapshotSize,
                "seeded-vs-entry(post-probe-transplant)");
            LOG(1,
                "[URT][SEED] mismatch probe attempting load by slot=%d seededSize=%d entrySize=%u unsafeMode=%d\n",
                slotIndex,
                seededSize,
                static_cast<unsigned int>(snapshotSize),
                Settings::settingsIni.urtReAllowUnsafeProbeLoad ? 1 : 0);
            const bool probeLoadOk = SafeLoadSnapshotIndex(apparatus, slotIndex);
            LOG(1,
                "[URT][SEED] mismatch probe load result=%d slot=%d\n",
                probeLoadOk ? 1 : 0,
                slotIndex);
            if (probeLoadOk) {
                return true;
            }
        }
        LOG(1,
            "[URT][SEED] hard incompatible snapshot sizes: seeded=%d entry=%u (skipping load to avoid crash; probe=%d)\n",
            seededSize,
            static_cast<unsigned int>(snapshotSize),
            allowMismatchProbe ? 1 : 0);
        if (outHardIncompatible) {
            *outHardIncompatible = true;
        }
        return false;
    }

    std::memcpy(slot._ptr_buf_saved_frame, snapshotBytes, snapshotSize);
    slot.field2_0x8 = static_cast<int>(snapshotSize);
    slot._framecount = g_gameVals.pFrameCount ? *g_gameVals.pFrameCount : 0;
    apparatus->last_saved_snapshot_size = static_cast<int>(snapshotSize);
    const uint32_t slotDigestAfter = FastDigest32(slot._ptr_buf_saved_frame, snapshotSize);
    LOG(1,
        "[URT][SEED] slot overwrite complete slot=%d size_now=%d framecount=%u digest_after=0x%08X\n",
        slotIndex,
        slot.field2_0x8,
        slot._framecount,
        slotDigestAfter);

    const bool loadOk = SafeLoadSnapshotIndex(apparatus, slotIndex);
    LOG(1, "[URT][SEED] load by slot result=%d slot=%d\n", loadOk ? 1 : 0, slotIndex);
    return loadOk;
}

void LogUrtContext(const char* tag) {
    const int gameMode = (g_gameVals.pGameMode != nullptr) ? *g_gameVals.pGameMode : -1;
    const int gameState = (g_gameVals.pGameState != nullptr) ? *g_gameVals.pGameState : -1;
    const int matchState = (g_gameVals.pMatchState != nullptr) ? *g_gameVals.pMatchState : -1;
    const int frameCount = (g_gameVals.pFrameCount != nullptr) ? *g_gameVals.pFrameCount : -1;
    const int matchTimer = (g_gameVals.pMatchTimer != nullptr) ? *g_gameVals.pMatchTimer : -1;
    const int round = static_cast<int>(*(GetBbcfBaseAdress() + 0x11C034C));
    LOG(1,
        "[URT][CTX] %s gm=%d gs=%d ms=%d frame=%d timer=%d round=%d p1=%p p2=%p\n",
        tag ? tag : "(null)",
        gameMode,
        gameState,
        matchState,
        frameCount,
        matchTimer,
        round,
        g_interfaces.player1.GetData(),
        g_interfaces.player2.GetData());
}

bool ValidatePostSnapshotStateForPlayback(std::string* outReason) {
    const int gameMode = (g_gameVals.pGameMode != nullptr) ? *g_gameVals.pGameMode : -1;
    const int gameState = (g_gameVals.pGameState != nullptr) ? *g_gameVals.pGameState : -1;
    const int matchState = (g_gameVals.pMatchState != nullptr) ? *g_gameVals.pMatchState : -1;
    const int frameCount = (g_gameVals.pFrameCount != nullptr) ? *g_gameVals.pFrameCount : -1;
    const int matchTimer = (g_gameVals.pMatchTimer != nullptr) ? *g_gameVals.pMatchTimer : -1;
    const int round = static_cast<int>(*(GetBbcfBaseAdress() + 0x11C034C));
    auto* p1 = g_interfaces.player1.GetData();
    auto* p2 = g_interfaces.player2.GetData();

    auto fail = [&](const char* reason) {
        if (outReason) {
            *outReason = reason ? reason : "unknown";
        }
        return false;
    };

    if (!p1 || !p2) {
        return fail("null player pointers");
    }
    if (gameMode != 6) {
        return fail("unexpected game mode after snapshot restore");
    }
    if (gameState != 15) {
        return fail("unexpected game state after snapshot restore");
    }
    if (matchState < 0 || matchState > 5) {
        return fail("match state out of expected range");
    }
    if (frameCount < 0 || frameCount > 200000) {
        return fail("frame count out of expected range");
    }
    if (matchTimer < 0 || matchTimer > 9999) {
        return fail("match timer out of expected range");
    }
    if (round < 0 || round > 5) {
        return fail("round value out of expected range");
    }
    return true;
}

bool ValidateRuntimePlaybackStateForTraining(std::string* outReason) {
    const int gameMode = (g_gameVals.pGameMode != nullptr) ? *g_gameVals.pGameMode : -1;
    const int gameState = (g_gameVals.pGameState != nullptr) ? *g_gameVals.pGameState : -1;
    const int matchState = (g_gameVals.pMatchState != nullptr) ? *g_gameVals.pMatchState : -1;
    const int frameCount = (g_gameVals.pFrameCount != nullptr) ? *g_gameVals.pFrameCount : -1;
    const int matchTimer = (g_gameVals.pMatchTimer != nullptr) ? *g_gameVals.pMatchTimer : -1;
    const int round = static_cast<int>(*(GetBbcfBaseAdress() + 0x11C034C));

    auto fail = [&](const char* reason) {
        if (outReason) {
            *outReason = reason ? reason : "unknown";
        }
        return false;
    };

    if (gameMode != 6) {
        return fail("runtime check failed: game mode");
    }
    if (gameState != 15) {
        return fail("runtime check failed: game state");
    }
    if (matchState < 0 || matchState > 5) {
        return fail("runtime check failed: match state");
    }
    if (frameCount < 0 || frameCount > 400000) {
        return fail("runtime check failed: frame count");
    }
    if (matchTimer < 0 || matchTimer > 9999) {
        return fail("runtime check failed: match timer");
    }
    if (round < 0 || round > 5) {
        return fail("runtime check failed: round");
    }
    return true;
}

bool TryRepairPostSnapshotCoreState(int fallbackRound, int fallbackMatchTimer, int fallbackMatchState) {
    char* base = GetBbcfBaseAdress();
    if (!base) {
        return false;
    }
    bool repaired = false;
    int* roundPtr = reinterpret_cast<int*>(base + 0x11C034C);
    if (roundPtr) {
        const int round = *roundPtr;
        if (round < 0 || round > 5) {
            const int newRound = (fallbackRound >= 0 && fallbackRound <= 5) ? fallbackRound : 0;
            LOG(1, "[URT][REPAIR] round repair old=%d new=%d\n", round, newRound);
            *roundPtr = newRound;
            repaired = true;
        }
    }
    if (g_gameVals.pMatchTimer) {
        const int timer = *g_gameVals.pMatchTimer;
        if (timer <= 0 || timer > 9999) {
            const int newTimer = (fallbackMatchTimer > 0 && fallbackMatchTimer <= 9999) ? fallbackMatchTimer : 3599;
            LOG(1, "[URT][REPAIR] timer repair old=%d new=%d\n", timer, newTimer);
            *g_gameVals.pMatchTimer = newTimer;
            repaired = true;
        }
    }
    if (g_gameVals.pMatchState) {
        const int ms = *g_gameVals.pMatchState;
        const bool fallbackValid = (fallbackMatchState >= 0 && fallbackMatchState <= 5);
        if (ms < 0 || ms > 5 || (fallbackValid && ms != fallbackMatchState)) {
            const int newMs = fallbackValid ? fallbackMatchState : 3;
            LOG(1, "[URT][REPAIR] matchState repair old=%d new=%d fallback=%d\n", ms, newMs, fallbackMatchState);
            *g_gameVals.pMatchState = newMs;
            repaired = true;
        }
    }
    return repaired;
}

void LogSnapshotApparatusState(const char* tag, SnapshotApparatus* apparatus) {
    if (!apparatus) {
        LOG(1, "[URT][SNAP] %s apparatus=null\n", tag ? tag : "(null)");
        return;
    }
    LOG(1,
        "[URT][SNAP] %s apparatus=%p snapshot_count=%u last_size=%d p1_ptr=%p p2_ptr=%p cb=%p\n",
        tag ? tag : "(null)",
        apparatus,
        apparatus->snapshot_count,
        apparatus->get_last_saved_snapshot_size(),
        apparatus->p1_ptr,
        apparatus->p2_ptr,
        apparatus->callbacks_ptr);
}

void LogFramePreview(const char* tag, const std::vector<char>& frames) {
    char preview[256] = {};
    size_t written = 0;
    const int previewCount = (std::min)(static_cast<int>(frames.size()), kPreviewFrameByteCount);
    for (int i = 0; i < previewCount && written + 4 < sizeof(preview); ++i) {
        const unsigned int b = static_cast<unsigned char>(frames[static_cast<size_t>(i)]);
        const int n = std::snprintf(preview + written, sizeof(preview) - written, "%02X%s", b, (i + 1 == previewCount) ? "" : " ");
        if (n <= 0) {
            break;
        }
        written += static_cast<size_t>(n);
    }
    LOG(1,
        "[URT][FRAMES] %s frame_count=%u preview(%d)=[%s]\n",
        tag ? tag : "(null)",
        static_cast<unsigned int>(frames.size()),
        previewCount,
        preview);
}

SnapshotManager* GetCurrentSnapshotManager() {
    char* base_addr = GetBbcfBaseAdress();
    if (!base_addr) {
        return nullptr;
    }
    static_DAT_of_PTR_on_load_4* dat = reinterpret_cast<static_DAT_of_PTR_on_load_4*>(base_addr + 0x612718);
    if (!dat) {
        return nullptr;
    }
    return dat->ptr_snapshot_manager_mine;
}

}

UnlimitedReplayTakeoverManager& UnlimitedReplayTakeoverManager::Instance() {
    static UnlimitedReplayTakeoverManager s_instance;
    return s_instance;
}

UnlimitedReplayTakeoverManager::UnlimitedReplayTakeoverManager() {
}

void UnlimitedReplayTakeoverManager::InitializeIfNeeded() {
    if (m_initialized) {
        return;
    }

    EnsureFolders();
    m_activeProfilePath = JoinPath(GetProfileFolder(), kDefaultProfileName);
    m_lastLoadedProfileFolder = GetProfileFolder();
    m_initialized = true;
    if (!LoadProfile(m_activeProfilePath)) {
        SaveProfile(m_activeProfilePath);
    }
}

void UnlimitedReplayTakeoverManager::Tick() {
    InitializeIfNeeded();
    PruneExpiredToasts();

    if (m_recordingActive) {
        if (!IsReplayMatchActive()) {
            CancelRecording("Replay recording cancelled (left replay match).");
        } else {
            const int currentRound = static_cast<int>(*(GetBbcfBaseAdress() + 0x11C034C));
            if (currentRound != m_recordingRound) {
                CancelRecording("Replay recording cancelled (round changed).");
            }
        }
    }

    if (m_lastPlaybackStartedByManager) {
        const unsigned long long nowMs = GetTickCount64();
        const int gameMode = (g_gameVals.pGameMode != nullptr) ? *g_gameVals.pGameMode : -1;
        const int gameState = (g_gameVals.pGameState != nullptr) ? *g_gameVals.pGameState : -1;
        const int matchState = (g_gameVals.pMatchState != nullptr) ? *g_gameVals.pMatchState : -1;
        const int frameCount = (g_gameVals.pFrameCount != nullptr) ? *g_gameVals.pFrameCount : -1;
        const int matchTimer = (g_gameVals.pMatchTimer != nullptr) ? *g_gameVals.pMatchTimer : -1;
        const int round = static_cast<int>(*(GetBbcfBaseAdress() + 0x11C034C));
        int playbackControl = -1;
        int playbackPosition = -1;
        int activeSlot = -1;
        if (m_playbackManager.playback_control_p) {
            playbackControl = static_cast<int>(*reinterpret_cast<short*>(m_playbackManager.playback_control_p));
        }
        if (m_playbackManager.bbcf_base_adress) {
            playbackPosition = *reinterpret_cast<int*>(m_playbackManager.bbcf_base_adress + 0x13AD940);
        }
        if (m_playbackManager.active_slot_p) {
            activeSlot = *reinterpret_cast<int*>(m_playbackManager.active_slot_p);
        }

        if (m_runtimePlaybackNextWatchdogLogAtMs == 0 || nowMs >= m_runtimePlaybackNextWatchdogLogAtMs) {
            const unsigned long long elapsedMs = (m_runtimePlaybackStartedAtMs > 0) ? (nowMs - m_runtimePlaybackStartedAtMs) : 0;
            LOG(1,
                "[URT][WATCH] t=%llums gm=%d gs=%d ms=%d frame=%d timer=%d round=%d pbCtrl=%d pbPos=%d activeSlot=%d frozen=%d setupPending=%d runtimePending=%d\n",
                elapsedMs,
                gameMode,
                gameState,
                matchState,
                frameCount,
                matchTimer,
                round,
                playbackControl,
                playbackPosition,
                activeSlot,
                g_gameVals.isFrameFrozen ? 1 : 0,
                m_setupPending ? 1 : 0,
                m_runtimeSlotRestorePending ? 1 : 0);
            m_runtimePlaybackNextWatchdogLogAtMs = nowMs + 250;
        }

        std::string runtimeInvalidReason;
        if (!ValidateRuntimePlaybackStateForTraining(&runtimeInvalidReason)) {
            LOG(1,
                "[URT][WATCH] Invalid runtime playback state; cancelling takeover. reason='%s'\n",
                runtimeInvalidReason.c_str());
            CancelActiveTakeover();
            PushToast("Replay takeover cancelled: runtime state became invalid.");
            return;
        }
    }

    if (m_setupPending && GetTickCount64() >= m_setupUnfreezeAtMs) {
        LOG(1, "[URT] Setup delay expired; enabling playback now\n");
        m_setupPending = false;
        m_playbackManager.set_playback_position(0);
        m_playbackManager.set_playback_control(3);
        LOG(1, "[URT] Playback started on runtime slot (after setup delay)\n");
        LogUrtContext("Tick:after setup-delay playback enable");
    }

    if (m_runtimeSlotRestorePending && !IsRuntimePlaybackActive()) {
        if (m_continuousSessionActive && m_continuousPicking) {
            size_t nextIndex = 0;
            if (!PickEntryIndex(&nextIndex) || !StartEntryByIndex(nextIndex)) {
                m_continuousSessionActive = false;
                RestoreRuntimeSlotNow();
                PushToast("Continuous replay takeover stopped.");
            }
        } else {
            RestoreRuntimeSlotNow();
            m_continuousSessionActive = false;
        }
    }

    if (!IsTrainingMatchActive()) {
        m_setupPending = false;
        if (m_continuousSessionActive || m_runtimeSlotRestorePending) {
            m_continuousSessionActive = false;
            if (m_playbackManager.playback_control_p) {
                m_playbackManager.set_playback_control(0);
            }
            if (m_runtimeSlotBackupValid) {
                RestoreRuntimeSlotNow();
            }
        }
        return;
    }

    if (IsKeyPressedEdge(m_cancelKeyCode)) {
        CancelActiveTakeover();
    }

    if (IsKeyPressedEdge(m_triggerKeyCode)) {
        TriggerNow();
    }
}

bool UnlimitedReplayTakeoverManager::StartRecording(bool takeoverAsP1) {
    InitializeIfNeeded();
    LOG(1, "[URT] StartRecording takeoverAs=%s\n", takeoverAsP1 ? "P1" : "P2");
    LogUrtContext("StartRecording:begin");

    if (!IsReplayMatchActive()) {
        LOG(1, "[URT] StartRecording aborted: replay match not active\n");
        PushToast("Start recording only while a replay match is active.");
        return false;
    }

    if (!TryEnsureCaptureSnapshotApparatus()) {
        LOG(1, "[URT] StartRecording aborted: capture snapshot apparatus init failed\n");
        PushToast("Failed to initialize replay snapshot apparatus.");
        return false;
    }
    LogSnapshotApparatusState("StartRecording:capture apparatus ready", m_captureSnapshotApparatus);

    int oldGameMode = 0;
    if (g_gameVals.pGameMode) {
        oldGameMode = *g_gameVals.pGameMode;
        *g_gameVals.pGameMode = GameMode_Training;
        LOG(1, "[URT] StartRecording switched game mode: old=%d new=%d\n", oldGameMode, GameMode_Training);
    }

    Snapshot* snapshotPtr = &m_recordingSnapshot;
    std::memset(&m_recordingSnapshot, 0, sizeof(m_recordingSnapshot));
    // Do not mutate snapshot_count here: forcing a dedicated slot in replay context can
    // poison later cleanup paths before we even attempt URT playback in training.
    LOG(1, "[URT] StartRecording capture save using current snapshot_count=%u\n",
        static_cast<unsigned int>(m_captureSnapshotApparatus->snapshot_count));
    const bool saveOk = m_captureSnapshotApparatus->save_snapshot(&snapshotPtr);
    LOG(1, "[URT] StartRecording save_snapshot result=%d snapshotPtr=%p\n", saveOk ? 1 : 0, snapshotPtr);
    LogSnapshotApparatusState("StartRecording:after save", m_captureSnapshotApparatus);

    if (g_gameVals.pGameMode) {
        *g_gameVals.pGameMode = oldGameMode;
        LOG(1, "[URT] StartRecording restored game mode=%d\n", oldGameMode);
    }

    if (!saveOk) {
        LOG(1, "[URT] StartRecording failed: save_snapshot returned false\n");
        PushToast("Failed to capture replay start state.");
        return false;
    }
    const int snapshotSize = m_captureSnapshotApparatus->get_last_saved_snapshot_size();
    LOG(1, "[URT] StartRecording snapshotSize=%d max=%u\n", snapshotSize, static_cast<unsigned int>(kSnapshotBytesSize));
    if (snapshotSize <= 0 || snapshotSize > static_cast<int>(kSnapshotBytesSize)) {
        LOG(1, "[URT] StartRecording failed: invalid snapshot size\n");
        PushToast("Failed to capture replay start state.");
        return false;
    }

    m_recordingActive = true;
    m_recordingTakeoverAsP1 = takeoverAsP1;
    m_recordingStartFrame = g_gameVals.pFrameCount ? *g_gameVals.pFrameCount : 0;
    m_recordingRound = static_cast<int>(*(GetBbcfBaseAdress() + 0x11C034C));
    m_recordingSnapshotValid = true;
    m_recordingSnapshotSize = static_cast<size_t>(snapshotSize);
    // Live-slot reuse is disabled for URT entries; serialized snapshot path is authoritative.
    m_recordingLiveSnapshotIndex = -1;
    m_recordingP1CharIndex = g_interfaces.player1.GetData() ? g_interfaces.player1.GetData()->charIndex : -1;
    m_recordingP2CharIndex = g_interfaces.player2.GetData() ? g_interfaces.player2.GetData()->charIndex : -1;
    LOG(1, "[URT] StartRecording success: startFrame=%d round=%d snapshotSize=%u liveSlot=%d\n",
        m_recordingStartFrame,
        m_recordingRound,
        static_cast<unsigned int>(m_recordingSnapshotSize),
        m_recordingLiveSnapshotIndex);
    LOG(1, "[URT] StartRecording character capture p1Char=%d p2Char=%d\n",
        m_recordingP1CharIndex,
        m_recordingP2CharIndex);
    PushToast(std::string("Recording started: takeover as ") + (takeoverAsP1 ? "P1" : "P2") + ".");
    return true;
}

bool UnlimitedReplayTakeoverManager::StopRecordingAndSave(const std::string& displayName) {
    InitializeIfNeeded();
    LOG(1, "[URT] StopRecordingAndSave requested name='%s'\n", displayName.c_str());
    LogUrtContext("StopRecordingAndSave:begin");

    if (!m_recordingActive) {
        LOG(1, "[URT] StopRecordingAndSave aborted: no active recording\n");
        PushToast("No replay recording in progress.");
        return false;
    }
    if (!m_recordingSnapshotValid) {
        LOG(1, "[URT] StopRecordingAndSave aborted: recording snapshot invalid\n");
        CancelRecording("Replay recording cancelled (snapshot unavailable).");
        return false;
    }

    if (!IsReplayMatchActive()) {
        LOG(1, "[URT] StopRecordingAndSave aborted: replay match not active anymore\n");
        CancelRecording("Replay recording cancelled (left replay match).");
        return false;
    }

    const int endFrame = g_gameVals.pFrameCount ? *g_gameVals.pFrameCount : 0;
    LOG(1, "[URT] StopRecordingAndSave frame range start=%d end=%d\n", m_recordingStartFrame, endFrame);
    if (endFrame <= m_recordingStartFrame) {
        LOG(1, "[URT] StopRecordingAndSave aborted: invalid frame range\n");
        CancelRecording("Replay recording cancelled (too short).");
        return false;
    }

    const int recordedPlayer = m_recordingTakeoverAsP1 ? 1 : 0;
    LOG(1, "[URT] StopRecordingAndSave recordedPlayer=%d recordingRound=%d\n", recordedPlayer, m_recordingRound);
    std::vector<char> frames;
    if (!BuildPlaybackFramesFromReplayRange(m_recordingRound, m_recordingStartFrame, endFrame, recordedPlayer, &frames)) {
        LOG(1, "[URT] StopRecordingAndSave failed: BuildPlaybackFramesFromReplayRange failed\n");
        CancelRecording("Replay recording cancelled (failed reading replay frames).");
        return false;
    }
    LogFramePreview("StopRecordingAndSave:capture", frames);

    LoadedEntryData data;
    data.loaded = true;
    data.facingLeft = m_recordingTakeoverAsP1
        ? (g_interfaces.player2.GetData() && g_interfaces.player2.GetData()->facingLeft2 != 0)
        : (g_interfaces.player1.GetData() && g_interfaces.player1.GetData()->facingLeft2 != 0);
    data.playbackFrames = frames;
    const size_t snapshotSizeToSave = m_recordingSnapshotSize;
    LOG(1, "[URT] StopRecordingAndSave snapshotSizeToSave=%u sourceSnapshotSize=%u\n",
        static_cast<unsigned int>(snapshotSizeToSave),
        static_cast<unsigned int>(m_recordingSnapshotSize));
    if (m_recordingSnapshotSize == 0 || m_recordingSnapshotSize > kSnapshotBytesSize) {
        LOG(1, "[URT] StopRecordingAndSave aborted: invalid recorded snapshot size=%u\n",
            static_cast<unsigned int>(m_recordingSnapshotSize));
        CancelRecording("Replay recording cancelled (snapshot unavailable).");
        return false;
    }
    data.snapshotBytes.resize(snapshotSizeToSave);
    std::memcpy(data.snapshotBytes.data(), &m_recordingSnapshot, snapshotSizeToSave);
    data.hasLiveSnapshotIndex = m_recordingLiveSnapshotIndex >= 0;
    data.liveSnapshotIndex = m_recordingLiveSnapshotIndex;
    data.snapshotManagerAddress = reinterpret_cast<uintptr_t>(GetCurrentSnapshotManager());
    data.recordedP1CharIndex = m_recordingP1CharIndex;
    data.recordedP2CharIndex = m_recordingP2CharIndex;
    LOG(1, "[URT] StopRecordingAndSave snapshot manager address=0x%08X\n",
        static_cast<unsigned int>(data.snapshotManagerAddress));

    std::string entryName = displayName.empty() ? ("Replay " + std::to_string(static_cast<unsigned long long>(GetTickCount64()))) : displayName;
    const std::string relPath = BuildUniqueRelativePath(entryName);
    const std::string fullPath = JoinPath(GetLibraryFolder(), relPath);
    LOG(1, "[URT] StopRecordingAndSave writing entry name='%s' rel='%s' full='%s'\n",
        entryName.c_str(),
        relPath.c_str(),
        fullPath.c_str());
    if (!SaveEntryData(fullPath, data)) {
        LOG(1, "[URT] StopRecordingAndSave failed: SaveEntryData failed\n");
        CancelRecording("Replay recording cancelled (failed writing entry).");
        return false;
    }

    ReplayTakeoverEntry entry;
    entry.id = MakeEntryId();
    entry.name = entryName;
    entry.relativePath = relPath;
    entry.enabled = true;
    entry.weight = 1.0f;
    m_entries.push_back(entry);
    m_cache[entry.id] = data;
    m_runtimeLiveSnapshotSlots[entry.id] = m_recordingLiveSnapshotIndex;

    m_recordingActive = false;
    m_recordingSnapshotValid = false;
    m_recordingSnapshotSize = 0;
    m_recordingLiveSnapshotIndex = -1;
    m_recordingP1CharIndex = -1;
    m_recordingP2CharIndex = -1;
    LOG(1, "[URT] StopRecordingAndSave success id='%s' total_entries=%u\n",
        entry.id.c_str(),
        static_cast<unsigned int>(m_entries.size()));
    PushToast("Replay takeover entry saved.");
    return true;
}

void UnlimitedReplayTakeoverManager::CancelRecording(const char* reason) {
    LOG(1, "[URT] CancelRecording reason='%s'\n", reason ? reason : "(null)");
    m_recordingActive = false;
    m_recordingSnapshotValid = false;
    m_recordingSnapshotSize = 0;
    m_recordingLiveSnapshotIndex = -1;
    m_recordingP1CharIndex = -1;
    m_recordingP2CharIndex = -1;
    if (reason && reason[0] != '\0') {
        PushToast(reason);
    }
}

bool UnlimitedReplayTakeoverManager::IsRecording() const { return m_recordingActive; }
bool UnlimitedReplayTakeoverManager::IsRecordingAsP1() const { return m_recordingTakeoverAsP1; }
int UnlimitedReplayTakeoverManager::GetRecordingStartFrame() const { return m_recordingStartFrame; }

bool UnlimitedReplayTakeoverManager::TriggerNow() {
    LOG(1, "[URT] TriggerNow\n");
    LogUrtContext("TriggerNow");
    if (!IsTrainingMatchActive()) {
        LOG(1, "[URT] TriggerNow aborted: training not active\n");
        PushToast("Replay takeover library only runs in training mode.");
        return false;
    }

    size_t idx = 0;
    if (!PickEntryIndex(&idx)) {
        LOG(1, "[URT] TriggerNow aborted: no eligible entry\n");
        PushToast("No eligible replay takeover entry.");
        return false;
    }
    LOG(1, "[URT] TriggerNow picked idx=%u continuous=%d\n", static_cast<unsigned int>(idx), m_continuousPicking ? 1 : 0);

    m_continuousSessionActive = m_continuousPicking;
    if (!StartEntryByIndex(idx)) {
        m_continuousSessionActive = false;
        return false;
    }
    return true;
}

bool UnlimitedReplayTakeoverManager::PlayEntryByIndex(size_t idx) {
    LOG(1, "[URT] PlayEntryByIndex idx=%u\n", static_cast<unsigned int>(idx));
    LogUrtContext("PlayEntryByIndex");
    if (!IsTrainingMatchActive()) {
        LOG(1, "[URT] PlayEntryByIndex aborted: training not active\n");
        PushToast("Replay takeover library only runs in training mode.");
        return false;
    }
    m_continuousSessionActive = false;
    return StartEntryByIndex(idx);
}

void UnlimitedReplayTakeoverManager::CancelActiveTakeover() {
    if (!m_runtimeSlotRestorePending && !m_setupPending && !m_continuousSessionActive && !m_lastPlaybackStartedByManager) {
        return;
    }

    m_continuousSessionActive = false;
    m_setupPending = false;

    if (m_playbackManager.playback_control_p) {
        m_playbackManager.set_playback_control(0);
    }

    if (m_runtimeSlotBackupValid) {
        RestoreRuntimeSlotNow();
    }
    m_runtimeSlotRestorePending = false;
    m_lastPlaybackStartedByManager = false;
    m_runtimePlaybackStartedAtMs = 0;
    m_runtimePlaybackNextWatchdogLogAtMs = 0;
    PushToast("Replay takeover cancelled.");
}

const std::vector<UnlimitedReplayTakeoverManager::ReplayTakeoverEntry>& UnlimitedReplayTakeoverManager::GetEntries() const {
    return m_entries;
}

std::vector<UnlimitedReplayTakeoverManager::ReplayTakeoverEntry>& UnlimitedReplayTakeoverManager::GetEntriesMutable() {
    return m_entries;
}

bool UnlimitedReplayTakeoverManager::RemoveEntryByIndex(size_t idx) {
    if (idx >= m_entries.size()) {
        return false;
    }
    const ReplayTakeoverEntry removed = m_entries[idx];
    m_cache.erase(removed.id);
    m_runtimeLiveSnapshotSlots.erase(removed.id);
    const std::string entryPath = ResolveEntryPath(removed);
    if (!entryPath.empty()) {
        DeleteFileA(entryPath.c_str());
    }
    m_entries.erase(m_entries.begin() + idx);
    PushToast("Replay takeover entry removed.");
    return true;
}

bool UnlimitedReplayTakeoverManager::RenameEntry(size_t idx, const std::string& newName) {
    if (idx >= m_entries.size() || newName.empty()) {
        return false;
    }
    m_entries[idx].name = newName;
    PushToast("Replay takeover entry renamed.");
    return true;
}

bool UnlimitedReplayTakeoverManager::SaveProfile(const std::string& profilePath) {
    std::string p = profilePath;
    if (!IsAbsolutePath(p)) {
        p = JoinPath(GetProfileFolder(), profilePath);
    }
    const size_t slash = p.find_last_of("/\\");
    if (slash != std::string::npos) {
        EnsureDirectoryRecursive(p.substr(0, slash));
    }

    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if (!out.good()) {
        PushToast("Failed to save replay takeover profile.");
        return false;
    }

    out << "format_kind=" << kProfileKind << "\n";
    out << "format_version=1.0\n";
    out << "selection_mode=" << m_selectionMode << "\n";
    out << "trigger_key=" << m_triggerKeyCode << "\n";
    out << "cancel_key=" << m_cancelKeyCode << "\n";
    out << "setup_delay_seconds=" << m_setupDelaySeconds << "\n";
    out << "continuous=" << (m_continuousPicking ? 1 : 0) << "\n";
    for (const auto& e : m_entries) {
        out << "entry="
            << e.id << "|"
            << e.name << "|"
            << e.relativePath << "|"
            << (e.enabled ? 1 : 0) << "|"
            << e.weight << "\n";
    }
    out.close();
    m_activeProfilePath = p;
    PushToast("Replay takeover profile saved.");
    return true;
}

bool UnlimitedReplayTakeoverManager::LoadProfile(const std::string& profilePath) {
    std::string p = profilePath;
    if (!IsAbsolutePath(p)) {
        p = JoinPath(GetProfileFolder(), profilePath);
    }

    std::ifstream in(p, std::ios::binary);
    if (!in.good()) {
        return false;
    }

    std::vector<ReplayTakeoverEntry> parsedEntries;
    int parsedSelectionMode = m_selectionMode;
    int parsedTriggerKey = m_triggerKeyCode;
    int parsedCancelKey = m_cancelKeyCode;
    float parsedSetupDelaySeconds = m_setupDelaySeconds;
    bool parsedContinuous = m_continuousPicking;

    std::string line;
    while (std::getline(in, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        const std::string key = Trim(line.substr(0, pos));
        const std::string value = Trim(line.substr(pos + 1));
        if (key == "format_kind" || key == "format_version") {
            continue;
        }
        if (key == "selection_mode") {
            parsedSelectionMode = std::atoi(value.c_str());
            continue;
        }
        if (key == "trigger_key") {
            parsedTriggerKey = std::atoi(value.c_str());
            continue;
        }
        if (key == "cancel_key") {
            parsedCancelKey = std::atoi(value.c_str());
            continue;
        }
        if (key == "setup_delay_seconds") {
            parsedSetupDelaySeconds = static_cast<float>(std::atof(value.c_str()));
            continue;
        }
        if (key == "continuous") {
            parsedContinuous = std::atoi(value.c_str()) != 0;
            continue;
        }
        if (key == "entry") {
            auto parts = Split(value, '|');
            if (parts.size() < 5) {
                continue;
            }

            ReplayTakeoverEntry e;
            e.id = parts[0];
            e.name = parts[1];
            e.relativePath = parts[2];
            e.enabled = std::atoi(parts[3].c_str()) != 0;
            e.weight = static_cast<float>(std::atof(parts[4].c_str()));
            if (e.id.empty()) {
                e.id = MakeEntryId();
            }
            if (e.weight <= 0.0f) {
                e.weight = 0.01f;
            }
            parsedEntries.push_back(e);
        }
    }

    m_entries = parsedEntries;
    m_selectionMode = (parsedSelectionMode >= Selection_Random && parsedSelectionMode <= Selection_NonRepeatingRandom)
        ? parsedSelectionMode : Selection_Random;
    m_triggerKeyCode = parsedTriggerKey;
    m_cancelKeyCode = parsedCancelKey;
    m_setupDelaySeconds = (std::max)(0.0f, parsedSetupDelaySeconds);
    m_continuousPicking = parsedContinuous;
    m_sequentialIndex = 0;
    m_nonRepeatPool.clear();

    const size_t slash = p.find_last_of("/\\");
    if (slash != std::string::npos) {
        m_lastLoadedProfileFolder = p.substr(0, slash);
    } else {
        m_lastLoadedProfileFolder.clear();
    }

    // Keep profile load lightweight; entry payloads are lazily loaded on demand.
    m_cache.clear();
    m_runtimeLiveSnapshotSlots.clear();

    m_activeProfilePath = p;
    PushToast("Replay takeover profile loaded.");
    return true;
}

std::string UnlimitedReplayTakeoverManager::GetActiveProfilePath() const {
    return m_activeProfilePath;
}

void UnlimitedReplayTakeoverManager::ClearAll() {
    m_entries.clear();
    m_cache.clear();
    m_runtimeLiveSnapshotSlots.clear();
    m_nonRepeatPool.clear();
    m_sequentialIndex = 0;
    PushToast("Replay takeover library cleared.");
}

int UnlimitedReplayTakeoverManager::GetSelectionMode() const { return m_selectionMode; }
void UnlimitedReplayTakeoverManager::SetSelectionMode(int mode) {
    if (mode < Selection_Random || mode > Selection_NonRepeatingRandom) {
        mode = Selection_Random;
    }
    m_selectionMode = mode;
    m_nonRepeatPool.clear();
    m_sequentialIndex = 0;
}
bool UnlimitedReplayTakeoverManager::GetContinuousPicking() const { return m_continuousPicking; }
void UnlimitedReplayTakeoverManager::SetContinuousPicking(bool enabled) { m_continuousPicking = enabled; }
float UnlimitedReplayTakeoverManager::GetSetupDelaySeconds() const { return m_setupDelaySeconds; }
void UnlimitedReplayTakeoverManager::SetSetupDelaySeconds(float seconds) { m_setupDelaySeconds = (std::max)(0.0f, seconds); }
bool UnlimitedReplayTakeoverManager::IsSetupDelayActive() const { return m_setupPending; }
float UnlimitedReplayTakeoverManager::GetSetupDelayRemainingSeconds() const {
    if (!m_setupPending) {
        return 0.0f;
    }
    const unsigned long long now = GetTickCount64();
    if (now >= m_setupUnfreezeAtMs) {
        return 0.0f;
    }
    const unsigned long long remainingMs = m_setupUnfreezeAtMs - now;
    return static_cast<float>(remainingMs) / 1000.0f;
}
int UnlimitedReplayTakeoverManager::GetTriggerKeyCode() const { return m_triggerKeyCode; }
void UnlimitedReplayTakeoverManager::SetTriggerKeyCode(int keyCode) { m_triggerKeyCode = keyCode; }
int UnlimitedReplayTakeoverManager::GetCancelKeyCode() const { return m_cancelKeyCode; }
void UnlimitedReplayTakeoverManager::SetCancelKeyCode(int keyCode) { m_cancelKeyCode = keyCode; }

std::string UnlimitedReplayTakeoverManager::GetStatusText() const { return m_statusText; }
const std::deque<UnlimitedReplayTakeoverManager::ToastMessage>& UnlimitedReplayTakeoverManager::GetToasts() const { return m_toasts; }

void UnlimitedReplayTakeoverManager::PruneExpiredToasts() {
    const unsigned long long now = GetTickCount64();
    while (!m_toasts.empty()) {
        const auto& t = m_toasts.front();
        if ((now - t.createdAtMs) <= t.durationMs) {
            break;
        }
        m_toasts.pop_front();
    }
}

void UnlimitedReplayTakeoverManager::PushToast(const std::string& text, unsigned long long durationMs) {
    ToastMessage t;
    t.text = text;
    t.createdAtMs = GetTickCount64();
    t.durationMs = durationMs;
    m_toasts.push_back(t);
    if (m_toasts.size() > 10) {
        m_toasts.pop_front();
    }
    m_statusText = text;
}

void UnlimitedReplayTakeoverManager::EnsureFolders() {
    EnsureDirectoryRecursive(GetLibraryFolder());
    EnsureDirectoryRecursive(GetProfileFolder());
}

std::string UnlimitedReplayTakeoverManager::GetLibraryFolder() const {
    return "BBCF_IM/unlimited_replay_takeover/library";
}

std::string UnlimitedReplayTakeoverManager::GetProfileFolder() const {
    return "BBCF_IM/unlimited_replay_takeover/profiles";
}

std::string UnlimitedReplayTakeoverManager::MakeEntryId() {
    ++m_entrySerial;
    return "rtk_" + std::to_string(static_cast<unsigned long long>(GetTickCount64())) + "_" + std::to_string(m_entrySerial);
}

std::string UnlimitedReplayTakeoverManager::SanitizeFileName(const std::string& input) const {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.') {
            out.push_back(c);
        } else if (c == ' ') {
            out.push_back('_');
        }
    }
    if (out.empty()) out = "entry";
    return out;
}

std::string UnlimitedReplayTakeoverManager::BuildUniqueRelativePath(const std::string& preferredName) const {
    std::string base = SanitizeFileName(preferredName);
    if (base.size() > 48) {
        base.resize(48);
    }
    int suffix = 0;
    while (true) {
        std::string candidate = base;
        if (suffix > 0) {
            candidate += "_" + std::to_string(suffix);
        }
        candidate += ".urte";
        const std::string full = JoinPath(GetLibraryFolder(), candidate);
        if (!PathExists(full)) {
            return candidate;
        }
        ++suffix;
    }
}

std::string UnlimitedReplayTakeoverManager::ResolveEntryPath(const ReplayTakeoverEntry& entry) const {
    if (IsAbsolutePath(entry.relativePath)) {
        return entry.relativePath;
    }

    const std::string lib = JoinPath(GetLibraryFolder(), entry.relativePath);
    if (PathExists(lib)) {
        return lib;
    }
    if (!m_lastLoadedProfileFolder.empty()) {
        const std::string profileRelative = JoinPath(m_lastLoadedProfileFolder, entry.relativePath);
        if (PathExists(profileRelative)) {
            return profileRelative;
        }
    }
    return lib;
}

bool UnlimitedReplayTakeoverManager::IsReplayMatchActive() const {
    return g_gameVals.pGameMode && g_gameVals.pGameState &&
        (*g_gameVals.pGameMode == GameMode_ReplayTheater) &&
        (*g_gameVals.pGameState == GameState_InMatch) &&
        !g_interfaces.player1.IsCharDataNullPtr() &&
        !g_interfaces.player2.IsCharDataNullPtr();
}

bool UnlimitedReplayTakeoverManager::IsTrainingMatchActive() const {
    return g_gameVals.pGameMode && g_gameVals.pGameState &&
        (*g_gameVals.pGameMode == GameMode_Training) &&
        (*g_gameVals.pGameState == GameState_InMatch) &&
        !g_interfaces.player1.IsCharDataNullPtr() &&
        !g_interfaces.player2.IsCharDataNullPtr();
}

bool UnlimitedReplayTakeoverManager::IsKeyPressedEdge(int virtualKey) const {
    if (virtualKey <= 0 || virtualKey >= 256) {
        return false;
    }
    return (GetAsyncKeyState(virtualKey) & 0x1) != 0;
}

bool UnlimitedReplayTakeoverManager::PickEntryIndex(size_t* outIndex) {
    std::vector<size_t> candidates;
    candidates.reserve(m_entries.size());
    for (size_t i = 0; i < m_entries.size(); ++i) {
        const auto& e = m_entries[i];
        if (e.enabled && e.weight > 0.0f) {
            candidates.push_back(i);
        }
    }
    if (candidates.empty()) {
        return false;
    }

    static std::mt19937 rng(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    if (m_selectionMode == Selection_Sequential) {
        const size_t pos = m_sequentialIndex % candidates.size();
        *outIndex = candidates[pos];
        m_sequentialIndex = (m_sequentialIndex + 1) % candidates.size();
        return true;
    }

    auto weightedPick = [&](const std::vector<size_t>& src, size_t* out) -> bool {
        double total = 0.0;
        for (size_t idx : src) total += m_entries[idx].weight;
        if (total <= 0.0) return false;
        std::uniform_real_distribution<double> dist(0.0, total);
        const double roll = dist(rng);
        double acc = 0.0;
        for (size_t idx : src) {
            acc += m_entries[idx].weight;
            if (roll <= acc) {
                *out = idx;
                return true;
            }
        }
        *out = src.back();
        return true;
    };

    if (m_selectionMode == Selection_NonRepeatingRandom) {
        if (m_nonRepeatPool.empty()) {
            m_nonRepeatPool = candidates;
        } else {
            std::vector<size_t> filtered;
            for (size_t idx : m_nonRepeatPool) {
                if (std::find(candidates.begin(), candidates.end(), idx) != candidates.end()) {
                    filtered.push_back(idx);
                }
            }
            m_nonRepeatPool.swap(filtered);
            if (m_nonRepeatPool.empty()) {
                m_nonRepeatPool = candidates;
            }
        }
        size_t picked = 0;
        if (!weightedPick(m_nonRepeatPool, &picked)) {
            return false;
        }
        *outIndex = picked;
        m_nonRepeatPool.erase(std::remove(m_nonRepeatPool.begin(), m_nonRepeatPool.end(), picked), m_nonRepeatPool.end());
        return true;
    }

    return weightedPick(candidates, outIndex);
}

bool UnlimitedReplayTakeoverManager::LoadEntryData(const ReplayTakeoverEntry& entry, LoadedEntryData* outData) {
    if (!outData) {
        LOG(1, "[URT] LoadEntryData aborted: outData null for id='%s'\n", entry.id.c_str());
        return false;
    }

    const std::string fullPath = ResolveEntryPath(entry);
    LOG(1, "[URT] LoadEntryData id='%s' path='%s'\n", entry.id.c_str(), fullPath.c_str());
    std::ifstream in(fullPath, std::ios::binary);
    if (!in.good()) {
        LOG(1, "[URT] LoadEntryData failed: file open failed\n");
        return false;
    }

    char magic[4] = {};
    in.read(magic, 4);
    if (!in.good() || std::strncmp(magic, kEntryMagic, 4) != 0) {
        LOG(1, "[URT] LoadEntryData failed: bad magic\n");
        return false;
    }

    unsigned char versionMajor = 0;
    unsigned char versionMinor = 0;
    unsigned char flags = 0;
    unsigned char reserved = 0;
    uint32_t snapshotSize = 0;
    uint32_t frameCount = 0;
    int32_t recordedP1CharIndex = -1;
    int32_t recordedP2CharIndex = -1;
    in.read(reinterpret_cast<char*>(&versionMajor), 1);
    in.read(reinterpret_cast<char*>(&versionMinor), 1);
    in.read(reinterpret_cast<char*>(&flags), 1);
    in.read(reinterpret_cast<char*>(&reserved), 1);
    in.read(reinterpret_cast<char*>(&snapshotSize), sizeof(snapshotSize));
    in.read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));
    if (versionMinor >= 1) {
        in.read(reinterpret_cast<char*>(&recordedP1CharIndex), sizeof(recordedP1CharIndex));
        in.read(reinterpret_cast<char*>(&recordedP2CharIndex), sizeof(recordedP2CharIndex));
    }
    if (!in.good()) {
        LOG(1, "[URT] LoadEntryData failed: header read failed\n");
        return false;
    }
    if (versionMajor != kEntryVersionMajor) {
        LOG(1, "[URT] LoadEntryData failed: version mismatch major=%u expected=%u\n",
            static_cast<unsigned int>(versionMajor),
            static_cast<unsigned int>(kEntryVersionMajor));
        return false;
    }

    if (snapshotSize == 0 || snapshotSize > static_cast<uint32_t>(kSnapshotBytesSize)) {
        LOG(1, "[URT] LoadEntryData failed: invalid snapshotSize=%u\n", snapshotSize);
        return false;
    }
    if (frameCount == 0 || frameCount > static_cast<uint32_t>(kMaxFramesPerEntry)) {
        LOG(1, "[URT] LoadEntryData failed: invalid frameCount=%u\n", frameCount);
        return false;
    }

    outData->snapshotBytes.resize(snapshotSize);
    outData->playbackFrames.resize(frameCount);
    in.read(outData->snapshotBytes.data(), static_cast<std::streamsize>(snapshotSize));
    if (in.gcount() != static_cast<std::streamsize>(snapshotSize)) {
        LOG(1, "[URT] LoadEntryData failed: snapshot payload truncated expected=%u got=%d\n",
            snapshotSize,
            static_cast<int>(in.gcount()));
        return false;
    }
    in.read(outData->playbackFrames.data(), static_cast<std::streamsize>(frameCount));
    if (in.gcount() != static_cast<std::streamsize>(frameCount)) {
        LOG(1, "[URT] LoadEntryData failed: playback payload truncated expected=%u got=%d\n",
            frameCount,
            static_cast<int>(in.gcount()));
        return false;
    }
    outData->facingLeft = (flags & 0x1) != 0;
    outData->loaded = true;
    outData->recordedP1CharIndex = recordedP1CharIndex;
    outData->recordedP2CharIndex = recordedP2CharIndex;
    LOG(1, "[URT] LoadEntryData success: snapshotSize=%u frameCount=%u facingLeft=%d\n",
        snapshotSize,
        frameCount,
        outData->facingLeft ? 1 : 0);
    LOG(1, "[URT] LoadEntryData metadata: version=1.%u p1Char=%d p2Char=%d\n",
        static_cast<unsigned int>(versionMinor),
        outData->recordedP1CharIndex,
        outData->recordedP2CharIndex);
    LogFramePreview("LoadEntryData", outData->playbackFrames);
    return true;
}

bool UnlimitedReplayTakeoverManager::SaveEntryData(const std::string& fullPath, const LoadedEntryData& data) {
    const size_t slash = fullPath.find_last_of("/\\");
    if (slash != std::string::npos) {
        EnsureDirectoryRecursive(fullPath.substr(0, slash));
    }

    std::ofstream out(fullPath, std::ios::binary | std::ios::trunc);
    if (!out.good()) {
        LOG(1, "[URT] SaveEntryData failed: could not open path='%s'\n", fullPath.c_str());
        return false;
    }

    const unsigned char flags = data.facingLeft ? 0x1 : 0x0;
    const unsigned char reserved = 0;
    const uint32_t snapshotSize = static_cast<uint32_t>(data.snapshotBytes.size());
    const uint32_t frameCount = static_cast<uint32_t>(data.playbackFrames.size());
    const int32_t recordedP1CharIndex = data.recordedP1CharIndex;
    const int32_t recordedP2CharIndex = data.recordedP2CharIndex;
    LOG(1, "[URT] SaveEntryData path='%s' snapshotSize=%u frameCount=%u facingLeft=%d\n",
        fullPath.c_str(),
        snapshotSize,
        frameCount,
        data.facingLeft ? 1 : 0);
    LOG(1, "[URT] SaveEntryData metadata p1Char=%d p2Char=%d\n",
        recordedP1CharIndex,
        recordedP2CharIndex);
    out.write(kEntryMagic, 4);
    out.write(reinterpret_cast<const char*>(&kEntryVersionMajor), 1);
    out.write(reinterpret_cast<const char*>(&kEntryVersionMinor), 1);
    out.write(reinterpret_cast<const char*>(&flags), 1);
    out.write(reinterpret_cast<const char*>(&reserved), 1);
    out.write(reinterpret_cast<const char*>(&snapshotSize), sizeof(snapshotSize));
    out.write(reinterpret_cast<const char*>(&frameCount), sizeof(frameCount));
    out.write(reinterpret_cast<const char*>(&recordedP1CharIndex), sizeof(recordedP1CharIndex));
    out.write(reinterpret_cast<const char*>(&recordedP2CharIndex), sizeof(recordedP2CharIndex));
    out.write(data.snapshotBytes.data(), static_cast<std::streamsize>(snapshotSize));
    out.write(data.playbackFrames.data(), static_cast<std::streamsize>(frameCount));
    const bool ok = out.good();
    LOG(1, "[URT] SaveEntryData result=%d\n", ok ? 1 : 0);
    return ok;
}

bool UnlimitedReplayTakeoverManager::StartEntryByIndex(size_t idx) {
    LOG(1, "[URT] StartEntryByIndex idx=%u\n", static_cast<unsigned int>(idx));
    LogUrtContext("StartEntryByIndex:begin");
    if (idx >= m_entries.size()) {
        LOG(1, "[URT] StartEntryByIndex aborted: invalid index\n");
        return false;
    }
    if (!IsTrainingMatchActive()) {
        LOG(1, "[URT] StartEntryByIndex aborted: training not active\n");
        return false;
    }

    const auto& entry = m_entries[idx];
    auto it = m_cache.find(entry.id);
    if (it == m_cache.end() || !it->second.loaded) {
        LoadedEntryData data;
        LOG(1, "[URT] Loading entry file: %s\n", entry.relativePath.c_str());
        if (!LoadEntryData(entry, &data)) {
            PushToast("Failed loading replay takeover entry.");
            LOG(1, "[URT] Failed loading entry file\n");
            return false;
        }
        m_cache[entry.id] = data;
        it = m_cache.find(entry.id);
    }
    if (it == m_cache.end() || !it->second.loaded) {
        return false;
    }

    if (it->second.snapshotBytes.empty()) {
        PushToast("Entry snapshot data missing.");
        LOG(1, "[URT] Entry snapshot data missing\n");
        return false;
    }
    if (it->second.playbackFrames.empty()) {
        PushToast("Entry playback data missing.");
        LOG(1, "[URT] Entry playback data missing\n");
        return false;
    }
    const int currentP1CharIndex = g_interfaces.player1.GetData() ? g_interfaces.player1.GetData()->charIndex : -1;
    const int currentP2CharIndex = g_interfaces.player2.GetData() ? g_interfaces.player2.GetData()->charIndex : -1;
    if (it->second.recordedP1CharIndex >= 0 && it->second.recordedP2CharIndex >= 0) {
        if (currentP1CharIndex != it->second.recordedP1CharIndex ||
            currentP2CharIndex != it->second.recordedP2CharIndex) {
            LOG(1,
                "[URT] StartEntryByIndex compatibility reject: recorded(p1=%d,p2=%d) current(p1=%d,p2=%d)\n",
                it->second.recordedP1CharIndex,
                it->second.recordedP2CharIndex,
                currentP1CharIndex,
                currentP2CharIndex);
            PushToast("Entry incompatible with current P1/P2 characters or side assignment.");
            return false;
        }
    }

    if (!TryEnsureTrainingSnapshotApparatus()) {
        PushToast("Failed to initialize training snapshot apparatus.");
        LOG(1, "[URT] Failed to initialize training snapshot apparatus\n");
        return false;
    }
    const int preMatchTimer = (g_gameVals.pMatchTimer != nullptr) ? *g_gameVals.pMatchTimer : -1;
    const int preMatchState = (g_gameVals.pMatchState != nullptr) ? *g_gameVals.pMatchState : -1;
    const int preRound = static_cast<int>(*(GetBbcfBaseAdress() + 0x11C034C));
    const size_t copySize = (std::min)(it->second.snapshotBytes.size(), kSnapshotBytesSize);
    LOG(1, "[URT] StartEntryByIndex entry id='%s' name='%s' snapshotBytes=%u playbackFrames=%u facingLeft=%d\n",
        entry.id.c_str(),
        entry.name.c_str(),
        static_cast<unsigned int>(it->second.snapshotBytes.size()),
        static_cast<unsigned int>(it->second.playbackFrames.size()),
        it->second.facingLeft ? 1 : 0);
    LogSnapshotApparatusState("StartEntryByIndex:training apparatus ready", m_trainingSnapshotApparatus);
    LogFramePreview("StartEntryByIndex", it->second.playbackFrames);
    if (copySize == 0) {
        PushToast("Snapshot load failed; entry snapshot is empty.");
        LOG(1, "[URT] Snapshot load failed: entry snapshot is empty\n");
        return false;
    }
    const uintptr_t currentSnapshotManagerAddr = reinterpret_cast<uintptr_t>(GetCurrentSnapshotManager());
    LOG(1,
        "[URT] Snapshot manager compare: recorded=0x%08X current=0x%08X\n",
        static_cast<unsigned int>(it->second.snapshotManagerAddress),
        static_cast<unsigned int>(currentSnapshotManagerAddr));

    bool snapshotLoaded = false;
    if (it->second.hasLiveSnapshotIndex && it->second.liveSnapshotIndex >= 0) {
        if (it->second.snapshotManagerAddress != 0 &&
            currentSnapshotManagerAddr != 0 &&
            it->second.snapshotManagerAddress == currentSnapshotManagerAddr) {
            LOG(1, "[URT] Attempting live snapshot slot load first. liveSlot=%d\n", it->second.liveSnapshotIndex);
            snapshotLoaded = SafeLoadSnapshotIndex(m_trainingSnapshotApparatus, it->second.liveSnapshotIndex);
            LOG(1, "[URT] Live snapshot slot load result=%d slot=%d\n",
                snapshotLoaded ? 1 : 0,
                it->second.liveSnapshotIndex);
        } else {
            LOG(1,
                "[URT] Skipping live slot load: snapshot manager mismatch/unknown (recorded=0x%08X current=0x%08X)\n",
                static_cast<unsigned int>(it->second.snapshotManagerAddress),
                static_cast<unsigned int>(currentSnapshotManagerAddr));
        }
    }

    if (!snapshotLoaded) {
        bool hardIncompatible = false;
        bool usedMismatchProbe = false;
        LOG(1, "[URT] Attempting seeded-slot serialized snapshot load first. entryBytes=%u copyBytes=%u\n",
            static_cast<unsigned int>(it->second.snapshotBytes.size()),
            static_cast<unsigned int>(copySize));
        snapshotLoaded = TryLoadSerializedSnapshotViaSeededSlot(
            m_trainingSnapshotApparatus,
            it->second.snapshotBytes.data(),
            copySize,
            kUrtDedicatedLiveSnapshotSlot,
            &hardIncompatible,
            &usedMismatchProbe);
        LOG(1, "[URT] Seeded-slot serialized snapshot load first result=%d\n", snapshotLoaded ? 1 : 0);
        if (snapshotLoaded && usedMismatchProbe) {
            LOG(1, "[URT] Seeded-slot load used mismatch probe; accepting and continuing with post-load sanity validation\n");
        }
        if (!snapshotLoaded && hardIncompatible) {
            PushToast("Snapshot incompatible with current training context (size mismatch).");
            LOG(1, "[URT] Aborting snapshot restore due to hard incompatibility\n");
            return false;
        }
    }

    if (!snapshotLoaded) {
        if (Settings::settingsIni.urtReAllowUnsafeProbeLoad) {
            LOG(1, "[URT] Seeded-slot load failed; attempting direct serialized snapshot load fallback (unsafe mode). entryBytes=%u copyBytes=%u\n",
                static_cast<unsigned int>(it->second.snapshotBytes.size()),
                static_cast<unsigned int>(copySize));
            std::memset(&m_runtimeLoadSnapshot, 0, sizeof(m_runtimeLoadSnapshot));
            std::memcpy(&m_runtimeLoadSnapshot, it->second.snapshotBytes.data(), copySize);
            snapshotLoaded = SafeLoadSnapshot(m_trainingSnapshotApparatus, &m_runtimeLoadSnapshot, copySize);
            LOG(1, "[URT] Direct serialized snapshot load fallback result=%d\n", snapshotLoaded ? 1 : 0);
        } else {
            LOG(1, "[URT] Seeded-slot load failed; direct serialized fallback skipped (unsafe mode disabled)\n");
        }
    }

    if (!snapshotLoaded) {
        PushToast("Snapshot load failed; playback cancelled.");
        LOG(1, "[URT] Snapshot load failed; playback cancelled (live+serialized paths)\n");
        return false;
    }
    LOG(1, "[URT] Snapshot load succeeded\n");
    LogUrtContext("StartEntryByIndex:after snapshot load");
    const bool repairedAny = TryRepairPostSnapshotCoreState(preRound, preMatchTimer, preMatchState);
    if (repairedAny) {
        LogUrtContext("StartEntryByIndex:after repair");
    }
    std::string postLoadSanityFailure;
    if (!ValidatePostSnapshotStateForPlayback(&postLoadSanityFailure)) {
        LOG(1, "[URT] Snapshot post-load sanity failed. reason='%s'\n", postLoadSanityFailure.c_str());
        if (TryRepairPostSnapshotCoreState(preRound, preMatchTimer, preMatchState)) {
            LogUrtContext("StartEntryByIndex:after repair");
            postLoadSanityFailure.clear();
            if (ValidatePostSnapshotStateForPlayback(&postLoadSanityFailure)) {
                LOG(1, "[URT] Snapshot post-load repair succeeded; proceeding\n");
            } else {
                LOG(1,
                    "[URT] Snapshot post-load repair failed; playback cancelled. reason='%s'\n",
                    postLoadSanityFailure.c_str());
                PushToast("Snapshot loaded but state was invalid; playback cancelled.");
                return false;
            }
        } else {
            LOG(1, "[URT] Snapshot post-load state not repaired; playback cancelled.\n");
            PushToast("Snapshot loaded but state was invalid; playback cancelled.");
            return false;
        }
    }

    BackupRuntimeSlotIfNeeded();
    LOG(1, "[URT] Runtime slot backup prepared\n");
    m_playbackManager.load_into_slot(it->second.playbackFrames, it->second.facingLeft ? 1 : 0, kRuntimeSlot);
    LOG(1, "[URT] Playback loaded into runtime slot. frameCount=%u\n", static_cast<unsigned int>(it->second.playbackFrames.size()));
    m_playbackManager.set_active_slot(kRuntimeSlot);
    m_playbackManager.set_playback_type(0);
    m_playbackManager.set_playback_position(0);
    m_runtimeSlotRestorePending = true;
    m_lastPlaybackStartedByManager = true;
    m_runtimePlaybackStartedAtMs = GetTickCount64();
    m_runtimePlaybackNextWatchdogLogAtMs = m_runtimePlaybackStartedAtMs;

    if (m_setupDelaySeconds > 0.0f) {
        m_setupPending = true;
        m_playbackManager.set_playback_control(0);
        const unsigned long long delayMs = static_cast<unsigned long long>(std::llround(m_setupDelaySeconds * 1000.0f));
        m_setupUnfreezeAtMs = GetTickCount64() + delayMs;
        LOG(1, "[URT] Setup delay armed seconds=%.3f delayMs=%llu startAt=%llu (playback held)\n",
            static_cast<double>(m_setupDelaySeconds),
            delayMs,
            m_setupUnfreezeAtMs);
    } else {
        m_setupPending = false;
        m_playbackManager.set_playback_control(3);
        LOG(1, "[URT] Playback started on runtime slot\n");
        LOG(1, "[URT] Setup delay disabled\n");
    }

    PushToast(std::string("Replay takeover loaded: ") + entry.name);
    return true;
}

void UnlimitedReplayTakeoverManager::BackupRuntimeSlotIfNeeded() {
    if (m_runtimeSlotBackupValid) {
        return;
    }
    PlaybackSlot slot(kRuntimeSlot);
    m_runtimeSlotBackupFrames = slot.get_slot_buffer();
    m_runtimeSlotBackupFacingLeft = slot.get_facing_direction() != 0;
    m_runtimeSlotBackupValid = true;
}

void UnlimitedReplayTakeoverManager::RestoreRuntimeSlotNow() {
    if (m_runtimeSlotBackupValid) {
        m_playbackManager.load_into_slot(m_runtimeSlotBackupFrames, m_runtimeSlotBackupFacingLeft ? 1 : 0, kRuntimeSlot);
        m_runtimeSlotBackupFrames.clear();
        m_runtimeSlotBackupValid = false;
    }
    m_runtimeSlotRestorePending = false;
    m_lastPlaybackStartedByManager = false;
    m_runtimePlaybackStartedAtMs = 0;
    m_runtimePlaybackNextWatchdogLogAtMs = 0;
}

bool UnlimitedReplayTakeoverManager::IsRuntimePlaybackActive() const {
    if (!m_playbackManager.playback_control_p) {
        return false;
    }
    short playbackControl = 0;
    std::memcpy(&playbackControl, m_playbackManager.playback_control_p, sizeof(playbackControl));
    return playbackControl == 3;
}

bool UnlimitedReplayTakeoverManager::TryEnsureCaptureSnapshotApparatus() {
    LOG(1, "[URT] TryEnsureCaptureSnapshotApparatus begin current=%p\n", m_captureSnapshotApparatus);
    if (!m_captureSnapshotApparatus) {
        m_captureSnapshotApparatus = new SnapshotApparatus();
        LOG(1, "[URT] TryEnsureCaptureSnapshotApparatus created new apparatus=%p\n", m_captureSnapshotApparatus);
    }
    const bool validNow = m_captureSnapshotApparatus &&
        m_captureSnapshotApparatus->check_if_valid(g_interfaces.player1.GetData(), g_interfaces.player2.GetData());
    if (!validNow) {
        LOG(1, "[URT] TryEnsureCaptureSnapshotApparatus apparatus invalid, recreating old=%p\n", m_captureSnapshotApparatus);
        delete m_captureSnapshotApparatus;
        m_captureSnapshotApparatus = new SnapshotApparatus();
    }
    LogSnapshotApparatusState("TryEnsureCaptureSnapshotApparatus:end", m_captureSnapshotApparatus);
    return m_captureSnapshotApparatus != nullptr;
}

bool UnlimitedReplayTakeoverManager::TryEnsureTrainingSnapshotApparatus() {
    LOG(1, "[URT] TryEnsureTrainingSnapshotApparatus begin current=%p\n", m_trainingSnapshotApparatus);
    if (!m_trainingSnapshotApparatus) {
        m_trainingSnapshotApparatus = new SnapshotApparatus();
        m_trainingSnapshotPrimed = false;
        LOG(1, "[URT] TryEnsureTrainingSnapshotApparatus created new apparatus=%p\n", m_trainingSnapshotApparatus);
    }
    const bool validNow = m_trainingSnapshotApparatus &&
        m_trainingSnapshotApparatus->check_if_valid(g_interfaces.player1.GetData(), g_interfaces.player2.GetData());
    if (!validNow) {
        LOG(1, "[URT] TryEnsureTrainingSnapshotApparatus apparatus invalid, recreating old=%p\n", m_trainingSnapshotApparatus);
        delete m_trainingSnapshotApparatus;
        m_trainingSnapshotApparatus = new SnapshotApparatus();
        m_trainingSnapshotPrimed = false;
    }
    LogSnapshotApparatusState("TryEnsureTrainingSnapshotApparatus:end", m_trainingSnapshotApparatus);
    return m_trainingSnapshotApparatus != nullptr;
}

bool UnlimitedReplayTakeoverManager::BuildPlaybackFramesFromReplayRange(
    int round,
    int startFrame,
    int endFrameExclusive,
    int recordedPlayer,
    std::vector<char>* outFrames) const {
    if (!outFrames || endFrameExclusive <= startFrame) {
        return false;
    }
    if (recordedPlayer < 0 || recordedPlayer > 1) {
        return false;
    }
    if (round < 0 || round > 2) {
        return false;
    }

    const int frameCount = (std::min)(endFrameExclusive - startFrame, kMaxFramesPerEntry);
    if (frameCount <= 0) {
        LOG(1, "[URT] BuildPlaybackFramesFromReplayRange failed: invalid frameCount=%d\n", frameCount);
        return false;
    }

    char* base = GetBbcfBaseAdress();
    char* replayBase = base + 0x115B470 + 0x8d4;
    char* playerBase = replayBase + (0x7080 * recordedPlayer) + (0xE100 * round);
    LOG(1, "[URT] BuildPlaybackFramesFromReplayRange round=%d start=%d end=%d player=%d frameCount=%d replayBase=%p playerBase=%p\n",
        round,
        startFrame,
        endFrameExclusive,
        recordedPlayer,
        frameCount,
        replayBase,
        playerBase);
    outFrames->clear();
    outFrames->reserve(static_cast<size_t>(frameCount));
    for (int i = 0; i < frameCount; ++i) {
        const int frame = startFrame + i;
        char* recordedInput = playerBase + frame * 2;
        outFrames->push_back(*recordedInput);
    }
    return true;
}
