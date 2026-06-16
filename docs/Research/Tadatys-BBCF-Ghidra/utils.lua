-- lua scripts for cheat engine

-- log stack trace
function print_stack_trace()
    local base = getAddress("BBCF.exe")
    local ln = (string.format("%08x ", EIP - base))
    local bp = EBP -- TODO: some functions don't do mov ebp,esp!
    for i = 1, 10 do
        local ip = readInteger(bp+4)
        ln = ln .. (string.format("%08x ", ip - base))
        bp = readInteger(bp)
        if bp == 0 or ip == 0 or ip - base == 0x3a5675 then
            break
        end
    end
    print ("stack: " .. ln)
    print (string.format("EAX %08x  EBX %08x  ECX %08x  EDX %08x", EAX, EBX, ECX, EDX))
    print (string.format("ESI %08x  EDI %08x  EBP %08x  ESP %08x", ESI, EDI, EBP, ESP))
    print ()
end

[[
BBCF.exe+9678F (get_button_hit) breakpoint condition:
if EAX > 0 then
    print ("key", readInteger(EBP+12))
    print_stack_trace()
end
]]


-- log vftables in task list
function print_tasks()
    local base = getAddress("BBCF.exe")
    local manager = readInteger(base + 0x008929c8)
    local app = readInteger(manager + 4)
    local node = readInteger(app + 12)
    while node ~= 0 do
        print (string.format("%08x -> bbcf.exe + %08x, p=%08x", node, readInteger(node) - base, readInteger(node+16)))
        node = readInteger(node + 4)
    end
end


-- change scene?
function change_scene(mode, scene)
    local base = getAddress("BBCF.exe")
    local scene_ptr = readInteger(base + 0x008903b0 + 0x2604)
    --writeInteger(base + 0x008903b0 + 0x108, mode) -- set GameMode
    writeInteger(base + 0x008903b0 + 0x110, scene) -- set next GameScene
    if scene_ptr ~= 0 then
        --writeInteger(scene_ptr + 0x2c, 11) -- skip status 10
        writeInteger(scene_ptr + 0x14, 2) -- set node for removal
    end
end

--[[ modes:

change_scene(6, 6) -- char select
change_scene(6, 14) -- vs info
change_scene(6, 15) -- battle
change_scene(11, 15) -- replay theater battle
change_scene(11, 26) -- replay theater list
change_scene(13, 24) -- story select
change_scene(13, 27) -- main menu
change_scene(13, 29) -- library
change_scene(12, 3) -- intro
change_scene(12, 4) -- title screen
change_scene(5, 16) -- end of battle?



intro -> replay theater segfaults, but title -> replay theater seems ok. but then segfaults on replay start?
]]


local start_loaded_replay_asm = [[
alloc(newmem, 2048)
createthread(newmem)

newmem:
  mov ecx, %x
  mov [ecx], 1
  mov ecx, %x
  call %x
  ret
]]

function start_loaded_replay()
    local base = getAddress("BBCF.exe")
    local scene_ptr = readInteger(base + 0x008903b0 + 0x2604)
    local menu_state = base + 0x01304b90
    local set_next_scene = base + 0x002c2960
    local asm = string.format(start_loaded_replay_asm, menu_state, scene_ptr, set_next_scene)
    print (asm)
    autoAssemble(asm)
end



-- export labels for ghidra
function export_labels()
    local al = getAddressList()
    local base = getAddress("BBCF.exe")
    for i = 0, al.Count-1 do
        -- TODO: handle pointer types
        if al[i].CurrentAddress ~= 0 then
            print (string.format('(0x%x, "CE_%s"),', al[i].CurrentAddress - base, al[i].Description))
        end
    end
end
