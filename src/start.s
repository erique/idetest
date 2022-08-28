	include 'exec/resident.i'
	include 'exec/nodes.i'

	xref	_main
	xref	_end
	xref	_IDString

	section	.text

start
	pea	.argv(pc)
	pea	((.argend-.argv)/4).w
	jsr	_main(pc)
	addq.l #8,sp
	rts

.argv	dc.l	.argv0
.argend
.argv0	dc.b	"idetest",0

.romtag
	dc.w	RTC_MATCHWORD	; RT_MATCHWORD
	dc.l	.romtag			; RT_MATCHTAG
	dc.l	__end			; RT_ENDSKIP
	dc.b	RTF_COLDSTART	; RT_FLAGS
	dc.b	VERSION			; RT_VERSION
	dc.b	NT_UNKNOWN		; RT_TYPE
	dc.b	0				; RT_PRI
	dc.l	.argv0			; RT_NAME
	dc.l	_IDString		; RT_IDSTRING
	dc.l	start			; RT_INIT
