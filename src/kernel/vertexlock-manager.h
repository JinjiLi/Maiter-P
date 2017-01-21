/* 
 * File:   lock-registry.h
 * 
 * Version 1.0
 *
 * Copyright [2016] [JinjiLi/ Northeastern University]
 * 
 * Author: JinjiLi<L1332219548@163.com>
 * 
 * Created on December 26, 2016, 3:40 PM
 */

#ifndef LOCK_REGISTRY_H
#define	LOCK_REGISTRY_H
#include <pthread.h>
#include <limits.h>
namespace dsm {
   
//vertex lock
    class Vertexlock {
    public:

        Vertexlock(int _num_nodes) : num_nodes(_num_nodes) {

            mutex = new pthread_mutex_t[num_nodes];

        }

        ~Vertexlock() {
            delete [] mutex;
        }

        void Initmutex() {
            int error;
            for (int i = 0; i < num_nodes; i++) {
                error = pthread_mutex_init(&mutex[i], NULL);
                if (error)
                    perror("Error: failed to init mutex");
                assert(!error);
            }

        }

        void Destroymutex() {
            int error;
            for (int i = 0; i < num_nodes; i++) {
                error = pthread_mutex_destroy(&mutex[i]);
                if (error)
                    perror("Error: failed to destroy mutex");
                assert(!error);
            }
        }

        void lock(int node) {
            int error = pthread_mutex_lock(&mutex[node]);
            assert(!error);
        }

        void unlock(int node) {
            int error = pthread_mutex_unlock(&mutex[node]);
            assert(!error);
        }

        bool try_lock(int node) {
            return pthread_mutex_trylock(&mutex[node]) == 0;
        }
    public:
        pthread_mutex_t * mutex;
        int num_nodes;

    };



}


#endif	/* LOCK_REGISTRY_H */

