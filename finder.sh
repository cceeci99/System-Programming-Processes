#!/bin/bash

files="./results/*.out"                                 # results files are stored in directory results

for tld in $@
do 
    sum=0

    for file in $files                                  # iterate over .out files in directory results
    do
        if [ -f "$file" ]
        then
            while IFS=' ' read -ra line;                # read line by line each file, and seperate line with " " blank space
            do
                if grep -q ".$tld " <<< "${line[0]} "   # search for .com  (with space at end) to avoid counting any non-existing TLD
                then
                    sum=$(expr $sum + ${line[1]})       # add num of appearances when tld is found
                fi

            done < $file
        else
            echo "Warning: $file is not file\n"
        fi
    done
    echo -e "$tld $sum"
done