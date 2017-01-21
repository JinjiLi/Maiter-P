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


uint64_t katz_source;
double katz_beta;
uint64_t shards;


struct KatzIterateKernel : public IterateKernel<int, float, vector<int> > {
    
    float zero;

    KatzIterateKernel() : zero(0){}

    void read_data(string& line, int& k, vector<int>& data){
        string linestr(line);
        int pos = linestr.find("\t");
        int source = boost::lexical_cast<int>(linestr.substr(0, pos));

        vector<int> linkvec;
        string links = linestr.substr(pos+1);
        int spacepos = 0;
        while((spacepos = links.find_first_of(" ")) != links.npos){
            int to;
            if(spacepos > 0){
                to = boost::lexical_cast<int>(links.substr(0, spacepos));
            }
            links = links.substr(spacepos+1);
            linkvec.push_back(to);
        }

        k = source;
        data = linkvec;
    }

    void init_c(const int& k, float& delta,vector<int>& data){
        if(k == katz_source){
            delta = 1000000;
        }else{
            delta = 0;
        }
    }

    void init_v(const int& k, float& delta,vector<int>&data){
        delta=zero;
    }
    void accumulate(float& a, const float& b){
        a = a + b;
    }

    void priority(float& pri, const float& value, const float& delta){
        pri = delta;
    }

    void g_func(const int& k,const float& delta, const float& value, const vector<int>& data, vector<pair<int, float> >* output){
        float outv = katz_beta * delta;
        for(vector<int>::const_iterator it=data.begin(); it!=data.end(); it++){
            int target = *it;
            output->push_back(make_pair(target, outv));
        }
    }

    const float& default_v() const {
        return zero;
    }
};


static int Katz() {
     maiter_init(); //configuration
     
      katz_source=get_config_option_long("katz_source",0);
      katz_beta=get_config_option_long("katz_beta",0);
      shards=get_config_option_long("shards",0);
     
    MaiterKernel<int, float, vector<int> >* kernel = new MaiterKernel<int, float, vector<int> >(
                                        new Sharding::Mod,
                                        new KatzIterateKernel,
                                        new TermCheckers<int, float>::Diff);
    
    
    RunMaiterKernel<int, float, vector<int> > * runkernel = new RunMaiterKernel<int, float, vector<int> >(kernel);

    runkernel->runMaiter();

    delete runkernel;

    return 0;
}


int main(int argc, char** argv) {

    Katz();
    return 0;
}




