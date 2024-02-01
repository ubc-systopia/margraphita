#!/bin/awk -f

{
    # Trim trailing whitespace
    sub(/[[:space:]]*$/, "")

    # Check if the line has the form "number<whitespace>number"
    if ($0 ~ /^[0-9]+[[:space:]]+[0-9]+$/) {
        # Replace whitespace with newline
        gsub(/[[:space:]]+/, "\n")

        # Print the modified line
        print
    }
}

