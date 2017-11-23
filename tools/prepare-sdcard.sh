#!/bin/bash

SD_CARD=$1


function writefile()
{
    echo "hello world! $1" > ${SD_CARD}${1}
}

echo "Preparing sd-card: $SD_CARD"

mkdir $SD_CARD/parent
mkdir $SD_CARD/parent-directory

writefile "/myfile.txt"
writefile "/mylongfilenamefile.txt"
writefile "/mylongfilenamefilesecond.txt"

writefile "parent/myfile.txt"
writefile "parent/mylongfilenamefile.txt"
writefile "parent/mylongfilenamefilesecond.txt"

writefile "parent-directory/myfile.txt"
writefile "parent-directory/mylongfilenamefile.txt"
writefile "parent-directory/mylongfilenamefilesecond.txt"


