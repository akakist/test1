#include <string>
#include <mutex>
#include <deque>
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace std;

struct line{
    int lineNo;
    std::string content;
    line():lineNo(0){}
};
struct _mx {
    std::mutex m;
    std::deque<std::string> dq;
    std::condition_variable cv;

};

_mx mx;
std::string templ;
void file_reader(const std::string & filename)
{
    FILE* f=fopen(filename.c_str(),"r");
    while(!feof(f))
    {
        char buf[300];
        char* s=fgets(buf, sizeof (buf),f);
        {
            size_t dq_size;
            {
                std::lock_guard<std::mutex> lk(mx.m);
                dq_size=mx.dq.size();

            }
            while(dq_size>10000)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                {
                    std::lock_guard<std::mutex> lk(mx.m);
                    dq_size=mx.dq.size();
                }

            }
            {
                std::unique_lock<std::mutex> lk(mx.m);
                mx.dq.push_back(s);
            }
            mx.cv.notify_one();
        }

    }

}
void scanner()
{
    while(true)
    {
        std::string sample;
        {
            std::unique_lock<std::mutex> lk(mx.m);
            mx.cv.wait(lk);
            if(mx.dq.size())
            {
                sample=mx.dq[0];
                mx.dq.pop_front();
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }
        bool matched=false;
        int position=-1;
        for(size_t i=0;i< sample.size()-templ.size();i++)
        {
            bool mt=true;
            for(int j=0;j<templ.size();j++)
            {
                if(mt && (templ[j]==sample[i+j] || templ[j]=='?'))
                {
                    mt=true;
                }
                else
                {
                    mt=false;
                    break;
                }
            }
            if(mt){

                position=i;
                matched=true;
                break;
            }

        }
        if(matched)
        {
            printf("");
        }

    }

}

int main()
{
    return 0;
}
