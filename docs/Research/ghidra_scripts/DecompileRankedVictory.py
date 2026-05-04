# Ghidra headless Jython script.
# Finds functions that reference the ranked network struct (VA 0x00CF7958)
# to understand what sets state1=63/64 (end-of-set) vs 57-60 (mid-set),
# and whether there is a win-count or set-completion field nearby.
#
# Usage:
# analyzeHeadless <project_dir> BBCF -process BBCF.exe -noanalysis \
#   -scriptPath <script_dir> -postScript DecompileRankedVictory.py <report_path>

from java.io import File, PrintWriter
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

# VA of ranked network struct (RVA 0x008F7958, base 0x00400000)
RANKED_NET_VA   = 0x00CF7958   # state  (+0x00)
RANKED_NET_VA1  = 0x00CF795C   # state1 (+0x04)

# Scan for state1 constants we observed at the victory screen
STATE1_TARGETS = [57, 58, 59, 60, 63, 64]

# Probe a wider window of the struct to find adjacent fields
PROBE_RANGE = 0x80  # bytes past struct base

# Extra functions suspected to be the ranked lobby/victory state machine
# (guesses based on addresses seen in prior Ghidra work; script will also
# auto-discover via xrefs so these are just supplements)
EXTRA_FUNCTION_ADDRS = [
    0x004A0FE0,  # get_NetUserData
]


def collect_xref_functions(out, base_va, byte_range):
    """Return a set of function entry-point VA ints that reference [base_va, base_va+byte_range)."""
    found = {}
    for offset in range(0, byte_range, 4):
        target = toAddr(base_va + offset)
        refs = getReferencesTo(target)
        for ref in refs:
            from_addr = ref.getFromAddress()
            fn = getFunctionContaining(from_addr)
            if fn is None:
                continue
            ep = fn.getEntryPoint().getOffset()
            if ep not in found:
                found[ep] = fn
                out.printf("[xref] struct+0x%02X  from %s  fn=%s @ %s%n",
                           offset, from_addr, fn.getName(), fn.getEntryPoint())
    return found


def collect_constant_functions(out, constants):
    """Return functions containing instructions with any of the given immediate constants."""
    found = {}
    listing = currentProgram.getListing()
    inst_iter = listing.getInstructions(
        currentProgram.getMemory().getAllInitializedAddressSet(), True)
    for inst in inst_iter:
        for op_index in range(inst.getNumOperands()):
            objs = inst.getOpObjects(op_index)
            for obj in objs:
                try:
                    value = int(str(obj).rstrip("h"), 16) if str(obj).endswith("h") else obj.getValue()
                except Exception:
                    continue
                if value in constants:
                    fn = getFunctionContaining(inst.getAddress())
                    if fn is None:
                        continue
                    ep = fn.getEntryPoint().getOffset()
                    if ep not in found:
                        found[ep] = fn
                        out.printf("[const %d] at %s  fn=%s @ %s%n",
                                   value, inst.getAddress(), fn.getName(), fn.getEntryPoint())
    return found


def decompile_functions(out, fn_map):
    ifc = DecompInterface()
    if not ifc.openProgram(currentProgram):
        raise Exception("openProgram failed")
    try:
        for ep, fn in sorted(fn_map.items()):
            out.printf("%n===== DECOMPILE %s  %s =====%n", fn.getEntryPoint(), fn.getName())
            result = ifc.decompileFunction(fn, 120, ConsoleTaskMonitor())
            if result.decompileCompleted():
                out.println(result.getDecompiledFunction().getC())
            else:
                out.printf("(decompile failed: %s)%n", result.getErrorMessage())
    finally:
        ifc.dispose()


args = getScriptArgs()
report_path = args[0] if len(args) > 0 else None
if report_path is None:
    exe_dir = File(currentProgram.getExecutablePath()).getParentFile()
    report_path = File(exe_dir, "ranked_victory_decompile.txt").getAbsolutePath()

out = PrintWriter(File(report_path), "UTF-8")
try:
    out.printf("Program : %s%n", currentProgram.getName())
    out.printf("ImageBase: %s%n%n", currentProgram.getImageBase())

    out.println("=== XREF SCAN (ranked network struct +-0x80) ===")
    xref_fns = collect_xref_functions(out, RANKED_NET_VA, PROBE_RANGE)

    out.println("%n=== CONSTANT SCAN (state1 values 57/58/59/60/63/64) ===")
    const_fns = collect_constant_functions(out, set(STATE1_TARGETS))

    # Add extra manually-listed functions
    for va in EXTRA_FUNCTION_ADDRS:
        fn = getFunctionAt(toAddr(va))
        if fn is None:
            fn = getFunctionContaining(toAddr(va))
        if fn is not None:
            ep = fn.getEntryPoint().getOffset()
            if ep not in xref_fns and ep not in const_fns:
                const_fns[ep] = fn

    all_fns = {}
    all_fns.update(xref_fns)
    all_fns.update(const_fns)

    out.printf("%n=== DECOMPILING %d FUNCTIONS ===%n", len(all_fns))
    decompile_functions(out, all_fns)
finally:
    out.close()

print("Wrote %s" % report_path)
