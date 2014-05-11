#!/bin/bash
# mutt will delete the file before open is finished with it

filepath=$1

if [ -n "$filepath" ] 
then
  filename="${filepath##*/}"
  directory="${filepath%$filename}"

  attachpath="$directory/attachments"
  attachment="$attachpath/$filename"
  mkdir -m 700 -p "$attachpath"
  rm -rf "$attachpath/*"

  cp -f "$filepath" "$attachment"
  open "$attachment"
fi
