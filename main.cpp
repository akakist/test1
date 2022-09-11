#include <string>
#include <mutex>
#include <deque>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <vector>

using namespace std;

struct line{
    int lineNo;
    std::string content;
    line():lineNo(0){}
};
struct _mx {
    std::mutex m;
    std::deque<line> dq;
    std::condition_variable cv;
    bool file_done;
    _mx():file_done(false){}
};

_mx mx;

/// worker to read lines from file & put them into mx.dq
void file_reader(const std::string & filename)
{
    FILE* f=fopen(filename.c_str(),"r");
    int idx=0;
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
            /// max size of dq - 10000 elements. Wait for handle elements by scanners
            while(dq_size>10000)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                {
                    std::lock_guard<std::mutex> lk(mx.m);
                    dq_size=mx.dq.size();
                }

            }
            if(s){
                std::unique_lock<std::mutex> lk(mx.m);
                line l;
                l.lineNo=idx;
                l.content=s;
                mx.dq.emplace_back(l);
            }
            mx.cv.notify_one();
            idx++;
        }

    }
    {
        std::lock_guard<std::mutex> lk(mx.m);
        mx.file_done=true;

    }
    mx.cv.notify_all();
    return;

}

/// worker to scan line for matching template & print out result
void scanner(const std::string & templ)
{
    while(true)
    {
        line sample;
        {
            std::unique_lock<std::mutex> lk(mx.m);
            if(mx.dq.size())
            {
                sample=mx.dq[0];
                mx.dq.pop_front();
            }
            else
            {
                if(mx.file_done)
                    return;
                mx.cv.wait(lk);
                continue;
            }
        }
        bool matched=false;
        int position=-1;
        if(sample.content.size()>=templ.size())
        {
            for(size_t is=0;is< sample.content.size()-templ.size();is++)
            {
                bool mt=true;
                for(size_t jt=0;jt<templ.size();jt++)
                {
                    if(mt && (templ[jt]==sample.content[is+jt] || templ[jt]=='?'))
                    {
                    }
                    else
                    {
                        mt=false;
                        break;
                    }
                }
                if(mt){

                    position=is;
                    matched=true;
                    break;
                }

            }
        }

        if(matched)
        {
            printf("%d %d %s\n", sample.lineNo, position, sample.content.substr(position,templ.size()).c_str());
        }

    }

}

#define SCANNER_COUNT 10
int main(int argc, char *argv[])
{
    if(argc!=3)
    {
        printf("usage: %s <filename> <template>\n",argv[0]);
        return 1;
    }
    std::string templ=argv[2];
    std::vector<std::thread> vt;
    for(int i=0;i<SCANNER_COUNT;i++)
        vt.emplace_back(scanner,templ);

    std::thread rd(file_reader,argv[1]);
    rd.join();
    mx.cv.notify_all();
    for(auto &z: vt)
    {
        z.join();
    }
    return 0;
}
