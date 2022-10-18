#include "reader.h"

#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "bulk_insert.h"
#include "common.h"
namespace reader
{

std::vector<edge> parse_edge_entries(std::string filename)
{
    std::vector<edge> edges;
    std::ifstream newfile(filename);
    std::cout << filename << std::endl;
    if (newfile.is_open())
    {
        std::string tp;
        while (getline(newfile, tp))
        {
            if ((tp.compare(0, 1, "#") == 0) or (tp.compare(0, 1, "%") == 0) or
                (tp.empty()))
            {
                continue;
            }

            else
            {
                node_id_t a, b;
                std::stringstream s_stream(tp);
                s_stream >> a;
                s_stream >> b;
                edge to_insert = {0};
                to_insert.src_id = a;
                to_insert.dst_id = b;

                edges.push_back(to_insert);
                lock.lock();
                in_adjlist[b].push_back(a);   // insert a in b's in_adjlist
                out_adjlist[a].push_back(b);  // insert b in a's out_adjlist
                lock.unlock();
            }
        }
        newfile.close();
    }
    else
    {
        std::cout << "**could not open " << filename << std::endl;
    }
    return edges;
}

std::vector<node> parse_node_entries(std::string filename)
{
    std::vector<node> nodes;
    std::ifstream newfile(filename);

    if (newfile.is_open())
    {
        std::string tp;
        while (getline(newfile, tp))
        {
            if ((tp.compare(0, 1, "#") == 0) or (tp.compare(0, 1, "%") == 0) or
                (tp.empty()))
            {
                continue;
            }
            else
            {
                node to_insert;
                std::stringstream s_str(tp);
                s_str >> to_insert.id;

                try
                {
                    to_insert.in_degree = in_adjlist.at(to_insert.id).size();
                }
                catch (const std::out_of_range &oor)
                {
                    to_insert.in_degree = 0;
                }

                try
                {
                    to_insert.out_degree = out_adjlist.at(to_insert.id).size();
                }
                catch (const std::out_of_range &oor)
                {
                    to_insert.out_degree = 0;
                }

                nodes.push_back(to_insert);
            }
        }
        newfile.close();
    }
    else
    {
        std::cout << "** could not open " << filename << std::endl;
    }
    return nodes;
}

}  // namespace reader