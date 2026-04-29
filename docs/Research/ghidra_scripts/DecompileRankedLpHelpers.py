# Ghidra headless Jython script. Dumps ranked LP helper functions around win/loss deltas.

from java.io import File, PrintWriter
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

FUNCTION_ADDRS = [
    0x004BDC90,
    0x004BDCA0,
    0x004BDCD0,
    0x004BDD30,
    0x004BDD60,
    0x004BDDF0,
    0x004BDE70,
    0x004BDFB0,
    0x004BE1D0,
    0x004BE280,
    0x004BE4B0,
]

args = getScriptArgs()
if len(args) > 0:
    report_file = File(args[0])
else:
    report_file = File(File(currentProgram.getExecutablePath()).getParentFile(), "ranked_lp_helpers.txt")

ifc = DecompInterface()
if not ifc.openProgram(currentProgram):
    raise Exception("openProgram failed")

out = PrintWriter(report_file, "UTF-8")
try:
    out.printf("Program: %s%n", currentProgram.getName())
    out.printf("Image base: %s%n%n", currentProgram.getImageBase())
    for addr_value in FUNCTION_ADDRS:
        addr = toAddr(addr_value)
        fn = getFunctionAt(addr)
        if fn is None:
            fn = getFunctionContaining(addr)
        out.printf("===== %s =====%n", addr)
        if fn is None:
            out.println("No function found.")
            out.println()
            continue
        out.printf("Function: %s at %s%n", fn.getName(), fn.getEntryPoint())
        result = ifc.decompileFunction(fn, 90, ConsoleTaskMonitor())
        if result.decompileCompleted():
            out.println(result.getDecompiledFunction().getC())
        else:
            out.printf("Decompile failed: %s%n", result.getErrorMessage())
        out.println()
finally:
    out.close()
    ifc.dispose()

print("Wrote %s" % report_file.getAbsolutePath())
