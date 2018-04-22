import sys
from operator import itemgetter
from itertools import groupby

tokenized_filename = []
#unsorted_file = open("","r")
unsorted_file = sys.stdin.read()

filenames = unsorted_file.split("\n")
print(filenames)
for i in range(len(filenames)) : 
	token_store = filenames[i].split(".")
	tokenized_filename.append(token_store)
	print(type(filenames[i]));

tokenized_filename.sort(key=itemgetter(-1))
sortted_filenames = groupby(tokenized_filename, itemgetter(-1))

for elt, items in sortted_filenames:
    #print(elt, items)
    for i in items:
    	final_string = '.'.join(i)
    	print(final_string)
