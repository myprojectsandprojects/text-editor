objects := main.o tab.o search-replace.o undo.o editing.o highlighting.o highlighting-c.o highlighting-cpp.o highlighting-rust.o strings.o file-browser.o search-in-files.o autocomplete-identifier.o root-navigation.o open.o text-expansion.o hotloader.o tests.o multi-cursor-ability.o lib.o

texty: $(objects)
#texty: %.o
	#g++ -g $^ $$(pkg-config --libs gtk+-3.0) -o texty
	g++ -o1 $^ $$(pkg-config --libs gtk+-3.0) -o texty

$(objects): %.o : %.cpp
	#g++ -g -Wall -Wextra -Wno-deprecated-declarations $$(pkg-config --cflags gtk+-3.0) -c $^ -o $@
	#g++ -g -Wno-deprecated-declarations $$(pkg-config --cflags gtk+-3.0) -c $^ -o $@
	g++ -o1 -Wno-deprecated-declarations $$(pkg-config --cflags gtk+-3.0) -c $^ -o $@

clean:
	rm -f texty $(objects)

