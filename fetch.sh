#!/bin/sh

if [ -z $1 ]
then
  echo Specify source machine as parameter
  exit
fi

rsync -r $1:NVHTM-extensions $HOME/.
