APP      = iview
STACK    = 4480k
MALLOC   = 0k

TOOLPATH = ../../Tools/
INCPATH  = ../../Tools/haribote/
APILIBPATH   = ../../apilib/
HARIBOTEPATH = ../../Neru/

MAKE     = $(TOOLPATH)make -r
NASK     = $(TOOLPATH)nask
CC1      = $(TOOLPATH)gocc1 -I$(INCPATH) -I../ -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask -a
OBJ2BIM  = $(TOOLPATH)obj2bim
MAKEFONT = $(TOOLPATH)makefont
BIN2OBJ  = $(TOOLPATH)bin2obj
BIM2HRB  = $(TOOLPATH)bim2hrb
BIM2BIN  = $(TOOLPATH)bim2bin
RULEFILE = ../haribote.rul
EDIMG    = $(TOOLPATH)edimg
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00 
HARITOL  = $(TOOLPATH)haritol
COPY     = $(HARITOL) copy
DEL      = $(HARITOL) remove

CLEAN_FILES := $(wildcard *.lst *.map *.bim *.org)

# デフォルト動作

default :
	$(MAKE) $(APP).hrb

# ファイル生成規則

$(APP).bim : $(APP).obj bmp.obj jpeg.obj $(APILIBPATH)apilib.lib Makefile
	$(OBJ2BIM) @$(RULEFILE) out:$(APP).bim map:$(APP).map stack:$(STACK) \
		$(APP).obj jpeg.obj bmp.obj $(APILIBPATH)apilib.lib

neruos.img : ../../Neru/ipl20.bin ../../Neru/haribote.sys $(APP).hrb \
		Makefile
	$(EDIMG)   imgin:../../Tools/fdimg0at.tek \
		wbinimg src:../../Neru/ipl20.bin len:512 from:0 to:0 \
		copy from:../../Neru/haribote.sys to:@: \
		copy from:$(APP).hrb to:@: \
		copy from:../../Data/font/nihongo/nihongo.fnt to:@: \
		imgout:neruos.img

# 一般規則

%.gas : %.c ../apilib.h Makefile
	$(CC1) -o $*.gas $*.c

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

%.org : %.bim Makefile
	$(BIM2HRB) $*.bim $*.org $(MALLOC)

%.hrb : %.org Makefile
	$(BIM2BIN) -osacmp in:$*.org out:$*.hrb

# コマンド

run :
	$(MAKE) neruos.img
	$(COPY) neruos.img ..\..\Tools\qemu\fdimage0.bin
	$(MAKE) -C ../../Tools/qemu

full :
	$(MAKE) -C $(APILIBPATH)
	$(MAKE) $(APP).hrb

run_full :
	$(MAKE) -C $(APILIBPATH)
	$(MAKE) -C ../../Neru
	$(MAKE) run

clean :
	-$(DEL) $(CLEAN_FILES)
	-$(DEL) iview.obj
	-$(DEL) jpeg.obj
	-$(DEL) neruos.img

src_only :
	$(MAKE) clean
	-$(DEL) $(APP).hrb
