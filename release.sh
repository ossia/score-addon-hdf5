#!/bin/bash
rm -rf release
mkdir -p release

cp -rf DataReader *.{hpp,cpp,txt,json} LICENSE release/

mv release score-addon-datareader
7z a score-addon-datareader.zip score-addon-datareader
