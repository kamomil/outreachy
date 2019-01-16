#!/bin/bash
if [ "$#" -ne 1 ]; then
echo "need to pass file to test"
exit 1
fi
tmpfile=tmp${RANDOM:0:100}.c
echo $tmpfile

git checkout HEAD~1 || { echo 'git checkout failed' ; exit 1; }

cp $1 $tmpfile
git checkout -

git add $tmpfile
git commit -m "$tmpfile"
cp $1 $tmpfile
git add $tmpfile
git commit -m "$tmpfile 2"

perl ../latest/scripts/checkpatch.pl --strict --ignore=UNSPECIFIED_INT,LONG_LINE -g HEAD

