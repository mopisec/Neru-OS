TOOLPATH = Tools/
INCPATH  = Tools/haribote/

MAKE     = $(TOOLPATH)make -r
EDIMG    = $(TOOLPATH)edimg
HARITOL  = $(TOOLPATH)haritol
COPY     = copy
MOVE     = move
DEL      = del

# デフォルト動作

default :
	$(MAKE) neruos.img

# ファイル生成規則

neruos.img : Neru/ipl09.bin Neru/haribote.sys Makefile \
		App/noodle/noodle.hrb App/type/type.hrb App/chklang/chklang.hrb \
		App/mcircle/mcircle.hrb App/invader/invader.hrb \
		App/calc/calc.hrb App/tview/tview.hrb App/mmlplay/mmlplay.hrb App/iview/iview.hrb
	$(EDIMG)   imgin:Tools/fdimg0at.tek \
		wbinimg src:Neru/ipl09.bin len:512 from:0 to:0 \
		copy from:Neru/haribote.sys to:@: \
		copy from:startup.hsf to:@: \
		copy from:App/noodle/noodle.hrb to:@: \
		copy from:App/type/type.hrb to:@: \
		copy from:App/chklang/chklang.hrb to:@: \
		copy from:Data/text/euc.txt to:@: \
		copy from:App/mcircle/mcircle.hrb to:@: \
		copy from:App/invader/invader.hrb to:@: \
		copy from:App/calc/calc.hrb to:@: \
		copy from:App/tview/tview.hrb to:@: \
		copy from:App/mmlplay/mmlplay.hrb to:@: \
		copy from:App/earnpi/earnpi.hrb to:@: \
		copy from:Data/mml/kirakira.mml to:@: \
		copy from:Data/mml/fujisan.mml to:@: \
		copy from:Data/mml/daigo.mml to:@: \
		copy from:Data/mml/daiku.mml to:@: \
		copy from:App/iview/iview.hrb to:@: \
		copy from:App/osinfo/osinfo.hrb to:@: \
		copy from:App/help/help.hrb to:@: \
		copy from:App/crack/crack.hrb to:@: \
		copy from:App/echo/echo.hrb to:@: \
		copy from:App/osselect/osselect.hrb to:@: \
		copy from:App/launcher/launcher.hrb to:@: \
		copy from:App/finfo/finfo.hrb to:@: \
		copy from:App/nnall/nnall.hrb to:@: \
		copy from:Data/image/fujisan.jpg to:@: \
		copy from:Data/image/night.bmp to:@: \
		copy from:Data/image/neruos.jpg to:@: \
		copy from:Data/text/help.txt to:@: \
		copy from:Data/text/sample.txt to:@: \
		copy from:Data/text/license.txt to:@: \
		copy from:Data/font/nihongo/nihongo.fnt to:@: \
		imgout:neruos.img
	$(MAKE) -C Tools/makeiso

# コマンド

run :
	$(MAKE) neruos.img
	$(HARITOL) concat Tools/qemu/fdimage0.bin neruos.img
	$(MAKE) -C Tools/qemu

brun :
	$(MAKE) neruos.img
	$(HARITOL) concat Tools/bochs/fdimage0.bin neruos.img
	$(MAKE) -C Tools/bochs

install :
	$(MAKE) neruos.img
	$(HARITOL) concat Tools/fdwrite/fdimage0.bin neruos.img
	$(MAKE) -C Tools/fdwrite

iso :
	$(MAKE) neruos.img
	$(HARITOL) concat Tools/makeiso/fdimage0.bin neruos.img
	$(MAKE) -C Tools/makeiso
	$(HARITOL) concat neruos.iso Tools/makeiso/neruos.iso

full :
	$(MAKE) -C Neru
	$(MAKE) -C apilib
	$(MAKE) -C App
	$(MAKE) neruos.img

run_full :
	$(MAKE) full
	$(HARITOL) concat Tools/qemu/fdimage0.bin neruos.img
	$(MAKE) -C Tools/qemu

install_full :
	$(MAKE) full
	$(IMGTOL) w a: neruos.img

run_os :
	$(MAKE) -C Neru
	$(MAKE) run

clean :
# 何もしない

src_only :
	$(MAKE) clean
	-$(HARITOL) remove neruos.img

clean_full :
	$(MAKE) -C Neru			clean
	$(MAKE) -C apilib		clean
	$(MAKE) -C App			clean

src_only_full :
	$(MAKE) -C Neru			src_only
	$(MAKE) -C apilib		src_only
	$(MAKE) -C App			src_only
	-$(HARITOL) remove neruos.img
	-$(HARITOL) remove neruos.iso

refresh :
	$(MAKE) full
	$(MAKE) clean_full
	-$(HARITOL) remove neruos.img
