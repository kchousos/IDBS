hp:
	@echo " Compile hp_main ...";
	gcc -g -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/hp_main.c ./src/record.c ./src/hp_file.c ./src/stats.c -lbf -o ./build/hp_main -O2

bf:
	@echo " Compile bf_main ...";
	gcc -g -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/bf_main.c ./src/record.c -lbf -o ./build/bf_main -O2;

ht:
	@echo " Compile hp_main ...";
	gcc -g -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_main.c ./src/record.c ./src/ht_table.c ./src/stats.c -lbf -o ./build/ht_main -O2

sht:
	@echo " Compile hp_main ...";
	gcc -g -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/sht_main.c ./src/record.c ./src/sht_table.c ./src/ht_table.c ./src/stats.c -lbf -o ./build/sht_main -O2

clean:
	rm *.db build/*

run: sht
	rm *.db; ./build/sht_main
