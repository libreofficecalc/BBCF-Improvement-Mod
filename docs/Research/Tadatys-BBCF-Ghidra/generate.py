#
# Scripts that automatically generate or modify many symbols
#
# Intended to be run one at a time, in the Jython console, not all at once
#

from utils import *


# process vftables
func_to_tab = defaultdict(list)

for s in currentProgram.getSymbolTable().getAllSymbols(True):
    if s.name in ("vftable", "vftable_1"): 
        tp, ns = find_tp_ns(ns=s.parentNamespace)
        if not tp:
            continue
        d = getDataAt(s.address)
        print s.parentNamespace, s.name, d.numComponents, s.address
        for i in range(d.numComponents):
            f = getFunctionAt(toAddr(d.getInt(i*4)))
            # XXX: it's good to add namespaces on first run, but when they're already added, it's even better not to add them on very short or empty functions
            #if f.parentNamespace.name == "Global":
            #    f.parentNamespace = s.parentNamespace
            if f.parentNamespace == ns:
                try: # XXX: some messed up functions give duplicate name errors
                    set_method_type(f, tp,
                        check_return="ctor" in f.name or "dtor" in f.name,
                        base_ptr=True)
                except:
                    print "x"
            func_to_tab[f].append((s, i))

# comment functions with vftables using them
for f, tabs in func_to_tab.items():
    txt = "vftables: " + ", ".join(s.parentNamespace.name + "::{@symbol " + str(s.address) + "}[" + str(idx) + "]" for s, idx in set(tabs)) # why would there be duplicates?
    a = f.entryPoint
    txt0 = getPlateComment(a) or ""
    if txt0:
        txt0 = txt0.split("\n")
        if txt0[-1].startswith("vftables: "):
            _ = txt0.pop()
        else:
            txt0.append("")
        txt = "\n".join(txt0) + "\n" + txt
    _ = setPlateComment(a, txt)

# set method type for any constructors in namespace
all_namespaces = Counter(s.parentNamespace for s in currentProgram.symbolTable.getAllSymbols(True))
for ns in all_namespaces:
    tp = find_tp_ns(ns=ns)[0]
    if not tp or not ns:
        continue
    for f in currentProgram.getSymbolTable().getSymbols(ns):
        if f.symbolType != symbol.SymbolType.FUNCTION:
            continue
        f = f.getObject()
        if "ctor" in f.name or "dtor" in f.name:
            if len(f.parameters) > 0:
                set_method_type(f, tp, check_return=True, base_ptr=True)


# add method-like functions to their type's namespaces
# XXX: this makes a mess if some functions have bad types!
# TODO: maybe also remove functions from namespace, once the type is 
for f in currentProgram.functionManager.getFunctions(True):
    tp = f.parameters[0] if len(f.parameters) > 0 else None
    tp = base_type(tp)
    ns = find_tp_ns(tp)[1]
    
    if not hasattr(tp, "components") or not is_bbcf_type(tp):
        continue
    if f.parentNamespace == ns:
        continue # no change
    if not f.symbol.parentNamespace.name == "Global":
        continue
    if not ns:
        continue
    
    print "x" if not ns else " ", f.entryPoint, f.parentNamespace, "->", ns or tp.name, f.signature
    #if ns:
    #    f.parentNamespace = ns


# check for strange function signatures
for f in currentProgram.functionManager.getFunctions(True):
    if any(is_bbcf_type(p.dataType) for p in f.parameters[1:]):
        
        print f.entryPoint, f.parentNamespace, f.signature
        
        if False: #if "__return_storage_ptr__" in str(f.signature):
            # all the cases with __return_storage_ptr__ were nonsense virtual methods
            
            tp, ns = find_tp_ns(ns=f.parentNamespace)
            p1tp = data.PointerDataType(base_type(tp), 4)
            p1 = listing.ParameterImpl(type_abbr(tp) + "_1", p1tp, currentProgram.getRegister("ECX"), currentProgram, symbol.SourceType.IMPORTED)
            
            r1 = listing.ParameterImpl("res", getDataTypes("undefined4")[0], currentProgram.getRegister("EAX"), currentProgram, symbol.SourceType.IMPORTED)
            
            f.updateFunction(None, r1,
                [p1],
                f.FunctionUpdateType.CUSTOM_STORAGE,
                False, symbol.SourceType.IMPORTED)



# check for names that have namespaces stuck in them
for f in currentProgram.functionManager.getFunctions(True):
    if f.symbol.source != symbol.SourceType.ANALYSIS:
        continue
    if f.name.startswith("_eval_") or f.name.startswith("__") or "<" in f.name:
        continue
    if "::" in f.name:
        p = f.name.split("::")
        print "setName", f.name, p[-2], p[-1]
        ns = getNamespace(None, p[-2])
        if not ns or len(p) != 2:
            print ("-- bad namespace")
            continue
        #f.symbol.setNamespace(ns)
        #f.setName(p[-1], symbol.SourceType.IMPORTED)
        continue


# rename known methods
def set_vftable_default_names(tab, ls):
    s = tab
    d = getDataAt(s.address)
    print s.parentNamespace, s.name, d.numComponents, s.address
    for i in range(min(d.numComponents, len(ls))):
        f = getFunctionAt(toAddr(d.getInt(i*4)))
        if f.parentNamespace == s.parentNamespace:
            if f.name != ls[i]:
                print f, "->", ls[i]
                if f.name.startswith("FUN_") or f.name.startswith("_eval_"):
                    f.setName(ls[i], symbol.SourceType.IMPORTED)

all_namespaces = Counter(s.parentNamespace for s in currentProgram.symbolTable.getAllSymbols(True))
for ns in all_namespaces:
    tab = getSymbols("vftable", ns)
    tab = tab[0] if len(tab) > 0 else None
    tp = tab and find_tp_ns(ns=ns)[0]
    if not tp or not tab:
        continue
    
    if ns.name.endswith("Factory"):
        d = getDataAt(tab.address)
        if d.numComponents == 1:
            set_vftable_default_names(tab, ["_create"])
        else:
            print "- big factory", ns.name, tab.address, d.numComponents
    else:
        if ns.name.startswith("SCENE_"):
            set_vftable_default_names(tab, ["_dtor", "_update_task", "_init_3", "_init_7", "_init_8_1", "_update_scene", "_set_next_scene", "_init_8_2", "_init_4", "_init_5", "_init_6", "_close_11", "_init_2"])
        
        if ns.name.endswith("Task") or ns.name.endswith("Controller"):
            set_vftable_default_names(tab, ["_dtor", "_update_task"])


# for each address in static initializer table, put its symbol in a namespace
ns = getNamespace(None, "static") or createNamespace(None, "static")
d = getDataAt(toAddr(0x44a660 + 4))
for i in range(100000):
    m = d.getInt(i*4)
    if m == 0:
        break
    s = getSymbolAt(toAddr(m))
    if s:
        s.setNamespace(ns)






# generate names for common short functions

fs = list(currentProgram.functionManager.getFunctions(True))
fs = [(f, fs[i+1].entryPoint.offset - f.entryPoint.offset, f.symbol.referenceCount) for i, f in enumerate(fs[:-1])]
fs = [(f, l, r) for f, l, r in fs if l < 100]
fs = sorted((p for p in fs if is_unnamed(p[0])), key=lambda p: p[1]-p[2]*2)
print len(fs)

_decompile_cache = {}
def minify_func(f):
    res = _decompile_cache.get(f)
    if not res:
        res = _decompile_cache[f] = decompile(f)
    code = res.decompiledFunction.c
    code = re.sub(r"/\*[^\*]*\*/", "", code) # drop comments
    code = re.sub(r"//[^\n]*\n", "\r\n", code)
    code = code[code.index("{")+1:code.rindex("}")] # drop signature and outer {}s
    code = [ln.strip() for ln in code.strip().split("\r\n") if ln.strip() and not ln.strip().startswith("/*")] # drop empty lines
    if code[-1] == "return;":
        code = code[:-1]
    
    vars = [] # collect local variables
    for i, p in enumerate(f.parameters):
        vars.append((p.name, "xyz"[i] if i < 3 else "x"+str(i)))
    
    vi = 0
    for vi in range(len(code)-1):
        m = re.match("[^\s]*\s+\*?([a-zA-Z0-9_]+)\s*(\[.*\])?;", code[vi])
        if not m:
            break
        vars.append((m.group(1), "vwu"[vi] if vi < 3 else "v"+str(vi)))
    
    code = code[vi:]
    code = "".join(code)
    code = re.sub(r"return|\s+|\([a-zA-Z0-9_]+\s*\*+\)|\(int\)|;$", "", code) # drop returns and type casts
    
    for l, r in vars: # shorten local vars. # TODO make sure not to remove fields: .param, ->param
        code = re.sub(l, r, code)
    
    # TODO: re.sub(r"\(*([a-zA-Z0-9_]+\+(0x[0-9a-fA-F]|[0-9]))\)\(", r"(\1.vf[\2])(" code) # divide the offset by 4 and turn to decimat, to make virtual calls easier to find
    code = re.sub("{","(", re.sub("}", ")", code)) # {}s in names would make parsing harder
    return "_eval_" + code

Counter("F" if f.name.startswith("FUN_") else "e" if f.name.startswith("_eval_") else "?" for f, _, _ in fs) 

for f, l, r in fs[:50]:
#for f, l, r in fs:
    nm = minify_func(f)
    print "=" if nm == f.name else "x" if not nm else "L" if len(nm)>100 else " ", f.entryPoint, r, l, f
    if nm and not nm == f.name and not len(nm)>100:
        print "->", nm
        f.setName(nm, ghidra.program.model.symbol.SourceType.ANALYSIS)

# TODO: change _eval_ -> _empty_func, _eval_&DAT_123 -> _get_DAT_123, etc






# infer constructors
fs = defaultdict(list) # function -> [(instruction, vftable)] -- the vftables referenced by likely constructors 
for s in currentProgram.getSymbolTable().getAllSymbols(True):
    if "vftable" == s.name:
        for r in s.references:
            addr = r.getFromAddress()
            instr = getInstructionAt(addr)
            func = getFunctionContaining(addr)
            if func:
                fs[func].append((instr, s))

cs = defaultdict(list) # vftable -> [function] -- constructors of each vftable type
for f, ls in sorted(fs.items(), key=lambda p: p[0].entryPoint):
    ls = sorted(ls, key=lambda p: p[0].address)
    ms = [(instr, s) for instr, s in ls if re.match(r'mov dword ptr \[\w\w\w\],', str(instr).lower())]
    if 0 < len(ms) < 4:
        last = ms[-1][1]
        if f.callingConvention.name == "__thiscall":
            last = ms[0][1] # in destructors the order can be reversed
        print "ctor", f.entryPoint, f, last.parentNamespace
        cs[last].append(f)
    else:
        print "weird", f.entryPoint, f, len(ms), "/", len(ls)

for s, ls in cs.items(): # apply names
    for i, f in enumerate(ls):
        if f.parentNamespace.name == "Global":
            f.parentNamespace = s.parentNamespace
        if f.name.startswith("FUN_"):
            f.setName("_ctor_" + str(f.entryPoint), ghidra.program.model.symbol.SourceType.IMPORTED)

# TODO: build an inherintance tree from which constructors use which vftables
# TODO: also find functions that have a lot of MOV [...], 0



# find all new() calls and constructors after them
class_sizes = {}
for s in currentProgram.getSymbolTable().getAllSymbols(True):
    if "operator_new" in s.name:
        print s.address, s, s.referenceCount
        for r in s.references:
            i0 = getInstructionAt(r.fromAddress)
            if not i0:
                break
                continue
            i00 = i0.previous
            if i00.mnemonicString == "PUSH" and len(i00.getOpObjects(0)) == 1:
                alloc_size = getattr(i00.getOpObjects(0)[0], "bigInteger", None)
                f1 = getFunctionAfter(i0.address)
                c = None
                i1 = i0.next
                while i1.address < f1.entryPoint:
                    if i1.mnemonicString == "CALL": # just assumes that the first call after is a constructor -- XXX: sometimes that's base constructor, and I'd need to find next vftable
                        if len(i1.getOpObjects(0)) == 1:
                            c = getSymbolAt(toAddr(i1.getOpObjects(0)[0].offset))
                            if c and "ctor" in c.name and c.parentNamespace.name != "Global":
                                class_sizes[c.parentNamespace] = (alloc_size, c, i0, i1)
                        break
                    i1 = i1.next

# for SCENE classes, set size and base type
for ns, (sz, ctor, i0, i1) in sorted(class_sizes.items(), key=lambda p: p[0].name):
    tp, _ = find_tp_ns(ns=ns)
    print i0.address, tp.pathName if tp else ns.name, ctor, hex(int(sz)) if sz else "---"
    #if tp and tp.pathName.startswith("/0Mine") and sz:
    #    print tp.length, int(sz)
    
    if tp:
        if tp.length <= 1 and tp.length < sz:
            tp.length = sz
        elif tp.length > sz:
            print "too big", tp.length
        elif tp.length < sz:
            print "too small?", tp.length
        
        if tp.name.startswith("SCENE_"):
            if tp.components[0].dataType == data.Undefined.DEFAULT:
                tp0 = find_tp_ns("SCENE_CBase")[0]
                tp.replaceAtOffset(0, tp0, tp0.length, "base", "")
        
        if tp.name.endswith("Task"):
            if tp.components[0].dataType == data.Undefined.DEFAULT:
                tp0 = find_tp_ns("AA_TaskNode")[0]
                tp.replaceAtOffset(0, tp0, tp0.length, "base", "")


# find PTR_vftable, which are likely static objects - there are only a few 
for s in currentProgram.getSymbolTable().getAllSymbols(True):
    if s.name.startswith("PTR_vftable"):
        print l, "->", "::".join(getSymbolAt(toAddr(getDataAt(l.address).getInt(0))).path)



# find all nonempty BBCF types, move them to /0Mine
#for tp in all_data_types(currentProgram.dataTypeManager.getCategory(0)): # too many headers
for tp in all_data_types("/ClassDataTypes"):
    if not hasattr(tp, "components"):
        continue
    l = tp.length
    c = len(tp.components)
    ct = sum(1 for c in tp.components if c.dataType != data.Undefined.DEFAULT)
    #if ct > 0: # non-emtpy
    if ct > 1: # defined
        print tp.pathName, hex(l), ct
    
    if currentProgram.dataTypeManager.getDataType("/0Mine/"+tp.name) or currentProgram.dataTypeManager.getDataType("/0Mine/Class/"+tp.name):
        print tp.pathName, "duplicate"
    
    if ct > 1:
        # move filled types to 0Mine
        tp.categoryPath = data.CategoryPath("/0Mine/Class")






# find all static variables

# TODO: make utils for this to run on a single static initializer, just to get its size
# TODO: there are also getters that do new(), like _get_AA_CFriendList
class search_for_bytes_Action(ghidra.util.bytesearch.GenericMatchAction):
    def __init__(self, ls):
        ghidra.util.bytesearch.GenericMatchAction.__init__(self, 0)
        self.ls = ls
    def apply(self, pr, addr, bytes):
        self.ls.append(addr)

def search_for_bytes(bs, mask=None, range=None):
    ls = []
    bs = list(bs)
    p = ghidra.util.bytesearch.GenericByteSequencePattern(
        bytes(bytearray(bs)), bytes(bytearray(mask or [0xff]*len(bs))),
        search_for_bytes_Action(ls))
    ss = ghidra.util.bytesearch.MemoryBytePatternSearcher("getters")
    ss.addPattern(p)
    ss.search(currentProgram, ghidra.program.model.address.AddressSet(*range) if range else currentProgram.getMemory(), ghidra.util.task.TaskMonitor.DUMMY)
    return ls

                      # pattern: MOV EAX, [????]; TEST AL,0x1; JNZ ?; OR EAX ,0x1
ls = search_for_bytes([0xa1, 0x00, 0x00, 0x00, 0x00, 0xa8, 0x01, 0x75, 0x00, 0x83, 0xc8, 0x01],
                      [0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff])
# TODO: sometimes it's MOV EBX, sometimes it's TEST 0x2, etc
len(ls)

from collections import defaultdict
d = defaultdict(list)
for l in ls:
    d[toAddr(getInt(l.add(1)))].append(l)

len(d)

d_results = {} # address -> parse results
it = ((k, v) for k, v in d.items() if k not in d_results)



# for each static var
flag_addr, lls = next(it); setCurrentLocation(lls[0]); print lls[0], "->", a
print next((i for i, a in enumerate(d.keys()) if a == flag_addr), None)

# look for data references
ops = [getInstructionAt(lls[0].add(17))]
lab_endif = lls[0].add(getByte(lls[0].add(8))).add(9)
while ops[-1].next.address < lab_endif:
    ops.append(ops[-1].next)

rs = [(op.mnemonicString, tuple(op.getOpObjects(0)), tuple(op.getOpObjects(1))) for op in ops]
ss = []
to_int = lambda v: v if type(v) in (int, float) else getattr(v, "bigInteger", getattr(v, "offset", None))
for i, (op, a1, a2) in enumerate(rs):
    if op == "PUSH" and len(a1) == 1 and rs[i+1][0] == "CALL":
        ss.append((to_int(a1[0]), to_int(rs[i+1][1][0])))
    if op == "MOV" and len(a1) == 1 and str(a1[0]) == "ECX" and rs[i+1][0] == "CALL":
        ss.append((to_int(a2[0]), to_int(rs[i+1][1][0])))
    if op == "MOV" and len(a1) == 1:
        ss.append((to_int(a1[0]), to_int(a2[0])))

ss1 = [(s and getSymbolAt(toAddr(s)), s and getSymbolAt(toAddr(z))) for s, z in ss]
ss2 = [(s.address, s, z) for s, z in ss1 if s and s.symbolType == symbol.SymbolType.LABEL]
l = len(ss2) > 0
_, first_data, first_val = min(ss2)
dt_size = flag_addr.offset - first_data.address.offset
ss1
print first_data, "=", first_val and first_val.parentNamespace, first_val, "\nto", flag_addr, "size:", hex(dt_size)
d_results[flag_addr] = (first_data, first_val)

if l and first_val and first_val.parentNamespace.name != "Global":
    nm = "_static_" + first_val.parentNamespace.name
    print "setName", first_data, nm
    if first_data.name.startswith("DAT_"):
        first_data.setName(nm, symbol.SourceType.IMPORTED)
elif l:
    nm = "_ctor_for_" + first_data.name
    print "setName", first_val, nm
    if first_val.name.startswith("FUN_"):
        first_val.setName(nm, symbol.SourceType.IMPORTED)

dt = getDataAt(first_data.address)
if not dt:
   nm0 = first_data.name
   try:
       print "new dt", createData(first_data.address, data.ArrayDataType(data.Undefined.DEFAULT, dt_size))
   except:
       print "! createData failed"
   # TODO: when this throws due to existing data, I could create a struct from it
   if first_data.name != nm0:
       print "! got renamed to", first_data.name
       first_data.setName(nm0, symbol.SourceType.IMPORTED)
elif dt.dataType.length != dt_size:
   print "! different"


# decide if the containing function is a simple getter that returns the address of the static variable

f = getFunctionContaining(lls[0])

res = decompile(f))
pops = list(res.highFunction.pcodeOps)
ret = [op for op in pops if op.opcode == op.RETURN]
print "len", len(pops), len(ret)
ret = ret[-1]
if len(ret.inputs) > 1 and not ret.inputs[1].isConstant():
    ret = ret.inputs[1].getDef()
    if ret.opcode == ret.CAST:
        ret = ret.inputs[0].getDef()

if len(ret.inputs) > 1 and ret.inputs[1].isConstant():
    v = ret.inputs[1].getOffset()
    print "ret", hex(v), v == first_data.address.offset
    
    if v == first_data.address.offset and len(pops) < 100 and len([op for op in pops if op.opcode == op.RETURN]) == 1:
        nm = "_get_" + first_data.name.split("static_")[-1]
        print "setName", f, nm
        if f.name.startswith("FUN_"):
            f.setName(nm, symbol.SourceType.IMPORTED)
else:
    print "not const"






# find all virtual calls
# TODO: need to decompile the functions to figure out what's being called. but before that, we need to create more types and apply them to more function signatures

i = getInstructionAt(getFunctionAfter(toAddr(0)).entryPoint)
vcalls = []
while i:
    if i.mnemonicString == "CALL":
        if len(i.getOpObjects(0)) > 1 or not hasattr(i.getOpObjects(0)[0], "offset"):
            vcalls.append(i)
    i = i.next

vcfs = defaultdict(list)
for i in vcalls:
    vcfs[getFunctionContaining(i.address)].append(i)

print len(vcalls), len(vcfs), sum(1 for f in vcfs if f and f.symbol.parentNamespace.name != "Global")

# list the virtual functions being called at the end of FUN_ names, to make them visible in call tree
for f, vfs in vcfs.items():
    if f and f.name.startswith("FUN_"): # or _eval_ ?
        i = f.name.find("_vf")
        nm = f.name if i == -1 else f.name[:i]
        seen = set()
        for c in vfs:
            off = 0
            if len(c.getOpObjects(0)) > 1:
                off = int(getattr(c.getOpObjects(0)[1], "bigInteger", 0))/4
            if off not in seen:
                if len(seen) >= 10:
                    nm += "..."
                    break
                nm += "_vf["+str(off)+"]"
                seen.add(off)
        print nm
        f.setName(nm, symbol.SourceType.IMPORTED)







# infer fpac index -> filename
# TODO: find all functions that call set_up_reading_0, decompile, then find calls to 
# 0007ca50 get_buffer(..., pac_index) and backtrack
# TODO: also indices used in LoadTask

hf = decompile(getFunctionAt(toAddr(0x000837d0))).highFunction # AA_PreloadTask::_init_0
for op in hf.getPcodeOps():
    # find CALL FPACBuffers::_get_data
    if op.opcode == op.CALL and op.inputs[0].address and op.inputs[0].offset == 0x0007ca50:
        print op.inputs[0].getPCAddress(), op
        if not op.inputs[2].constant:
            backtrack_op(op.inputs[2].def)
        print






