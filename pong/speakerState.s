	.arch msp430g2553
	.p2align 1,0
	.text

	;; speaker_state is the state mach in to handle speaker states
	;; the state machine uses a switch statement
	
	.data			;s is static (in ram)
s:	.word 0 		; selector (which state)

	.text			; jt is constants (in flash)
jt:	.word option1		; jt[0]
	.word option2 		; jt[1]



	.global speaker_state
speaker_state:
	;; range check on selector (s)
	mov r12, &s		;arg in r12 (state)
	
	cmp #3, &s		; s-3
	jnc default		; doesn't brrow if s>2

	;; index into jt
	mov &s, r12
	add r12, r12 		; r12 = 2*s
	mov jt(r12), r0		; jmp jt[s]

	;; switch table options
	;; same order as normal c code
option1:
	mov #200, r12		; param is 200
	call #speaker_set_period
	jmp end
option2:
	mov #12000, r12		; param is 12000
	call #speaker_set_period
	jmp end
default:
end:
	pop r0			; return
