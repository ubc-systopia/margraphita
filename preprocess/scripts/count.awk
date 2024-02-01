#!/bin/awk -f

# This script counts the number of lines that don't begin with #

# Initialize the count variable
BEGIN {
    count = 0
}

# For each line that doesn't start with #
!/^#/ {
    count++
}

# At the end, print the count
END {
    print count
}