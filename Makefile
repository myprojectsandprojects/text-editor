objects := main.o tab.o search-replace.o undo.o editing.o highlighting.o highlighting-c.o highlighting-cpp.o highlighting-rust.o strings.o file-browser.o search-in-files.o autocomplete-identifier.o root-navigation.o open.o text-expansion.o hotloader.o tests.o multi-cursor-ability.o lib.o

texty: $(objects)
	g++ $^ $$(pkg-config --libs gtk+-3.0) -o texty

$(objects): %.o : %.cpp
	g++ -Wno-deprecated-declarations $$(pkg-config --cflags gtk+-3.0) -c $^ -o $@

clean:
	rm -f texty $(objects)

