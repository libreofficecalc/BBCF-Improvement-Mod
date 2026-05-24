#Requires AutoHotkey v2.0
#SingleInstance Force

; Temporary URT automation template.
; Edit this file directly for your exact menu path and waits.

signalPath := ""
resultPath := ""
for arg in A_Args {
    if InStr(arg, "/signal=") = 1 {
        signalPath := SubStr(arg, 9)
    } else if InStr(arg, "/result=") = 1 {
        resultPath := SubStr(arg, 9)
    }
}

if (resultPath = "") {
    MsgBox "Missing /result= path from runner."
    ExitApp 21
}

if (signalPath != "") {
    FileAppend("START " A_Now "`n", signalPath, "UTF-8")
}

WriteResult(status, detail := "") {
    global resultPath
    payload := status
    if (detail != "") {
        payload .= "`n" detail
    }
    FileDelete(resultPath)
    FileAppend(payload "`n", resultPath, "UTF-8")
}

Tap(key, holdMs := 45, gapMs := 120) {
    Send("{" key " down}")
    Sleep(holdMs)
    Send("{" key " up}")
    Sleep(gapMs)
}

TapMany(key, count, holdMs := 45, gapMs := 120) {
    Loop count {
        Tap(key, holdMs, gapMs)
    }
}

try {
    if !WinWait("ahk_exe BBCF.exe",, 30) {
        WriteResult("SCRIPT_REPORTED_FAILURE", "BBCF window not found within 30s")
        ExitApp 1
    }

    WinActivate("ahk_exe BBCF.exe")
    Sleep(500)

    ; --------- EDIT BELOW THIS LINE ---------
    ; Example sequence from your description.
    ; Replace timings/actions with whatever is needed.
    expectGameRunningAtEnd := true

    ; Wait for intro to become skippable, then press J twice.
    Sleep(5500)
    Tap("j")
    Sleep(1800)
    Tap("j")

    ; Menu navigation example: up x2, confirm x3 with waits.
    Sleep(1200)
    TapMany("Up", 2)
    Tap("j")
    Sleep(700)
    Tap("j")
    Sleep(700)
    Tap("j")

    ; Example wait for beginning animation and skip.
    Sleep(2000)
    Tap("j")

    ; Optional: manually close game at end from script.
    ; Send("!{F4}")
    ; Sleep(500)

    ; --------- EDIT ABOVE THIS LINE ---------

    gameRunning := ProcessExist("BBCF.exe")
    if (expectGameRunningAtEnd && gameRunning) {
        WriteResult("DONE", "AHK sequence finished; BBCF still running")
        ExitApp 0
    }

    if (!expectGameRunningAtEnd && !gameRunning) {
        WriteResult("DONE", "AHK sequence finished; BBCF intentionally closed")
        ExitApp 0
    }

    if (expectGameRunningAtEnd && !gameRunning) {
        WriteResult("SCRIPT_REPORTED_FAILURE", "Sequence ended but BBCF is not running (likely crash/close)")
        ExitApp 2
    }

    WriteResult("SCRIPT_REPORTED_FAILURE", "Sequence ended but BBCF is still running when closure was expected")
    ExitApp 4
}
catch as err {
    WriteResult("SCRIPT_REPORTED_FAILURE", "AHK exception: " err.Message)
    ExitApp 3
}
