NAME = file_masm\head
OBJS = $(NAME).obj
FILEPATH = hosC±‡“Î\Debug


LINK_FLAG = /base:0xc0000000 /out:system.exe /subsystem:windows /warn:0
ML_FLAG = /c /coff

$(NAME).exe: $(OBJS)  file_masm\interrup.obj  $(FILEPATH)\*.obj
#**************************************************
	Link $(LINK_FLAG)  $(OBJS)  file_masm\interrup.obj  $(FILEPATH)\*.obj
 
.asm.obj:
	ml $(ML_FLAG) $<

