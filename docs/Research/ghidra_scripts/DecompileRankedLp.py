# Ghidra headless Jython script. Decompiles ranked-LP functions and dumps bounds table.
# Usage after import:
# analyzeHeadless <project_dir> BBCF -process BBCF.exe -scriptPath <script_dir> -postScript DecompileRankedLp.py <report_path>

from java.io import File, PrintWriter
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

FUNCTION_ADDRS = [
    0x004BDDF0,
    0x004BE320,
    0x004BE700,
    0x004BE730,
]

BOUNDS_TABLE = 0x009DFFD0
BOUNDS_ROWS = 40


def s16(value):
    value = value & 0xffff
    if value >= 0x8000:
        value -= 0x10000
    return value


def dump_functions(out):
    ifc = DecompInterface()
    if not ifc.openProgram(currentProgram):
        raise Exception("openProgram failed")

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

        out.printf("Function: %s%n", fn.getName())
        result = ifc.decompileFunction(fn, 90, ConsoleTaskMonitor())
        if result.decompileCompleted():
            out.println(result.getDecompiledFunction().getC())
        else:
            out.printf("Decompile failed: %s%n", result.getErrorMessage())
        out.println()

    ifc.dispose()


def dump_bounds_table(out):
    memory = currentProgram.getMemory()
    out.println("===== DAT_009DFFD0 ranked LP bounds table =====")
    out.println("rank  upperOff  lowerOff  unk4  counter  upper  lower")

    for rank in range(BOUNDS_ROWS):
        row = toAddr(BOUNDS_TABLE + rank * 8)
        upper_off = s16(memory.getShort(row))
        lower_off = s16(memory.getShort(row.add(2)))
        unk4 = s16(memory.getShort(row.add(4)))
        counter = s16(memory.getShort(row.add(6)))
        upper = 0x7fff + upper_off
        lower = 0x7fff + lower_off
        out.printf("%-5d %-9d %-9d %-5d %-8d %-6d %-6d%n",
                   rank, upper_off, lower_off, unk4, counter, upper, lower)
    out.println()


args = getScriptArgs()
if len(args) > 0:
    report_file = File(args[0])
else:
    report_file = File(File(currentProgram.getExecutablePath()).getParentFile(), "ranked_lp_decompile.txt")

out = PrintWriter(report_file, "UTF-8")
try:
    out.printf("Program: %s%n", currentProgram.getName())
    out.printf("Image base: %s%n%n", currentProgram.getImageBase())
    dump_functions(out)
    dump_bounds_table(out)
finally:
    out.close()

print("Wrote %s" % report_file.getAbsolutePath())
