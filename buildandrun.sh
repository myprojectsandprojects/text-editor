g++ -g\
	main.c\
	tab.c\
	search-replace.c\
	undo.c\
	editing.c\
	highlighting-simple.cpp\
	highlighting-c.cpp\
	highlighting-cpp.cpp\
	highlighting-rust.cpp\
	strings.c\
	file-browser.c\
	search-in-files.c\
	autocomplete.c\
	root-navigation.c\
	open.c\
	autocomplete-character.c\
	hotloader.c\
	tests.c\
	lib.cpp\
	$(pkg-config --cflags gtk+-3.0) $(pkg-config --libs gtk+-3.0) && ./a.out
