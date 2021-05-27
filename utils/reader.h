#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "common.h"

namespace reader
{
    std::vector<edge> parse_entries(std::string filename)
    {
        std::vector<edge> edges;
        std::ifstream newfile(filename);

        if (newfile.is_open())
        {
            std::string tp;
            while (getline(newfile, tp))
            {
                if ((tp.compare(0, 1, "#") == 0) or (tp.compare(0, 1, "%") == 0))
                {
                    continue;
                }

                else
                {
                    int a, b;
                    std::stringstream s_stream(tp);
                    s_stream >> a;
                    s_stream >> b;
                    edge to_insert;
                    to_insert.src_id = a;
                    to_insert.dst_id = b;
                    edges.push_back(to_insert);
                }
            }
            newfile.close();
        }
        else
        {
            cout << "could not open";
        }
        return edges;
    }
}