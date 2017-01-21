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

//DECLARE_string(graph_dir);

struct PagerankIterateKernel : public IterateKernel<int, double, vector<int> > {
    double zero;

    PagerankIterateKernel() : zero(0) {
    }

    void read_data(string& line, int& k, vector<int>& data) {
        string linestr(line);
        int pos = linestr.find("\t"); //找到制表符的位置
        if (pos == -1) return;

        int source = atoi(linestr.substr(0, pos).c_str()); //字符串转换成数值  源点

        vector<int> linkvec;
        string links = linestr.substr(pos + 1);
        //cout<<"links:"<<links<<endl;
        if (*(links.end() - 1) != ' ') {
            links = links + " ";
        }
        int spacepos = 0;
        while ((spacepos = links.find_first_of(" ")) != links.npos) {
            int to;
            if (spacepos > 0) {
                to = atoi(links.substr(0, spacepos).c_str());
                //cout<<"to:"<<to<<endl;
            }
            links = links.substr(spacepos + 1);
            linkvec.push_back(to);
        }

        k = source;
        data = linkvec;
    }

    void init_c(const int& k, double& delta, vector<int>& data) {
        delta = 0.2;
    }

    void init_v(const int& k, double& v, vector<int>& data) {
        v = 0;
    }

    void accumulate(double& a, const double& b) {
        a = a + b;
    }

    void priority(double& pri, const double& value, const double& delta) {
        pri = delta;
    }

    void g_func(const int& k, const double& delta, const double&value, const vector<int>& data, vector<pair<int, double> >* output) {
        int size = (int) data.size();
        double outv = delta * 0.8 / size;
        //cout << "size " << size << endl;
        for (vector<int>::const_iterator it = data.begin(); it != data.end(); it++) {
            int target = *it;
            output->push_back(make_pair(target, outv));
            //cout << k<<"  "<<target<< "   "<<outv<<endl;

        }
    }

    const double& default_v() const {
        return zero;
    }
};

static int Pagerank() {

    maiter_init(); //configuration

    MaiterKernel<int, double, vector<int> >* kernel = new MaiterKernel<int, double, vector<int> >(
            new Sharding::Mod,
            new PagerankIterateKernel,
            new TermCheckers<int, double>::Diff);

    RunMaiterKernel<int, double, vector<int> > * runkernel = new RunMaiterKernel<int, double, vector<int> >(kernel);

    runkernel->runMaiter();
   
   
    delete runkernel;
      
    return 0;
}

/*
 * 
 */
int main(int argc, char** argv) {

    Pagerank();
    return 0;
}
