# Ghidra headless Jython script. Decompiles ranked confirmation UI / netUserData users.
# Usage after import:
# analyzeHeadless <project_dir> BBCF -process BBCF.exe -noanalysis -scriptPath <script_dir> -postScript DecompileRankedConfirm.py <report_path>

from java.io import File, PrintWriter
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor


FUNCTION_ADDRS = [
    0x004A9DE0,  # ranked confirmation renderer from prior disasm
    0x004A80F0,  # nearby UI path reads +0x6A69/+0x6A79
    0x004A11C0,  # helper used to convert slot byte to displayed rank-ish value
    0x004A1430,  # confirm helper feeding 004A11C0
    0x004A1470,  # confirm helper
    0x004A1490,  # confirm helper
    0x004A17E0,  # confirm helper
    0x004A1C20,  # confirm display helper, args include member +0x58/+0x5A
    0x004A1C60,  # confirm display helper, args include member +0x58/+0x5A
    0x004A1CA0,  # confirm display helper, args include member +0x59/+0x58/+0x5A
    0x004A27D0,  # confirm helper fed by member +0x5A
    0x004A2870,  # confirm helper after 004A27D0
    0x004BDFB0,  # rank-delta helper fed by member +0x5A
    0x004BDFE0,  # row rank getter
    0x004BDFF0,  # row packed getter/helper
    0x004BE320,  # loss update, opponent rank arg
    0x004BE4B0,  # win update, opponent rank arg
    0x004B3CD0,  # confirm accept/setup helper
    0x004B48C0,  # confirm accept/setup helper
    0x004B48F0,  # confirm accept/setup helper
    0x004B4920,  # confirm accept/setup helper
    0x0049D560,  # lookup helper used before confirm display draw
    0x0049D5C0,  # lookup helper used before confirm display draw
    0x0049A4C0,  # netUserData + ecx*8 + 0x6A69 user
    0x0060F2B0,  # netUserData + ecx*8 + 0x6A69 user
    0x0061BD00,  # direct +0x6A69/+0x6A79 user
    0x0061BFF0,  # direct +0x6A69/+0x6A79 user
]

INTEREST_ADDRS = [
    0x004A9F06,
    0x004A9F0D,
    0x004A9F1C,
    0x004A9F23,
    0x004AA017,
    0x004AA078,
    0x004AA091,
    0x004AA09A,
    0x004AA40F,
    0x0049A4ED,
    0x0060F2D4,
    0x0061BD2F,
    0x0061BD4A,
    0x0061C006,
    0x0061C00D,
]

OFFSETS = [
    0x68D0,
    0x68D1,
    0x68F8,
    0x68F9,
    0x6920,
    0x6921,
    0x6948,
    0x6949,
    0x6A69,
    0x6A79,
]


def get_fn(addr_value):
    addr = toAddr(addr_value)
    fn = getFunctionAt(addr)
    if fn is None:
        fn = getFunctionContaining(addr)
    return fn


def decompile_functions(out):
    ifc = DecompInterface()
    if not ifc.openProgram(currentProgram):
        raise Exception("openProgram failed")

    seen = set()
    for addr_value in FUNCTION_ADDRS + INTEREST_ADDRS:
        fn = get_fn(addr_value)
        if fn is None:
            out.printf("===== %s =====%nNo function found.%n%n", toAddr(addr_value))
            continue
        key = fn.getEntryPoint().toString()
        if key in seen:
            continue
        seen.add(key)

        out.printf("===== DECOMPILE %s containing %s =====%n", fn.getEntryPoint(), toAddr(addr_value))
        out.printf("Function: %s%n", fn.getName())
        result = ifc.decompileFunction(fn, 120, ConsoleTaskMonitor())
        if result.decompileCompleted():
            out.println(result.getDecompiledFunction().getC())
        else:
            out.printf("Decompile failed: %s%n", result.getErrorMessage())
        out.println()

    ifc.dispose()


def dump_listing_for_function(out, fn):
    listing = currentProgram.getListing()
    body = fn.getBody()
    inst_iter = listing.getInstructions(body, True)
    for inst in inst_iter:
        out.printf("%s: %s%n", inst.getAddress(), inst.toString())
    out.println()


def dump_interest_listings(out):
    seen = set()
    out.println("===== DISASSEMBLY FOR INTEREST FUNCTIONS =====")
    for addr_value in FUNCTION_ADDRS + INTEREST_ADDRS:
        fn = get_fn(addr_value)
        if fn is None:
            continue
        key = fn.getEntryPoint().toString()
        if key in seen:
            continue
        seen.add(key)
        out.printf("----- %s %s ----- %n", fn.getEntryPoint(), fn.getName())
        dump_listing_for_function(out, fn)


def dump_operand_constant_hits(out):
    listing = currentProgram.getListing()
    out.println("===== OPERAND CONSTANT HITS =====")
    for off in OFFSETS:
        out.printf("----- constant 0x%04X ----- %n", off)
        count = 0
        inst_iter = listing.getInstructions(currentProgram.getMemory().getAllInitializedAddressSet(), True)
        for inst in inst_iter:
            for op_index in range(inst.getNumOperands()):
                objs = inst.getOpObjects(op_index)
                for obj in objs:
                    try:
                        value = obj.getValue()
                    except:
                        try:
                            value = int(str(obj).rstrip("h"), 16)
                        except:
                            continue
                    if value == off:
                        fn = getFunctionContaining(inst.getAddress())
                        fn_name = fn.getName() if fn else "<none>"
                        out.printf("%s %s fn=%s%n", inst.getAddress(), inst.toString(), fn_name)
                        count += 1
                        break
            if count >= 200:
                out.println("(truncated)")
                break
        if count == 0:
            out.println("(none)")
        out.println()


def dump_calls_to_netuserdata(out):
    target = toAddr(0x004A0FE0)
    refs = getReferencesTo(target)
    out.println("===== REFERENCES TO get_NetUserData 004A0FE0 =====")
    for ref in refs:
        from_addr = ref.getFromAddress()
        fn = getFunctionContaining(from_addr)
        fn_name = fn.getName() if fn else "<none>"
        out.printf("%s type=%s fn=%s%n", from_addr, ref.getReferenceType(), fn_name)
    out.println()


args = getScriptArgs()
if len(args) > 0:
    report_file = File(args[0])
else:
    report_file = File(File(currentProgram.getExecutablePath()).getParentFile(), "ranked_confirm_decompile.txt")

out = PrintWriter(report_file, "UTF-8")
try:
    out.printf("Program: %s%n", currentProgram.getName())
    out.printf("Image base: %s%n%n", currentProgram.getImageBase())
    dump_calls_to_netuserdata(out)
    dump_operand_constant_hits(out)
    decompile_functions(out)
    dump_interest_listings(out)
finally:
    out.close()

print("Wrote %s" % report_file.getAbsolutePath())
