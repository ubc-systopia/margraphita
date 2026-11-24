Axis 1: what signal does the score use?

Structural only: CN, Jaccard, Adamic-Adar, PA, RA. Uses only get_out_nodes_id and get_out_degree. No properties.

Structural + scalar features: structural score, weighted or filtered by properties like degree, community_id, account_age, is_bot. Reads a few scalar properties.

Embedding-based: dot product or cosine of two node-embedding vectors. Reads one dense vector per node.

Hybrid: concatenate structural features + embedding dims, feed to a learned classifier.

Axis 2: how are those features stored?

No properties at all (structural-only).

Embedded blob (prop_mode = EMBEDDED): one set_node_properties(id, ptr, size) / get_node_properties(id) call, decode with decode_*_blob().

Columnar (get_node_prop_cursor(TABLE, CG_X)->seek(id)->get_uint64(col)): one seek per colgroup, read only the columns you ask for.



Perf
https://www.brendangregg.com/perf.html
