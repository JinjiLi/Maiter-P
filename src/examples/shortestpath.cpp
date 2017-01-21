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


uint64_t shortestpath_source;

struct Link{
  Link(int inend, double inweight) : end(inend), weight(inweight) {}
  int end;
  double weight;
};
struct ShortestpathIterateKernel : public IterateKernel<int, double, vector<Link> > {
    double imax;

    ShortestpathIterateKernel(){
        imax = std::numeric_limits<double>::max();
    }

    void read_data(string& line, int& k, vector<Link>& data){
        string linestr(line);
        int pos = linestr.find("\t");
        int source = boost::lexical_cast<int>(linestr.substr(0, pos));

        vector<Link> linkvec;
        int spacepos = 0;
        string links = linestr.substr(pos+1);
        while((spacepos = links.find_first_of(" ")) != links.npos){
            Link to(0, 0);

            if(spacepos > 0){
                string link = links.substr(0, spacepos);
                int cut = links.find_first_of(",");
                to.end = boost::lexical_cast<int>(link.substr(0, cut));
                to.weight = boost::lexical_cast<double>(link.substr(cut+1));
            }
            links = links.substr(spacepos+1);
            linkvec.push_back(to);
        }

        k = source;
        data = linkvec;
    }

    void init_c(const int& k, double& delta,vector<Link>& data){
        if(k ==shortestpath_source){
            delta = 0;
        }else{
            delta = imax;
        }
    }
    void init_v(const int& k,double& v,vector<Link>& data){
        v = imax;  
    }
        
    void accumulate(double& a, const double& b){
        a = std::min(a, b); 
    }

    void priority(double& pri, const double& value, const double& delta){
        pri = value - std::min(value, delta); 
    }

    void g_func(const int &k, const double& delta,const double& value, const vector<Link>& data, vector<pair<int, double> >* output){
        for(vector<Link>::const_iterator it=data.begin(); it!=data.end(); it++){
            Link target = *it;
            double outv = delta + target.weight;
            output->push_back(make_pair(target.end, outv));
        }
    }

    const double& default_v() const {
        return imax;
    }
};


static int Shortestpath() {
    
       maiter_init(); //configuration
     
      shortestpath_source=get_config_option_long("SHORTESTPATH",0);
         
       MaiterKernel<int, double, vector<Link> >* kernel = new MaiterKernel<int, double, vector<Link> >(                                        
                                        new Sharding::Mod,
                                        new ShortestpathIterateKernel,
                                        new TermCheckers<int, double>::Diff);

  
    RunMaiterKernel<int, double, vector<Link> > * runkernel = new RunMaiterKernel<int, double, vector<Link> >(kernel);

    runkernel->runMaiter();

    delete runkernel;

    return 0;
}

int main(int argc, char** argv) {

    Shortestpath();
    return 0;
}
