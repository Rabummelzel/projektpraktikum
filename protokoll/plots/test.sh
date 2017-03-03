#!/bin/bash
mkdir $PWD/pdf
for file in $PWD/*.svg
    do
        filename=$(basename $file)
        inkscape "$file" -d 1200 -A $PWD/pdf/"${filename%.svg}.pdf"
    done
