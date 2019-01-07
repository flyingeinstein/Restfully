//
// Created by Colin MacKenzie on 2018-02-27.
//

#ifndef IQANALYTICSSDK_STRESSTESTER_H
#define IQANALYTICSSDK_STRESSTESTER_H


#include <mtobjects.h>
#include <stdio.h>
#include <time.h>  

template<class TFunc>
class StressTester
{
public:
    class result {
    public:
        short code;
        int N;
        double cycles_ms;

        inline result() : code(0), N(0), cycles_ms(0.0) {}
    };

    //static result single(void* pVoid);
    static result sequential(int N, void* pVoid) {
        StressTester<TFunc> st;
        st.data = pVoid;

        result r;
        clock_t started = clock();
        for(int i=0; i<N; i++) {
            TFunc test_func;
            short s = test_func(pVoid);
        }
        r.cycles_ms = 1000.0 * (clock() - started) / CLOCKS_PER_SEC;

        return r;
    }

    static result stressful(int N, int batch_size, void* pVoid) {
        StressTester<TFunc> st;
        st.data = pVoid;

        if(batch_size > N)
            batch_size = N;

        // create and startup the workers
        st.workers = new mt::thread[batch_size];
        for(int i=0; i<batch_size; i++)
            st.workers[i].start();

        // create our executer (our employer class)
        mt::event ready_for_more;
        executer ex(&st, &ready_for_more);

        // now create jobs for the threads
        result r;
        clock_t started = clock();
        while(r.N < N) {
            for (int i = 0; i < N; i++) {
                for (int i = 0; i < batch_size && r.N < N; i++) {
                    mt::thread_state state = st.workers[i].get_state();
                    if (state == mt::THREAD_WAITING || state == mt::THREAD_SLEEPING) {
                        mt::job_task job(&ex, r.N, INFINITE, st.data);
                        if(st.workers[i].assign_job(job, INFINITE)) {
                            r.N++;
                            st.workers[i].wakeup();
                        }
                    }
                }
            }
            if(r.N < N)
                ready_for_more.wait(INFINITE);
        }
        r.cycles_ms = 1000.0 * (clock() - started) / CLOCKS_PER_SEC;

        for(int i=0; i<batch_size; i++)
            st.workers[i].stop();
        delete [] st.workers;

        return r;
    }

private:
    mt::thread* workers;
    void* data;

    inline StressTester()
            : workers(NULL), data(NULL)
    {}


    class executer : public mt::job_employer {
    public:
        StressTester<TFunc>* tester;
        mt::event* send_more;

        executer(StressTester<TFunc>* _tester, mt::event* _send_more) : tester(_tester), send_more(_send_more) {}

        virtual bool prepare_job(size_t ident, void *user_data) {
            return true;
        }

        virtual void run_job(size_t ident, void *pVoid) {
            TFunc test_func;
            short s = test_func(pVoid);
            if(send_more)
                send_more->signal();
        }
    };
};


#endif //IQANALYTICSSDK_STRESSTESTER_H
