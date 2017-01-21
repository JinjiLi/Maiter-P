/* 
 * File:   kernel.h
 * 
 * Version 1.0
 *
 * Copyright [2016] [JinjiLi/ Northeastern University]
 * 
 * Author: JinjiLi<L1332219548@163.com>
 * 
 * Created on December 26, 2016, 3:40 PM
 */

#ifndef KERNEL_H
#define	KERNEL_H
#include "table.h"
#include "statetable.h"
#include "globaltable.h"
#include "../util/common.h"
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include "../util/cmdopts.h"
#include "vertexlock-manager.h"
#include <pthread.h>



namespace dsm {

    template <class K, class V, class D>
    class MaiterKernel;

    class DSMKernel {
    };

    double total_time_1, total_time_2, total_time_3;

    template <class K, class V, class D>
    class MaiterKernel1 : public DSMKernel { //the first phase: initialize the statetable
    private:
        MaiterKernel<K, V, D>* maiter; //user-defined iteratekernel
    public:

        void set_maiter(MaiterKernel<K, V, D>* inmaiter) {
            maiter = inmaiter;
        }

        ~MaiterKernel1() {
        }

        void read_file(TypedGlobalTable<K, V, V, D>* table) {
            string patition_file = StringPrintf("%s/input_graph", maiter->input.c_str());
            //cout<<"Unable to open file: " << patition_file<<endl;
            ifstream inFile;
            inFile.open(patition_file.c_str());
            if (!inFile) {
                //cout<<"Unable to open file: " << patition_file;
                cerr << "Unable to open file" << patition_file;
                exit(1); // terminate with error
            }

            string linechr;
            int _line = 0;
            while (getline(inFile, linechr)) { //read a line of the input file
                K key;
                V delta;
                D data;
                V value;
                string line(linechr);
                maiter->iterkernel->read_data(line, key, data); //invoke api, get the value of key field and data field
                maiter->iterkernel->init_v(key, value, data); //invoke api, get the initial v field value
                maiter->iterkernel->init_c(key, delta, data); //invoke api, get the initial delta v field value
                //cout<<"key: "<<key<<"delta: "<<delta<<"value: "<<value<<"   "<<data[0][0]<<"  "<<data[1][0]<<"   "<<data[2][0]<<endl;
                table->put(key, delta, value, data); //initialize a row of the state table (a node)
                _line++;

            }
            maiter->line = _line;
            logstream(LOG_INFO) << "input data total  line is " << _line << std::endl;

            inFile.close();
            if (maiter->num_threads > maiter->line) {
                logstream(LOG_FATAL) << "input data total  line is small , num of threads is big,please reset the num of threads,let it less line  " << std::endl;
            }
        }

        void init_table(TypedGlobalTable<K, V, V, D>* a) {

            if (!a->initialized()) {
                a->InitStateTable(maiter->num_threads, maiter->schedule_portion,
                        maiter->sharder, maiter->iterkernel, maiter->termchecker); //initialize the statetable
            }
            a->resize(maiter->num_nodes); //create local state table based on the input size

            read_file(a); //initialize the state table fields based on the input data file
        }

        void run() {//run first phase
            logstream(LOG_INFO) << "initializing table " << std::endl;

            Timer timer;
            init_table(maiter->globaltable);
            total_time_1 = timer.elapsed();
        }
    };

    template <class K, class V, class D>
    struct Thread_Arg {
        MaiterKernel<K, V, D>* _maiter;
        typename TypedGlobalTable<K, V, V, D>::Iterator **it2_address;
        Vertexlock * vertexlock;

        int count_wait;
        pthread_mutex_t schedulem;
        pthread_cond_t schedulec;

        bool schedule_wait;
        pthread_mutex_t threadm;
        pthread_cond_t threadc;

        pthread_barrier_t barrier;
    };

    template <class K, class V, class D>
    class MaiterKernel2 : public DSMKernel { //the second phase: iterative processing of the statetable
    public:
        MaiterKernel<K, V, D>* maiter; //user-defined iteratekernel
        // vector<pair<K, V> >* output; //the output buffer    
        typename TypedGlobalTable<K, V, V, D>::Iterator *it2;
        Vertexlock * _vertexlock;
    public:

        void set_maiter(MaiterKernel<K, V, D>* inmaiter) {
            maiter = inmaiter;
            it2 = NULL;
        }

        ~MaiterKernel2() {
            delete it2;
            delete _vertexlock;
        }

        //schedule thread function

        static void* obtain_iterator(void* arg) {

            Thread_Arg<K, V, D>* Arg = (Thread_Arg<K, V, D>*)arg;
            MaiterKernel<K, V, D>* maiter = Arg->_maiter;
            typename TypedGlobalTable<K, V, V, D>::Iterator **it2_address = Arg->it2_address;
            int num_threads = maiter->num_threads;
            TypedGlobalTable<K, V, V, D>* a = maiter->globaltable;
            logstream(LOG_INFO) << "schedule thread  :" << pthread_self() << " run..." << std::endl;
            while (true) {
                //get schedule queue
                *it2_address = a->get_typed_iterator(num_threads, false);

                if (*it2_address == NULL) break;

                pthread_mutex_lock(&Arg->threadm);
                while (Arg->count_wait < maiter->num_threads) {
                    pthread_mutex_unlock(&Arg->threadm);
                    sleep(0.1);
                    // cout << "sleep 0.1" << endl;
                    pthread_mutex_lock(&Arg->threadm);
                }
                Arg->count_wait = 0;
                pthread_mutex_unlock(&Arg->threadm);
                pthread_cond_broadcast(&Arg->threadc);
                pthread_mutex_lock(&Arg->schedulem);
                //       Arg->schedule_wait = true;
                //wait excute thread
                pthread_cond_wait(&Arg->schedulec, &Arg->schedulem);
                pthread_mutex_unlock(&Arg->schedulem);

                delete *it2_address;
            }

            //wait excute thread
            pthread_mutex_lock(&Arg->threadm);
            while (Arg->count_wait < maiter->num_threads) {
                pthread_mutex_unlock(&Arg->threadm);
                sleep(0.1);
                pthread_mutex_lock(&Arg->threadm);
                cout << "迭代睡眠" << std::endl;
            }
            pthread_mutex_unlock(&Arg->threadm);


            pthread_exit(0);
        }

        static void run_iter(MaiterKernel<K, V, D>* maiter, Vertexlock *vertexlock, vector<pair<K, V> >* output, const K& k, V &v1, V &v2, D &v3) {
            vertexlock->lock(k); //lock
            //aim to simmok algorithm
            maiter->iterkernel->process_delta_v(k, v1, v2, v3);

            maiter->globaltable->accumulateF2(k, v1); //perform v=v+delta_v     

            maiter->iterkernel->g_func(k, v1, v2, v3, output); //invoke api, perform g(delta_v) and send messages to out-neighbors

            maiter->globaltable->updateF1(k, maiter->iterkernel->default_v()); //perform delta_v=0, reset delta_v after delta_v has been spread out
            vertexlock->unlock(k); //unlock

            typename vector<pair<K, V> >::iterator iter;
            for (iter = output->begin(); iter != output->end(); iter++) { //send the buffered messages to  statetable
                pair<K, V> kvpair = *iter;
                vertexlock->lock(kvpair.first);
                maiter->globaltable->accumulateF1(kvpair.first, kvpair.second); //apply the output messages to remote state table
                vertexlock->unlock(kvpair.first);
            }
            output->clear(); //clear the output buffer
        }

        static void* run_loop(void* arg) {
            Thread_Arg<K, V, D>* Arg = (Thread_Arg<K, V, D>*)arg;
            MaiterKernel<K, V, D>* maiter = Arg->_maiter;

            typename TypedGlobalTable<K, V, V, D>::Iterator **it2_address = Arg->it2_address;

            vector<pair<K, V> >* output;
            int num_threads = maiter->num_threads;
            TypedGlobalTable<K, V, V, D>* a = maiter->globaltable;
            double totalF2 = 0; //the sum of v, it should be larger and larger as iterations go on
            long updates = 0; //for experiment, recording number of update operations
            output = new vector<pair<K, V> >;
            logstream(LOG_INFO) << "excute thread  :" << pthread_self() << " run..." << std::endl;
            //the main loop for iterative update
            while (true) {
                //wait for schedule thread
                pthread_mutex_lock(&Arg->threadm);
                Arg->count_wait++;
                pthread_cond_wait(&Arg->threadc, &Arg->threadm);
                pthread_mutex_unlock(&Arg->threadm);

                typename TypedGlobalTable<K, V, V, D>::Iterator *it2 = *it2_address;

                if (it2 == NULL) break;


                int64_t pos;
                int64_t begin = it2->thread_begin(pthread_self());
                if (begin == -1) {
                    logstream(LOG_FATAL) << "thread" << pthread_self() << "create begin fail " << std::endl;
                }

                int64_t end = it2->thread_end(pthread_self());
                if (end == -1) {
                    logstream(LOG_FATAL) << "thread" << pthread_self() << "create end fail " << std::endl;
                }

                for (pos = begin; pos <= end; pos++) {

                    if (!it2->is_in_use(pos))
                        continue;
                    run_iter(maiter, Arg->vertexlock, output, it2->key(pos), it2->value1(pos), it2->value2(pos), it2->value3(pos));
                }
                //wait other excute thread
                pthread_barrier_wait(&Arg->barrier);
                //send to schedule thread
                pthread_cond_broadcast(&Arg->schedulec);

            }

            pthread_exit(0);
        }
        //run thead function

        void runexcutethread() {
            pthread_t thread[maiter->num_threads];
            pthread_t iter_thread;

            Thread_Arg<K, V, D> * Arg = new Thread_Arg<K, V, D>();

            Arg->_maiter = maiter;
            Arg->it2_address = &it2;
            Arg->vertexlock = _vertexlock;

            Arg->count_wait = 0;

            pthread_mutex_init(&Arg->threadm, NULL);
            pthread_cond_init(&Arg->threadc, NULL);

            Arg->schedule_wait = false;
            pthread_mutex_init(&Arg->schedulem, NULL);
            pthread_cond_init(&Arg->schedulec, NULL);

            pthread_barrier_init(&Arg->barrier, NULL, maiter->num_threads); //等待



            //create n excute thread
            Timer timer; //for experiment, time recording
            for (int i = 0; i < maiter->num_threads; i++) {
                int err;
                logstream(LOG_INFO) << "create thread  " << i << std::endl;
                if ((err = pthread_create(&thread[i], NULL, run_loop, (void*) Arg) != 0)) {
                    logstream(LOG_FATAL) << "excutethread:" << i << " create error" << endl;
                }

                KVVPAIR iterval = {thread[i], 0, 0};
                thread_interval.push_back(iterval);

            }


            //create schedule thread
            int err;
            logstream(LOG_INFO) << "create iter_thread  " << std::endl;
            if ((err = pthread_create(&iter_thread, NULL, obtain_iterator, (void*) Arg) != 0)) {
                logstream(LOG_FATAL) << "iter_thread create error" << endl;
            }

            logstream(LOG_INFO) << "main thread start... " << std::endl;

            //main thread terminate check
            maiter->globaltable->TermCheck();

            logstream(LOG_INFO) << "over excute threads" << std::endl;

            pthread_join(iter_thread, NULL);

            logstream(LOG_INFO) << "over schedule thread" << std::endl;

            pthread_mutex_destroy(&Arg->threadm);
            pthread_cond_destroy(&Arg->threadc);
            pthread_mutex_destroy(&Arg->schedulem);
            pthread_cond_destroy(&Arg->schedulec);

            pthread_barrier_destroy(&Arg->barrier);

            delete Arg;
            total_time_2 = timer.elapsed();

        }

        void run() {//run second phase

            logstream(LOG_INFO) << "start performing iterative update" << std::endl;
            logstream(LOG_INFO) << "create " << maiter->line << "mutex" << std::endl;

            _vertexlock = new Vertexlock(maiter->line);

            _vertexlock->Initmutex();

            runexcutethread();

            _vertexlock->Destroymutex();



            logstream(LOG_INFO) << "over iterative " << std::endl;
        }
    };

    template <class K, class V, class D>
    struct Dump_Arg {
        MaiterKernel<K, V, D>* _maiter;
        double* totalF1;
        double* totalF2;
        int shard;
    };

    template <class K, class V, class D>
    class MaiterKernel3 : public DSMKernel { //the third phase: dumping the result, write the in-memory table to disk
    private:
        MaiterKernel<K, V, D>* maiter; //user-defined iteratekernel
    public:

        void set_maiter(MaiterKernel<K, V, D>* inmaiter) {
            maiter = inmaiter;

        }

        ~MaiterKernel3() {
        }

        void dump(TypedGlobalTable<K, V, V, D>* a) {

            Timer timer;
            double totalF1 = 0; //the sum of delta_v, it should be smaller enough when iteration converges  
            double totalF2 = 0; //the sum of v, it should be larger enough when iteration converges
            fstream File; //the output file containing the local state table infomation

            string file = StringPrintf("%s/result_graph", maiter->output.c_str()); //the output path
            File.open(file.c_str(), ios::out);

            //get the iterator of the local state table
            typename TypedGlobalTable<K, V, V, D>::Iterator *it = a->get_entirepass_iterator();

            while (!it->done()) {
                bool cont = it->Next();
                if (!cont) break;
                totalF1 += it->value1();
                totalF2 += it->value2();
                File << it->key() << "\t" << it->value2() << "\n";
            }
            delete it;

            File.close();
            total_time_3 = timer.elapsed();
            logstream(LOG_INFO) << "dumpthread  over" << std::endl;

            sleep(1);

            cout << "\n";
            logstream(LOG_INFO) << "---------------------------------" << std::endl;
            cout << "\n";
            logstream(LOG_INFO) << "kernel information:" << std::endl;
            logstream(LOG_INFO) << "first phase(initialize statetable):run--> total_time : " << total_time_1 << std::endl;
            logstream(LOG_INFO) << "second phase(iterative processing ):run--> total_time : " << total_time_2 << std::endl;
            logstream(LOG_INFO) << "third phase(dumping result):run--> total_time : " << total_time_3 << std::endl;
            logstream(LOG_INFO) << "total times : " << total_time_1 + total_time_2 + total_time_3 << std::endl;

            logstream(LOG_INFO) << "total delta : " << totalF1 << endl;
            logstream(LOG_INFO) << "total value : " << totalF2 << endl;
        }

        void run() {//run third phase
            logstream(LOG_INFO) << "dumping result" << std::endl;

            dump(maiter->globaltable);

        }
    };

    typedef std::map<std::string, std::string> Map_Pair;

    template <class K, class V, class D>
    class MaiterKernel {
    public:
        int num_threads;
        int64_t num_nodes;
        double schedule_portion;
        string input;
        string output;
        int line;
        Sharder<K> *sharder;
        IterateKernel<K, V, D> *iterkernel;
        TermChecker<K, V> *termchecker;
        TypedGlobalTable<K, V, V, D> *globaltable;

        MaiterKernel() {
            Reset();
        }

        MaiterKernel(
                Sharder<K>* insharder, //the user-defined partitioner
                IterateKernel<K, V, D>* initerkernel, //the user-defined iterate kernel
                TermChecker<K, V>* intermchecker) { //the user-defined terminate checker
            Reset();

            num_threads = get_config_option_int("THREADS", 0);
            num_nodes = get_config_option_long("NODES", 0); //local state table size
            schedule_portion = get_config_option_double("PORTION", 0); //priority scheduling, scheduled portion
            input = get_config_option_string("INPUT", "input/pagerank");
            output = get_config_option_string("RESULT", "input/pagerank"); //output dir
            sharder = insharder; //the user-defined partitioner
            iterkernel = initerkernel; //the user-defined iterate kernel
            termchecker = intermchecker; //the user-defined terminate checker
            termcheck_interval = get_config_option_double("TERMCHECKINTERVAL", 1.0); //the user-defined termcheck_interval
            termcheck_threshold = get_config_option_double("TERMTHRESH", 0); //the user-defined termcheck_threshold
        }

        ~MaiterKernel() {
            delete sharder;
            delete iterkernel;
            delete termchecker;
            delete globaltable;
        }

        void Reset() {
            num_nodes = 0;
            schedule_portion = 1;
            output = "result";
            sharder = NULL;
            iterkernel = NULL;
            termchecker = NULL;
            line = 0;

        }


    public:

        void registerMaiter() {
            //create globaltable
            globaltable = new TypedGlobalTable<K, V, V, D>(schedule_portion, sharder, termchecker, line);

            if (schedule_portion < 1) {
                logstream(LOG_INFO) << "schedule_portion < 1,select schedule iterator  " << std::endl;
            } else {
                logstream(LOG_INFO) << "schedule_portion = 1,select normal iterator  " << std::endl;
            }

            logstream(LOG_INFO) << "num_threads:" << num_threads << std::endl;
            logstream(LOG_INFO) << "termcheck_interval:" << termcheck_interval << std::endl;
            logstream(LOG_INFO) << "termcheck_threshold:" << termcheck_threshold << std::endl;

        }
    };

    template <class K, class V, class D>
    class RunMaiterKernel {
    public:
        MaiterKernel<K, V, D>* maiterkernel;
        MaiterKernel1<K, V, D>* maiterkernel1;
        MaiterKernel2<K, V, D>* maiterkernel2;
        MaiterKernel3<K, V, D>* maiterkernel3;

        RunMaiterKernel(MaiterKernel<K, V, D>* maiterkernel) : maiterkernel(maiterkernel) {
        }
        //start run Maiter
        void runMaiter() {
            maiterkernel->registerMaiter();

            maiterkernel1 = new MaiterKernel1<K, V, D>();

            maiterkernel1->set_maiter(maiterkernel);

            maiterkernel2 = new MaiterKernel2<K, V, D>();

            maiterkernel2->set_maiter(maiterkernel);

            maiterkernel3 = new MaiterKernel3<K, V, D>();

            maiterkernel3->set_maiter(maiterkernel);

            maiterkernel1->run();

            maiterkernel2->run();

            maiterkernel3->run();



        }

        ~RunMaiterKernel() {
            logstream(LOG_INFO) << "free  space." << std::endl;
            logstream(LOG_INFO) << "Exiting." << std::endl;
            delete maiterkernel;
            delete maiterkernel1;
            delete maiterkernel2;
            delete maiterkernel3;

        }

    };


}


#endif	/* KERNEL_H */

