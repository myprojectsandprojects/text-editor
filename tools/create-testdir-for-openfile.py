# python3 create-testdir-for-openfile.py
# sudo updatedb

import os
from os import path


alphabet = "abcdefghijklmnopqrstuvwxyz"

dir_name = "test-dir"
if not path.exists(dir_name):
	os.mkdir(dir_name)

for i in range(1, len(alphabet)+1):
	file_name = alphabet[0:i]
	file_path = path.join(dir_name, file_name)

	f = open(file_path, "w")
	f.write(file_name)
	f.close()
