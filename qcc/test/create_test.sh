#!/bin/bash

name="$1.qz"
echo "Do you want to create test named '$name'? [y/N]"
read response

if [ $response == "y" ]; then
    echo "Creating '../programs/$name'"
    touch ../programs/$name
    echo "Creating './cases/$name'"
    touch ./cases/$name
else
    echo "NO!"
fi
