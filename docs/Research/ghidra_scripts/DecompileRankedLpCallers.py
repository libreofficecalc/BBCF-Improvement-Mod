# Ghidra headless Jython script. Dumps callers/references for ranked LP functions.
# Usage:
# analyzeHeadless <project_dir> BBCF -process BBCF.exe -scriptPath <script_dir> -postScript DecompileRankedLpCallers.py <report_path>

from java.io import File, PrintWriter
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

TARGET_ADDRS = [
    0x004BDDF0,
    0x004BE320,
    0x004BE700,
    0x004BE730,
]


def decompile_function(ifc, out, fn):
    out.printf("Function: %s at %s%n", fn.getName(), fn.getEntryPoint())
    result = ifc.decompileFunction(fn, 90, ConsoleTaskMonitor())
    if result.decompileCompleted():
        out.println(result.getDecompiledFunction().getC())
    else:
        out.printf("Decompile failed: %s%n", result.getErrorMessage())
    out.println()


def collect_callers(target):
    callers = []
    refs = getReferencesTo(target)
    for ref in refs:
        from_addr = ref.getFromAddress()
        fn = getFunctionContaining(from_addr)
        callers.append((from_addr, ref.getReferenceType().toString(), fn))
    return callers


args = getScriptArgs()
if len(args) > 0:
    report_file = File(args[0])
else:
    report_file = File(File(currentProgram.getExecutablePath()).getParentFile(), "ranked_lp_callers.txt")

ifc = DecompInterface()
if not ifc.openProgram(currentProgram):
    raise Exception("openProgram failed")

out = PrintWriter(report_file, "UTF-8")
try:
    out.printf("Program: %s%n", currentProgram.getName())
    out.printf("Image base: %s%n%n", currentProgram.getImageBase())

    seen = set()
    for target_value in TARGET_ADDRS:
        target = toAddr(target_value)
        out.printf("===== References to %s =====%n", target)
        callers = collect_callers(target)
        for from_addr, ref_type, fn in callers:
            if fn is None:
                out.printf("%s %-16s <no function>%n", from_addr, ref_type)
            else:
                out.printf("%s %-16s %s at %s%n", from_addr, ref_type, fn.getName(), fn.getEntryPoint())
        out.println()

        for from_addr, ref_type, fn in callers:
            if fn is None:
                continue
            key = fn.getEntryPoint().toString()
            if key in seen:
                continue
            seen.add(key)
            out.printf("===== Caller decompile for target %s via %s =====%n", target, from_addr)
            decompile_function(ifc, out, fn)
finally:
    out.close()
    ifc.dispose()

print("Wrote %s" % report_file.getAbsolutePath())
