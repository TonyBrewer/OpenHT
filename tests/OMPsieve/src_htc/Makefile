TARGETS = rose_Sieve.c ../src_pers/design.htd ../src_pers/rose_rose_Sieve.c

$(TARGETS): Sieve.c
	rm -f PersOUT*.cpp Pers__HTC_HO*.cpp rose* design.htd
	/work/htc_releases/latest/bin/htc -I../src -rose:openmp:lowering -htc:split Sieve.c -htc:sysc -rose:skipfinalCompileStep -htc:sysc
	/work/htc_releases/latest/bin/htc -I../src -rose:openmp:lowering -DCNY_HTC_COPROC rose_Sieve.c -rose:skipfinalCompileStep -htc:sysc
	grep -v "___htc_dummy_decl" rose_rose_Sieve.c > ../src_pers/rose_rose_Sieve.c
	rm -f ../src_pers/PersOUT*.cpp ../src_pers/Pers__HTC_HO*.cpp
	cp Pers*.cpp ../src_pers

	sed 's/\(\#define __HTC_HOST_ENTRY_OUT_.*\)5/\11/' design.htd | sed 's/\(\#define OUT_.*HTID_W \)5/\19/' >../src_pers/design.htd

clean:
	rm -f $(TARGETS) rose_rose_Sieve.c design.htd PersOUT*.cpp Pers__HTC_HO*.cpp
