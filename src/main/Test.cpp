#include <iostream>
#include <string>
#include <vector>
#include <set>
#include "configuration/types.hpp"
#include "suff/Filter.hpp"

void TestFilter(char *argv[]) {
    std::string pattern_path(argv[1]);
    suff::Filter filter(pattern_path, {0});
    VMapping mapping;
    mapping.insert_or_assign(0, 0);
    suff::VFilter vfilter(filter, mapping, 0);
    std::vector<VertexID> vids = {0, 2, 3, 5, 7, 11, 13, 17, 19,
                                  23, 29, 31, 37, 41, 43, 47, 53,
                                  59, 61, 67, 71, 73, 79, 83, 89,
                                  97, 1, 4, 8, 16, 32, 64};
    vfilter.filters[0].init_filter(vids.size());
    std::set<VertexID> vid_set;
    for (auto i = 0; i < vids.size(); i++) {
        vfilter.filters[0].add(vids[i]);
        vid_set.insert(vids[i]);
    }
    // test, fn is false negative and so on
    long tp = 0, tn = 0, fp = 0, fn = 0;
    for (ui i = 0; i < 1000; i++) {
        if (vfilter.contains(i)) {
            if (vid_set.contains(i)) {
                tp++;
            } else {
                fp++;
            }
        } else {
            if (vid_set.contains(i)) {
                fn++;
            } else {
                tn++;
            }
        }
    }
    std::cout << "-- original --" << std::endl;
    std::cout << "tp = " << tp << ", tn = " << tn << ", fp = " << fp << ", fn = " << fn << std::endl;
    // test save and load
    vfilter.filters[0].dump("./temp/0");
    suff::Filter filter2("./temp/0");
    suff::VFilter vfilter2(filter2, mapping, 0);
    // filter2.load_data();
    vfilter2.load_data();
    // std::cout << (vfilter.filters[0].bf == vfilter2.filters[0].bf) << std::endl;
    tp = 0, tn = 0, fp = 0, fn = 0;
    for (ui i = 0; i < 1000; i++) {
        if (vfilter2.contains(i)) {
            if (vid_set.contains(i)) {
                tp++;
            } else {
                fp++;
            }
        } else {
            if (vid_set.contains(i)) {
                fn++;
            } else {
                tn++;
            }
        }
    }
    std::cout << "-- loaded --" << std::endl;
    std::cout << "tp = " << tp << ", tn = " << tn << ", fp = " << fp << ", fn = " << fn << std::endl;
}

int main(int argc, char *argv[]) {
    TestFilter(argv);
}