
ZIPNAME=kpidop_$(shell date +%Y%m%d%H%M).zip

FILES=../Release/kpidop.kpi ../zlib128-dll/zlib1.dll \
	COPYING COPYING.libid3tag COPYRIGHT.libid3tag LICENSE.zlib \
	kpidop.txt

zip:
	zip -9j $(ZIPNAME) $(FILES)
