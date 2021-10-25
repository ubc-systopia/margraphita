# Packing Values
### The Problem
* So as it turns out, packing using the wiredtiger stream methods is 2X faster
  than the garbage standard library "__" delimiter hack. 
* The implementation I had for pack_int_vector_wt is robust. The buffer malloc'd
  is **to the best of my understanding** guaranteed to be large enough.
* Also, I don't ever expect the situation where the caller does not know the
  format of the columns being packed. This format string can be saved as a DB
  metadata. 
* This simplifies greatly the packing process -- now we don't need to create the
 format string and can simply use what's provided to us.

 ### Next Steps:
1. Create a test branch and try using adjacencylist + wt_pack. I expect it to
    work. If it doesn't -- move on, Mary.
    - I have a feeling that this is going to involve changing the format
      specifiers for the packed adjlist to 'u' <-- I think that's the format
      used by the packed binary
    - You might consider changing the signature of the
      CommonUtil::pack_int_vector to (std::vector<int>, string format, WT_ITEM
      packed). WT_ITEM has fields for the value and it's size so that's directly
      usable in the function.


