;
; Ullrich von Bassewitz, 2004-06-30
;
; CC65 runtime: right shift support for unsigneds
;
; Note: The standard declares a shift count that is negative or >= the
; bitcount of the shifted type for undefined behaviour.
;
; Note^2: The compiler knowns about the register/zero page usage of this
; function, so you need to change the compiler source if you change it!
;


	.export		tosshrax
	.import		popax
	.importzp	tmp1

tosshrax:
        and     #$0F            ; Bring the shift count into a valid range
        sta     tmp1            ; Save it

        jsr     popax           ; Get the left hand operand

        ldy     tmp1            ; Get shift count
        beq     L9              ; Bail out if shift count zero

        cpy     #8              ; Shift count 8 or greater?
        bcc     L3              ; Jump if not

; Shift count is greater 7. The carry is set when we enter here.

        tya
        sbc     #8
        tay                     ; Adjust shift count
        txa
        ldx     #$00            ; Shift by 8 bits
        beq     L2              ; Branch always
L1:     lsr     a
L2:     dey
        bpl     L1
        rts

; Shift count is less than 8. Do the actual shift.

L3:     stx     tmp1            ; Save high byte of lhs
L4:     lsr     tmp1
        ror     a
        dey
       	bne     L4

; Done with shift

        ldx	tmp1
L9:     rts

