#!/bin/awk -f

# Check if a file is sorted by the first and second number fields

# Initialize the previous numbers
BEGIN {
    prev_field1 = -9999999999999;  # A very small initial value
    prev_field2 = -9999999999999;  # A very small initial value
    sorted = 1;  # Assume the file is sorted initially
}

# Process each line in the file
{
    current_field1 = $1;  # Extract the first number field
    current_field2 = $2;  # Extract the second number field
    
    # Check if the current line is sorted by the first field
    if (current_field1 < prev_field1) {
        sorted = 0;  # File is not sorted
        print "File is not sorted by the first field at line " NR ": " $0;
        exit;  # Exit the script immediately
    }

    # If the current line has the same first field, check the second field
    if (current_field1 == prev_field1 && current_field2 < prev_field2) {
        sorted = 0;  # File is not sorted
        print "File is not sorted by the second field at line " NR ": " $0;
        exit;  # Exit the script immediately
    }

    # Update the previous fields for the next iteration
    prev_field1 = current_field1;
    prev_field2 = current_field2;
}

# Print the result
END {
    if (sorted) {
        print "File is sorted by both the first and second fields.";
    }
}

