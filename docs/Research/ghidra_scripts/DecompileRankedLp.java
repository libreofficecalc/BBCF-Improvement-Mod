// Ghidra headless script. Decompiles ranked-LP functions and dumps the LP bounds table.
// Usage from repo root:
// /mnt/d/Programs/ghidra_11.4.2_PUBLIC/support/analyzeHeadless docs/Research/BBCF-Ghidra-Project BBCF -process BBCF.exe -scriptPath docs/Research/ghidra_scripts -postScript DecompileRankedLp.java

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.Memory;
import ghidra.util.task.ConsoleTaskMonitor;

public class DecompileRankedLp extends GhidraScript {
	private static final long[] FUNCTION_ADDRS = {
		0x004BDDF0L,
		0x004BE320L,
		0x004BE700L,
		0x004BE730L
	};

	private static final long BOUNDS_TABLE = 0x009DFFD0L;
	private static final int BOUNDS_ROWS = 40;

	@Override
	public void run() throws Exception {
		File outDir = new File(currentProgram.getExecutablePath()).getParentFile();
		File reportFile = new File(outDir, "ranked_lp_decompile.txt");
		if (getScriptArgs().length > 0) {
			reportFile = new File(getScriptArgs()[0]);
		}

		try (PrintWriter out = new PrintWriter(reportFile, "UTF-8")) {
			out.printf("Program: %s%n", currentProgram.getName());
			out.printf("Image base: %s%n%n", currentProgram.getImageBase());
			dumpFunctions(out);
			dumpBoundsTable(out);
		}

		printf("Wrote %s%n", reportFile.getAbsolutePath());
	}

	private void dumpFunctions(PrintWriter out) throws Exception {
		DecompInterface ifc = new DecompInterface();
		if (!ifc.openProgram(currentProgram)) {
			throw new IllegalStateException("openProgram failed");
		}

		for (long addrValue : FUNCTION_ADDRS) {
			Address addr = toAddr(addrValue);
			Function fn = getFunctionAt(addr);
			if (fn == null) {
				fn = getFunctionContaining(addr);
			}

			out.printf("===== %s =====%n", addr);
			if (fn == null) {
				out.println("No function found.");
				out.println();
				continue;
			}

			out.printf("Function: %s%n", fn.getName());
			DecompileResults result = ifc.decompileFunction(fn, 90, new ConsoleTaskMonitor());
			if (result.decompileCompleted()) {
				out.println(result.getDecompiledFunction().getC());
			}
			else {
				out.printf("Decompile failed: %s%n", result.getErrorMessage());
			}
			out.println();
		}

		ifc.dispose();
	}

	private void dumpBoundsTable(PrintWriter out) throws Exception {
		Memory memory = currentProgram.getMemory();
		out.println("===== DAT_009DFFD0 ranked LP bounds table =====");
		out.println("rank  upperOff  lowerOff  unk4  counter  upper  lower");

		for (int rank = 0; rank < BOUNDS_ROWS; ++rank) {
			Address row = toAddr(BOUNDS_TABLE + rank * 8L);
			short upperOff = memory.getShort(row);
			short lowerOff = memory.getShort(row.add(2));
			short unk4 = memory.getShort(row.add(4));
			short counter = memory.getShort(row.add(6));
			int upper = 0x7FFF + upperOff;
			int lower = 0x7FFF + lowerOff;
			out.printf("%-5d %-9d %-9d %-5d %-8d %-6d %-6d%n",
				rank, (int) upperOff, (int) lowerOff, (int) unk4, (int) counter, upper, lower);
		}
		out.println();
	}
}
