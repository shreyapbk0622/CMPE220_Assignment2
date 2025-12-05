; call_demo.asm
; Simple recursive countdown to demonstrate CALL and RET
; Uses your custom ISA: NOP, MOV, ADD, SUB, JMP, JZ, CALL, RET, HALT

; Entry point
start:
    MOV R0, 4       ; start with n = 4
    CALL func       ; call func(n)
    HALT            ; when recursion is done, halt the CPU

; func():
; if R0 == 0: return
; else: R0 = R0 - 1; CALL func(); return

func:
    ; --- base case check: if R0 == 0, jump to end_func ---
    SUB R0, 0       ; ALU out = R0, sets ZR flag if R0 == 0
    JZ end_func     ; if zero flag set, go to RET

    ; --- recursive case: R0 = R0 - 1 ---
    SUB R0, 1       ; decrement R0

    ; --- recursive call ---
    CALL func

end_func:
    RET
