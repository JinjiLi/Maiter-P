/* 
 * File:   tuple.h
 *  
 * Version 1.0
 *
 * Copyright [2016] [JinjiLi/ Northeastern University]
 * 
 * Author: JinjiLi<L1332219548@163.com>
 * 
 * Created on December 26, 2016, 3:40 PM
 */
#include <iostream>
#include <fstream>
#include "../kernel/kernel.h"
#include <cstdlib>


using namespace dsm;

uint64_t adsorption_starts;
double adsorption_damping;

struct Link {

    Link(int inend, double inweight) : end(inend), weight(inweight) {
    }
    int end;
    double weight;
};

struct AdsorptionIterateKernel : public IterateKernel<int, float, vector<Link> > {
    float zero;

    AdsorptionIterateKernel() : zero(0) {
    }

    void read_data(string& line, int& k, vector<Link>& data) {
        string linestr(line);
        int pos = linestr.find("\t");
        int source = boost::lexical_cast<int>(linestr.substr(0, pos));

        vector<Link> linkvec;
        int spacepos = 0;
        string links = linestr.substr(pos + 1);
        while ((spacepos = links.find_first_of(" ")) != links.npos) {
            Link to(0, 0);

            if (spacepos > 0) {
                string link = links.substr(0, spacepos);
                int cut = links.find_first_of(",");
                to.end = boost::lexical_cast<int>(link.substr(0, cut));
                to.weight = boost::lexical_cast<float>(link.substr(cut + 1));
            }
            links = links.substr(spacepos + 1);
            linkvec.push_back(to);
        }

        k = source;
        data = linkvec;
    }

    void init_c(const int& k, float& delta, vector<Link>& data) {
        if (k < adsorption_starts) {
            delta = 10;
        } else {
            delta = 0;
        }
    }

    void init_v(const int& k, float& delta, vector<Link>& data) {
        delta = zero;
    }

    void accumulate(float& a, const float& b) {
        a = a + b;
    }

    void priority(float& pri, const float& value, const float& delta) {
        pri = delta;
    }

    void g_func(const int& k, const float& delta, const float& value, const vector<Link>& data, vector<pair<int, float> >* output) {
        for (vector<Link>::const_iterator it = data.begin(); it != data.end(); it++) {
            Link target = *it;
            float outv = delta * adsorption_damping * target.weight;
            output->push_back(make_pair(target.end, outv));
        }
    }

    const float& default_v() const {
        return zero;
    }
};

static int Adsorption() {

    maiter_init(); //configuration

    MaiterKernel<int, float, vector<Link> >* kernel = new MaiterKernel<int, float, vector<Link> >(
            new Sharding::Mod,
            new AdsorptionIterateKernel,
            new TermCheckers<int, float>::Diff);


    RunMaiterKernel<int, float, vector<Link> > * runkernel = new RunMaiterKernel<int, float, vector<Link> >(kernel);

    runkernel->runMaiter();

    delete runkernel;

    return 0;
}


int main(int argc, char** argv) {

    Adsorption();
    return 0;
}



