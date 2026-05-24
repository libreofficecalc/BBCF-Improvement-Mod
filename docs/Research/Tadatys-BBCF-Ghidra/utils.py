#
# Utility functions for managing ghidra state
#
# Intended for use in the Jython console
#

from ghidra.program.model import *
from collections import defaultdict, Counter
import re

def base_type(tp):
    while hasattr(tp, "dataType") and tp != tp.dataType:
        tp = tp.dataType
    return tp

def is_bbcf_type(tp):
    # try not to mess with types outside of these categories
    if "<" in tp.name:
        return False # ignore template types
    return tp.pathName.startswith("/0Mine/") or tp.pathName.startswith("/ClassDataTypes/")

def is_unnamed(s):
    #return s.source == symbol.SourceType.ANALYSIS
    return any(s.name.startswith(pr) for pr in ["FUN_", "DAT_", "LAB_", "_eval_"])


def find_tp_ns(tp=None, ns=None, create_ns=False):
    # quickly look up related type and namespace from each other's names
    if ns and not tp:
        tp = ns.name
    
    if type(tp) in (str, unicode):
        for pr in ["", "/0Mine/", "/0Mine/Class/", "/0Mine/Tmp/", "/ClassDataTypes/"]:
            dt = currentProgram.dataTypeManager.getDataType(pr + tp)
            if dt:
                tp = dt
                break
        if not dt:
            tp = None
    
    if type(ns) in (str, unicode):
        ns = getNamespace(None, ns) or create_ns and createNamespace(None, ns)
    
    elif tp and not ns:
        btp = base_type(tp)
        ns = getNamespace(None, btp.name)
        if not ns and btp.name.endswith("!"):
            ns = getNamespace(None, btp.name[:-1])
        if not ns and create_ns:
            ns = createNamespace(btp.name)
    
    return (tp, ns)

def decompile(f): # takes a function or currentLocation in existing decompile
    if hasattr(f, "decompile"):
        return f.decompile
    
    if hasattr(f, "address"):
        f = getFunctionContaining(f.address)
    
    dcc = ghidra.app.decompiler.DecompInterface()
    dcc.openProgram(currentProgram)
    res = dcc.decompileFunction(f, 60, ghidra.util.task.ConsoleTaskMonitor())
    
    if not res.decompileCompleted():
        print "x decompile failed for", f
        return None
    
    return res



# parsing headers
def import_header_file(filename, dst_manager=None):
    # reads types from header and puts them in /{Header.h} category
    main = currentProgram.dataTypeManager
    dst_manager = dst_manager or main # = data.StandAloneDataTypeManager("Temp")
    r = ghidra.app.util.cparser.C.CParserUtils.parseHeaderFiles([main], [filename], [], dst_manager, ghidra.util.task.ConsoleTaskMonitor())
    
    # structs get imported with packingEnabled, which causes weird alignment problems
    import os.path
    cat_name = "/" + os.path.basename(filename)
    cat = dst_manager.getCategory(data.CategoryPath(cat_name))
    tps = [t for t in sorted_dependencies(map(base_type, cat.dataTypes)) if t.pathName.startswith(cat_name)]
    for t in tps:
        if not hasattr(t, "packingEnabled"):
            continue
        t.packingEnabled = False
        for i in range(len(t.components)-1, -1, -1):
            c = t.components[i]
            if c.dataType == data.Undefined.DEFAULT and not c.fieldName:
                t.delete(i)
    
    return r and r.successful()

def import_header(code):
    # much more fragile than the above
    p = ghidra.app.util.cparser.C.CParser(currentProgram.dataTypeManager)
    p.parse(code)
    cp = ghidra.program.model.data.CategoryPath("/0Mine")
    for ls in [p.getTypes(), p.getComposites(), p.getEnums()]:
        for tp in ls.values():
            print tp
            tp.setCategoryPath(cp)
            currentProgram.dataTypeManager.addDataType(tp, None) # XXX: on name collision creates a "type_name.conflict" type
    return p

if False:
    import_header("""struct MenuItem {
        int32_t pad_00;
        char id[32];
        char title[32];
        // size 0x44
    };""")

def import_symbols(ls, add=True, rename=False):
    # TODO: also apply data types, except on functions
    for l in ls:
        addr, name = l[:2]
        addr = toAddr(addr + currentProgram.imageBase.offset)
        s = getSymbolAt(addr)
        #name = re.sub('[^a-zA-Z0-9_]', '_', name)
        path = name.split("::")
        ns = path[-2] if len(path) > 1 else None
        ns = ns and getNamespace(None, ns)
        name = path[-1]
        if s:
            if s.name != path:
                print addr, "::".join(s.path), "->", "::".join(path)
                if add and is_unnamed(s) or rename:
                    s.setName(name, ghidra.program.model.symbol.SourceType.IMPORTED)
                    if ns:
                        s.setNamespace(ns)
        else:
            print addr, "add", "::".join(path)
            if add:
                s = currentProgram.symbolTable.createLabel(addr, name, ghidra.program.model.symbol.SourceType.IMPORTED)
                if ns:
                    s.setNamespace(ns)

def _symbols_from_fields(tp, add=True, rename=False):
    ls = []
    for c in currentProgram.dataTypeManager.getDataType("/BBCF.h/BBCF").components:
        if not c.fieldName:
            continue # XXX: these should have already been removed?
        nm = re.sub(r"([^_])__(_*[^_])", r"\1::\2", c.fieldName) # replace __ with ::
        if not nm.startswith("__"): # ignore padding
            ls.append((c.offset, nm, c.dataType))
    
    return ls

def write_header_file_0(tp, filename):
    # dumps the t=given type and all of its dependencies to a file
    # the file doesn't parse well
    #w = java.io.StringWriter()
    w = java.io.FileWriter(filename)
    tw = ghidra.program.model.data.DataTypeWriter(currentProgram.dataTypeManager, w, True)
    tw.write([tp], ghidra.util.task.ConsoleTaskMonitor())
    w.close()

def sorted_dependencies(tps, pred=None):
    # topological sort on a list of types
    seen = set()
    res = []
    
    def dfs(tp):
        if tp in seen or not tp:
            return
        seen.add(tp)
        
        for c in getattr(tp, "components", []):
            if not c.dataType:
                continue
            b = base_type(c.dataType)
            if pred and not pred(c.dataType, b, tp): # type, base type, containing type
                continue
            
            dfs(b)
        
        res.append(tp)
    
    for t in tps:
        dfs(t)
    
    return res

def field_info(c, escape=lambda x: x, seen_names=None):
    # returns C-like `type_name field_name`
    # TODO: function pointers?
    nm = c.fieldName and escape(c.fieldName) or ("__%04x" % c.offset)
    if seen_names is not None:
        nm0, i = nm, 1
        while nm in seen_names:
            nm = nm0 + ("_" if nm0[-1:] != "_" else "") + str(i)
            i += 1
        seen_names.add(nm)
    
    nm = " " + nm
    tp = c.dataType
    while hasattr(tp, "dataType") and tp.dataType != tp and tp.dataType != None:
        if isinstance(tp, data.Array):
            if nm[0]=="*":
                nm = "(" + nm + ")" # pointer to array type
            nm += "[" + short_hex(tp.numElements) + "]"
        elif isinstance(tp, data.Pointer):
            nm = "*" + nm
        tp = tp.dataType
    
    if hasattr(tp, "bitSize"):
        nm = nm + ":" + str(tp.bitSize)
        tp = tp.baseDataType
    
    tpnm = escape(tp.name)
    if not is_bbcf_type(tp) and len(tp.pathName.split("/")) > 2:
        tpnm += "_placeholder"
    
    nm = tpnm + nm
    if isinstance(tp, data.Structure):
        nm = "struct " + nm;
    if isinstance(tp, data.Enum):
        nm = "enum " + nm;
    
    return nm

def c_escape(name):
    name = re.sub('<', '_lt', re.sub('>', '_gt', name)) # so that templates don't end up looking like namespaces
    name = re.sub('[^a-zA-Z0-9_]', '_', name)
    if name[:1].isnumeric():
        name = "_"+name
    return name

def write_header_file(tps, filename):
    with open(filename, "w") as fo:
        fo.write("""typedef unsigned char undefined;
typedef unsigned char undefined1;
typedef unsigned short undefined2;
typedef unsigned int undefined4;
typedef undefined* pointer;

typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;
typedef long long longlong;
typedef unsigned int dword;

#pragma pack ( push, 1 )


""")
        for t in tps:
            if not is_bbcf_type(t):
                fo.write("/* " + t.pathName + " */\nstruct " + c_escape(t.name) + "_placeholder { undefined __0000[" + short_hex(t.length) + "]; };\n\n")
                continue
            
            if t.description:
                fo.write("/*" + t.description + "*/\n")
            
            if hasattr(t, "components"):
                nm = c_escape(t.name)
                if isinstance(t, data.Union):
                    fo.write("union " + nm + " {\n")
                else:
                    fo.write("struct " + nm + " {\n")
                seen_names = set()
                for c in t.components:
                    fo.write("    " + field_info(c, c_escape, seen_names) + ";" + (" //" + c.comment if c.comment else "")+ "\n")
                fo.write("};\n")
                fo.write("static_assert(sizeof(" + nm + ") == " + str(t.length) + ");\n\n")
            
            elif hasattr(t, "names"):
                fo.write("enum " + c_escape(t.name) + " {\n")
                for n in t.names:
                    v = int(t.getValue(n))
                    if n == "_" + str(v): # skip default names?
                        continue
                    fo.write("    " + c_escape(n) + " = " + short_hex(v) + ",\n")
                fo.write("};\n\n")
            
            else:
                print "skipped", t.pathName
        
        fo.write("\n#pragma pack ( pop )\n")

def diff_types(a, b, check_field_type=True):
    # prting offsets where the two types have different fields
    print a.pathName, hex(a.length), "|", b.pathName, hex(b.length)
    i, j, off = 0, 0, 0
    for _ in range(len(a.components) + len(b.components)):
        while (a.components[i+1].offset if i+1 < len(a.components) else a.length)  <= off:
            i += 1
        
        ac = a.components[i] if i < len(a.components) else None
        anm = ac.fieldName or "_"+hex(ac.offset) if ac else "(end)"
        aoff = ac.offset if ac else a.length
        alen = ac.length if ac else 0x1000000
        if check_field_type:
            anm = ac.dataType.name + " " + anm if ac else anm
        anmo = anm + " + " + hex(off-aoff) if off != aoff else anm
        
        while (b.components[j+1].offset if j+1 < len(b.components) else b.length) <= off:
            j += 1
        
        bc = b.components[j] if j < len(b.components) else None
        bnm = bc.fieldName or "_"+hex(bc.offset) if bc else "(end)"
        boff = bc.offset if bc else b.length
        blen = bc.length if bc else 0x1000000
        if check_field_type:
            bnm = bc.dataType.name + " " + bnm if bc else bnm
        bnmo = bnm + " + " + hex(off-boff) if off != boff else bnm
        
        if anmo != bnmo:
            print hex(off)+":", anmo, "|", bnmo
        
        off = max(min(aoff+alen, boff+blen), off+1)
        if off >= max(a.length, b.length):
            break


def build_type(tp, fields):
    # reset type fields from given list of (offset, name, data_type, comment)
    tp.setLength(0)
    last_off = 0
    last_pad = None
    
    for i, (off, name, tp1, comment) in enumerate(fields):
        if hasattr(off, "offset"): # can be an address or int
            off = off.offset
        
        if off < last_off:
            # new field is inside the previous
            continue
        
        elif off > last_off:
            # add padding
            if last_pad:
                tp0 = data.ArrayDataType(data.Undefined.DEFAULT, off-last_off+last_pad.length)
                tp.delete(len(tp.components)-1)
                tp.add(tp0, last_pad.fieldName, last_pad.comment)
                last_pad = None
            else:
                tp0 = data.ArrayDataType(data.Undefined.DEFAULT, off-last_off)
                tp.add(tp0, None, "")
        
        if hasattr(name, "parentNamespace"): # can be a symbol or string
            s = name
            name = "::".join(s.path)
        
        tp0 = tp1 or data.ArrayDataType(data.Undefined.DEFAULT, 1)
        c = tp.add(tp0, name, comment)
        last_pad = c if tp1 is None else None
        
        last_off = off + tp0.length
    
    return tp

# pack struct undefined components into arrays
def pack_struct(tp):
    i, i0, tp0 = 0, 0, None
    runs = []
    if not hasattr(tp, "components"):
        return
    
    for i, c in enumerate(tp.components):
        if c.dataType != tp0 or tp0 is None or c.fieldName is not None:
            if i > i0+3 and tp0:
                runs.append((i0, i, tp0))
            i0 = i
            if not c.fieldName and c.dataType.length == 1:
                tp0 = c.dataType
            else:
                tp0 = None
    
    if i > i0+3 and tp0:
        runs.append((i0, i+1, tp0))
    
    if len(runs):
        print "gaps", tp.name, runs
    
    for i, j, tp0 in runs[::-1]:
        # not sure what happends if tp.packingEnabled
        tp.replace(i, data.ArrayDataType(tp0, j-i), -1)


def all_data_types(*cats):
    for cat in cats:
        if type(cat) == str:
            cat = currentProgram.dataTypeManager.getCategory(ghidra.program.model.data.CategoryPath(cat))
        
        for t in all_data_types(*cat.categories):
            yield t
        
        for t in cat.dataTypes:
            yield t

def pack_all_structs():
    for tp in all_data_types("/0Mine", "/ClassDataTypes"):
        pack_struct(tp)

if False:
    pack_all_structs()


# rename a type and all of it's related symbols
def rename_object(nm, nm1):
    get = [s for k in ["get_" + nm, "_get_" + nm] for s in getSymbols(k, None)]
    st = [s for k in ["static_" + nm, "_static_" + nm, nm] for s in getSymbols(k, None)]
    tp, ns = find_tp_ns(nm)
    
    print get, st, tp and tp.name, ns
    
    assert len(get) <= 1, "too many" + repr(get)
    assert len(st) <= 1, "too many" + repr(st)
    
    for s in get:
        s.setName(s.name[:-len(nm)] + nm1, symbol.SourceType.USER_DEFINED)
    
    for s in st:
        pr = s.name[:-len(nm)]
        if nm1.startswith("DAT"):
            pr = ""
        elif pr == "":
            pr = "_static_"
        s.setName(pr + nm1, symbol.SourceType.USER_DEFINED)
    
    if ns:
        ns.symbol.setName(nm1, symbol.SourceType.USER_DEFINED)
    
    if tp:
        tp.setName(nm1)
    
    print get, st, tp and tp.name, ns

# TODO: util to remove the _ from all funcs in a namespace?
# util to apply a new type for static, getter and all ns items?



# rename param_01 with known type
_data_type_abbr = {}
def type_abbr(tp):
    tp_nm = tp.name
    nm = _data_type_abbr.get(tp.name)
    
    if not nm and is_bbcf_type(tp):
        i = tp_nm.find("_")
        if i > 0:
            tp_nm = tp_nm[0] + tp_nm[i:]
        nm = "".join(c for c in tp_nm if c.isupper()).lower()
        if nm and i > 0:
            nm = nm[0] + "_" + nm[1:]
    
    return nm or "param"

if False:
    for f in currentProgram.functionManager.getFunctions(True):
        p = f.parameters[0] if len(f.parameters) > 0 else None
        tp = p and base_type(p.dataType)
        if not hasattr(tp, "components"):
            continue
        if not is_bbcf_type(tp):
            continue
        nm = type_abbr(tp) + "_1"
        if nm != p.name:
            print "setName", f.entryPoint, f, nm, "--", f.signature
            try:
                p.setName(nm, symbol.SourceType.IMPORTED)
            except:
                print ("x")


# update function signature
def set_method_type(func, tp=None, ns=None, check_return=False, base_ptr=True):
    # make Name::func(Name* n, ...)
    tp, ns = find_tp_ns(tp, ns)
    if not tp:
        print "no type", getattr(tp, "name", tp), ns
        return
    if ns:
        func.parentNamespace = ns
    
    p1tp = data.PointerDataType(base_type(tp), 4) if base_ptr else tp
    if func.callingConventionName == "__thiscall":
        p1 = listing.ParameterImpl(type_abbr(tp) + "_1", p1tp, currentProgram.getRegister("ECX"), currentProgram, symbol.SourceType.IMPORTED)
    else:
        p1 = listing.ParameterImpl(type_abbr(tp) + "_1", p1tp, currentProgram, symbol.SourceType.IMPORTED)
    
    r1 = p1 if check_return and func.return.dataType.length == 4 else func.return
    func.updateFunction(None, r1,
        [p1] + list(func.parameters[1:]),
        func.FunctionUpdateType.CUSTOM_STORAGE if func.callingConventionName == "__thiscall" else func.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,
        False, symbol.SourceType.IMPORTED)
    
    print func.parentNamespace, func.signature

def set_vftable_method_types(tp=None, ns=None):
    # every function in a vftable takes this as first parameter
    tp, ns = find_tp_ns(tp, ns)
    if not tp or not ns:
        print "not found", getattr(tp, "name", tp), ns
        return
    
    for s in getSymbols("vftable", ns):
        d = getDataAt(s.address)
        print s.parentNamespace, s.name, d.numComponents, s.address
        for i in range(d.numComponents):
            f = getFunctionAt(toAddr(d.getInt(i*4)))
            if f.parentNamespace == ns:
                set_method_type(f, tp, None, "ctor" in f.name or "dtor" in f.name)

def set_ns_method_types(tp=None, ns=None):
    # most functions in a namespace take this as first parameter. but not all!
    tp, ns = find_tp_ns(tp, ns)
    if not tp or not ns:
        print "not found", getattr(tp, "name", tp), ns
        return
    
    for f in currentProgram.getSymbolTable().getSymbols(ns):
        if f.symbolType != symbol.SymbolType.FUNCTION:
            continue
        f = f.getObject()
        set_method_type(f, tp, "ctor" in f.name or "dtor" in f.name)



# infer function parameter types from decompile (param_1 only)
def infer_call_param_type_from_op(op):
    if op.opcode != op.CALL or len(op.inputs) <= 1:
        return "no call"
    func = getFunctionAt(toAddr(op.inputs[0].offset))
    tp = op.inputs[1].high.dataType
    btp = base_type(tp)
    df = op.inputs[1].getDef()
    if not is_bbcf_type(btp) and df and df.opcode == op.CAST:
        tp = df.inputs[0].high.dataType
        btp = base_type(tp)
    if not hasattr(btp, "components") or not is_bbcf_type(btp):
        return "bad type?", btp.pathName
    print " " if func.parameters[0].dataType != tp else "=", func, func.parameters[0].dataType, tp
    if func.parameters[0].dataType != tp:
        set_method_type(func, tp, None, base_ptr=False)

def infer_call_param_types(f):
    hf = decompile(f).highFunction
    for op in hf.pcodeOps:
        if op.opcode == op.CALL:
            infer_call_param_type_from_op(op)

# TODO: also infer return type

if False:
    op = next(currentLocation.decompile.highFunction.getPcodeOps(currentLocation.address)); infer_call_param_type_from_op(op)

if False:
    infer_call_param_types(currentLocation)


# annotate virtual function calls of known type
def backtrack_op(op, n=10): # trace first non-constant varnode upwards
    print op
    for i in range(n):
        op1 = None
        for j, ii in enumerate(op.inputs):
            if not ii.constant:
                op = op1 = ii.def
                break
        if not op1:
            return
        print j, ":", op, "::" + ii.high.dataType.name

def _infer_vftable(op):
    tp, field, offset = None, None, None
    if op.opcode == op.CALLIND:
        op = op.inputs[0].def
    else:
        return None, None, None
    while True:
        if op.opcode == op.CAST:
            op = op.inputs[0].def
        elif op.opcode == op.LOAD and op.inputs[0].constant:
            op = op.inputs[1].def
        else:
            break
    
    if op.opcode == op.PTRADD and op.inputs[1].constant:
        offset = op.inputs[1].offset
        op = op.inputs[0].def
    elif op.opcode == op.INT_ADD and op.inputs[1].constant:
        offset = op.inputs[1].offset/4
        op = op.inputs[0].def
    elif op.opcode == op.PTRSUB and op.inputs[1].constant:
        offset = op.inputs[1].offset
        op = op.inputs[0].def
    else:
        return None, None, None
    
    while True:
        if op.opcode == op.CAST:
            op = op.inputs[0].def
        elif op.opcode == op.LOAD and op.inputs[0].constant:
            op = op.inputs[1].def
        else:
            break
    
    if op.opcode == op.PTRSUB and op.inputs[1].constant:
        field = op.inputs[1].offset
        tp = op.inputs[0].high.dataType # this works for foo->vftable[n]()
        #tp = op.output.high.dataType # this was needed for the INT_ADD?
        # with (*foo->vftable)(), there is only one PTRSUB and two LOADs
        return tp, field, offset
    else:
        return None, None, None

_type_to_vftable = {
    "OBJ_CBase": [("OBJ_CCharBase", "vftable")],
    "OBJ_CCharBase": [("OBJ_CBase", "vftable")], # in case some base pointers are mistyped as child pointers
} # parent type -> child namespaces, or vftables with non-standard names
# TODO: manually add SCENE_CBase, AA_TaskNode
# TODO: infer parent-child relations from constructors?

def infer_vftable_call(op):
    tp, field, offset = _infer_vftable(op)
    tp = tp and base_type(tp)
    tp, ns = find_tp_ns(tp)
    if not ns:
        return
    tabs = [(ns.name, "vftable"), (ns.name, "vftable_1")] + _type_to_vftable.get(tp.name, []) if tp and ns else []
    ss = [s for ns, tb in tabs for s in getSymbols(tb, getNamespace(None, ns))]
    if len(ss): # in case of 2 vftables, we should check `field`, but it may be hard to tell
        seen = set()
        fs = []
        for s in ss:
            d = getDataAt(s.address)
            f = getFunctionAt(toAddr(d.getInt(offset*4)))
            if not f:
                print "bad vftable", s.address, hex(int(offset*4)), toAddr(d.getInt(offset*4))
                continue
            if f not in seen:
                fs.append(f)
                seen.add(f)
        return fs

def annotate_vftable_call(op):
    fs = infer_vftable_call(op)
    if fs:
        setPreComment(op.inputs[0].high.getPCAddress(), "calls " +
            " or ".join("{@symbol " + str(f.entryPoint) + "}" for f in fs))

def annotate_vftable_calls(f):
    hf = decompile(f).highFunction
    for op in hf.pcodeOps:
        if op.opcode == op.CALLIND:
            annotate_vftable_call(op)

if False:
    op = next(currentLocation.decompile.highFunction.getPcodeOps(currentLocation.address)); annotate_vftable_call(op)

if False:
    annotate_vftable_calls(currentLocation)


# expand addresses in text
def re_find_and_replace(pattern, txt, f):
    res = ""
    i = 0
    for m in re.finditer(pattern, txt):
        res += txt[i:m.start()]
        res += f(m)
        i = m.end()
    
    return res + txt[i:]

short_hex = lambda n: str(n) if n < 10 else hex(n)

def tp_offset_info(tp, off):
    r = ""
    while off > 0:
        if hasattr(tp, "components"):
            c = tp.getComponentContaining(off)
            r += "." + (c.fieldName or "_"+short_hex(c.offset))
            off -= c.offset
            tp = c.dataType
        elif hasattr(tp, "numElements"):
            i = int(off/tp.elementLength)
            r += "["+short_hex(i)+"]"
            off = off % tp.elementLength
            tp = tp.dataType
        else:
            r += "+"+short_hex(off)
            off = 0
    return r

def addr_info(a):
    if a.offset == 0:
        return "null"
    s = getSymbolAt(a) or getSymbolBefore(a)
    f = getFunctionContaining(a)
    d = getDataContaining(a)
    r = "??"
    if d:
        r = "::".join(d.symbols[0].path)
        tp = d.dataType
        off = int(a.offset - d.address.offset)
        r += tp_offset_info(tp, off)
    elif f:
        r = "::".join(f.symbol.path)
        if a != f.entryPoint:
            r = "in " + r
    elif s:
        r = "::".join(s.path)
        off = int(a.offset - s.address.offset)
        if off > 0:
            r += "+"+short_hex(off)
            off = 0
    return r

def expand_addrs(txt):
    def fmt(m):
        k = m.group(1)
        a = toAddr(int(k, 16))
        return k + " (" + addr_info(a) + ")"
    
    return re_find_and_replace(r"([0-9A-Fa-f]{8})", txt, fmt)

def format_doc(txt):
    def fmt(m):
        k = m.group(1) or m.group(2)
        a = toAddr(int(k, 16))
        return "`" + k + " " + addr_info(a) + "`"
    
    return re_find_and_replace(r"([0-9A-Fa-f]{8})|`([0-9A-Fa-f]{8})(\s[^`]*)?`", txt, fmt)

def format_doc_file(filename):
    doc = ""
    with open(filename) as f:
        doc = format_doc(f.read())
    
    with open(filename, "w") as f:
        f.write(doc)

def field_offset(tp, path):
    if type(path) == str:
        path = path.split(".")
    tp0 = tp
    offsets = []
    for i, nm in enumerate(path):
        tp1 = None
        for c in tp.components:
            if c.fieldName == nm:
                tp1 = c.dataType
                offsets.append(c.offset)
        if tp1:
            tp = tp1
        else:
            print "path_offset", i, tp.name, "has no", nm
            break
    return " + ".join(hex(int(o)) for o in offsets)


if False:
    # some useful actions:
    infer_call_param_type_from_op(next(currentLocation.decompile.highFunction.getPcodeOps(currentLocation.address)))
    
    infer_call_param_types(currentLocation)
    
    annotate_vftable_calls(currentLocation)
    
    expand_addrs("""    """)
    
    format_doc_file("path/to/README.md")
    
    rename_object("old", "new")




