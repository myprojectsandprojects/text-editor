#gcc -g main.c fileio.c tab.c search.c undo.c editing.c highlighting.c sidebar.c strings.c $(pkg-config --cflags gtk+-3.0) $(pkg-config --libs gtk+-3.0) && ./a.out
gcc -g main.c fileio.c tab.c search.c undo.c editing.c highlighting.c strings.c file-browser.c search-in-files.c autocomplete.c $(pkg-config --cflags gtk+-3.0) $(pkg-config --libs gtk+-3.0) && ./a.out
