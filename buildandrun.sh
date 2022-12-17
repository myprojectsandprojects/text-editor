g++ -g\
	-Wno-deprecated-declarations\
	main.cpp\
	tab.cpp\
	search-replace.cpp\
	undo.cpp\
	editing.cpp\
	highlighting.cpp\
	highlighting-c.cpp\
	highlighting-cpp.cpp\
	highlighting-rust.cpp\
	strings.cpp\
	file-browser.cpp\
	search-in-files.cpp\
	autocomplete-identifier.cpp\
	root-navigation.cpp\
	open.cpp\
	autocomplete-character.cpp\
	hotloader.cpp\
	tests.cpp\
	lib.cpp\
	$(pkg-config --cflags gtk+-3.0) $(pkg-config --libs gtk+-3.0) && ./a.out
