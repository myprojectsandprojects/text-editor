# gcc -g main.c undo.c editing.c $(pkg-config --cflags gtk+-3.0) $(pkg-config --libs gtk+-3.0) && ./a.out
gcc -g main.c fileio.c tab.c search.c undo.c editing.c highlighting.c sidebar.c $(pkg-config --cflags gtk+-3.0) $(pkg-config --libs gtk+-3.0) && ./a.out
