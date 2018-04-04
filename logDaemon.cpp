#include <signal.h>
#include <stdlib.h>
#include <mutex>
#include <condition_variable>
#include <string.h>
#include <vector>
#include <sys/time.h> // struct itimeral. setitimer()
#include <time.h>
#include <string>
#include <thread>
#include <iostream>
#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <sys/stat.h>
#include <cstdlib>
#include <iostream>
#include <dirent.h>

using namespace std;
using namespace std::chrono;

static const char ENV_XIAOMENG_DEVICEID[] = "ENV_XIAOMENG_DEVICEID";
static const char ENV_XIAOMENG_DEBUG[] = "ENV_XIAOMENG_DEBUG";
static const char ReportServer_ADDR[] = "http://118.126.96.129";
static const char ReportServer_SUBADDR[] =     "/interflow/locker/log/report";
static const char TrackerServer_SUBADDR[] =    "/interflow/locker/tracker/report";
static const char CURL_SUCCESS_STR[] = "\"code\":\"0\"";

static string gLogdir  = "./logs";
static string gTmpdir  = "./logs_uploaded"; //TODO: temp save the files here, instead of deleting it.
static string gLogfile = "logfile";

static long gTimerInterval=1*60*60;
string gDeviceID = "ENV_XIAOMENG_DEVICEID_UNKNOWN";

vector<string> logfileVect;
mutex  mtx;
condition_variable cv;
bool bExit = false;


const string postfix_log = ".log";
const string postfix_gz = ".log.gz";


std::string get_date() {
	char tp[64] = {'\0'};
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::tm tm = *std::localtime(&t);
	//strftime(tp, 64, "%Y_%m_%d_%H_%M_%S", &tm);
	strftime(tp, 64, "%Y_%m_%d_%H_%M_%S", &tm);
	return std::string(tp);
}

static bool isDebug(){
	const char *debug = getenv(ENV_XIAOMENG_DEBUG);
	if (debug == nullptr){
		cout<<"ENV_XIAOMENG_DEBUG is not set, assume false"<<endl;
		return false;
	}
	if (strcmp(debug, "true") == 0){
		return true;
	}
	return false;
}

string& trim(string &s)
{
	if (s.empty())
	{
		return s;
	}
	s.erase(0,s.find_first_not_of(" "));
	s.erase(s.find_last_not_of(" ") + 1);
	return s;
}



string launch_cmd(const char* cmd){
	FILE *fpipe;
	string result;
	char val[128] = {0};

	if (0 == (fpipe = (FILE*)popen(cmd, "r"))) {
		perror("popen() failed.");
		return "";
	}

	while (fgets(val, sizeof(val)-1,fpipe)) {
		//printf("%s", val);
		result += val;
	}
	if (result.size() > 0 && result[result.size()-1] == '\n')
		//result[result.size()-1]='\0';//tony:wrong!
		result.erase(result.end()-1);

	//cout << result <<endl;
	pclose(fpipe);
	return trim(result);
}

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int listDir(string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    string fname;
    while ((dirp = readdir(dp)) != NULL) {
        switch (dirp->d_type) {
                case DT_REG:
                    fname = dirp->d_name;
                    if (ends_with(fname, postfix_log) || ends_with(fname, postfix_gz) )
                        files.push_back(move(fname));
                    break;
                case DT_DIR:
                    break;
                default:
                    break;
        }
    }
    closedir(dp);
    return 0;
}


//input: logfile. the file name which will be compress/uploaded. without path, only name
int gzipAndUploadLog(const string& logfile){
    string srcfile; //tmp name, with full path
	string filename; //compressed logfile, with full path, which will be uploaded to server
    string tmpname; //tmp dir, with full path, the uploaded file will be moved to such path
    string cmd;

    
    if (ends_with(logfile, postfix_log)){//do compress then upload
        srcfile = gLogdir + "/" + logfile;
        filename = gLogdir + "/" + logfile + ".gz";
        tmpname = gTmpdir + "/" + logfile + ".gz";
        //get gz file
        cmd = "gzip " + srcfile;
        int ret = system(cmd.c_str());
        if (ret){
            cout<<__FUNCTION__<<": fail to execute: " << cmd <<endl;
            return ret;
        }
    }
    else{//already/previously compressed log, upload directly
        filename = gLogdir + "/" + logfile;
        tmpname = gTmpdir + "/" + logfile;
    }

	//get md5
	cmd = string("md5sum ") +  filename + " | awk '{print $1}'";
	string md5 = launch_cmd(cmd.c_str());
	if (md5.empty()){
		cout<<__FUNCTION__<<": fail to execute: " <<cmd <<endl;
		return -1;
	}
    cout<<__FUNCTION__<<": filename="<<filename<<" md5="<<md5<<endl;

	string timeStr = to_string(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    string header;
    header.append(" -H \"Content-Type:application/octet-stream\" ");
    header.append(" -H \"devId:" + gDeviceID + "\"");
    header.append(" -H \"filename:" + filename + "\"");
    header.append(" -H \"time:" + timeStr + "\"");
    header.append(" -H \"md5:" + md5 + "\"");

    cmd = "curl --silent --show-error --request POST";
    cmd.append(" ");
    cmd.append(header);
    cmd.append(" ");
    cmd.append("--data-binary @\""+filename+"\"");
    cmd.append(" ");
    cmd.append(ReportServer_ADDR);
	if (isDebug()){
		cmd += "/test";
	}
    cmd.append(ReportServer_SUBADDR);

//	ret = system(cmd.c_str());
//	if (ret){
//		cout<<__FUNCTION__<<": fail to execute: " << cmd <<endl;
//		return ret;
//	}
    string curl_res = launch_cmd(cmd.c_str());
	if (curl_res.empty()){
        cout<<__FUNCTION__<<": fail to execute: "<<cmd<<endl;
    }
    else{

        cout<<"curl return:"<<curl_res<<endl;
        if (curl_res.find(CURL_SUCCESS_STR) != string::npos){
	        //cout<<__FUNCTION__<<": success to execute: "<<cmd<<endl;
	        cout<<__FUNCTION__<<": success to execute curl upload."<<endl;
            cout<<__FUNCTION__<<": delete uploaded file: "<<filename<<endl;
            /*
            if( remove(filename.c_str()) != 0 ){
                cout<<__FUNCTION__<<": fail to delete file :" <<filename<<endl;
            }
            else{
                cout<<__FUNCTION__<<": success to delete file :" <<filename<<endl;
            }
            */
            if( rename(filename.c_str(), tmpname.c_str()) != 0 ){
                cout<<__FUNCTION__<<": fail to move file:" <<filename<<endl;
            }
            else{
                cout<<__FUNCTION__<<": success to move file:" <<filename<<endl;
            }
        }
        else
            cout<<__FUNCTION__<<": fail to execute: "<<cmd<<endl;
    }
}

volatile sig_atomic_t e_flag = 0;

void timefunc(int sig)                      /* 定时事件代码 */
{
    e_flag = 1;
    fprintf(stderr, "ITIMER_REAL\n");//[%d]\n", n++);
    signal(ITIMER_REAL, timefunc);              /* 捕获定时信号 */
}
void setTimer(long interval_sec){
    struct itimerval value;
    value.it_value.tv_sec=1;
    value.it_value.tv_usec=0;
    value.it_interval.tv_sec=interval_sec;
    value.it_interval.tv_usec=0;
    signal(SIGALRM, timefunc);
    setitimer(ITIMER_REAL, &value, NULL);   /* 定时开始 */
}
void cancelTimer(){
    struct itimerval value;
    value.it_value.tv_sec=0;
    value.it_value.tv_usec=0;
    value.it_interval.tv_sec=0;
    value.it_interval.tv_usec=0;
    setitimer(ITIMER_REAL, &value, NULL);   /* 定时开始 */
}

int renameAndPushVector(string logfile){
    string timetag = get_date();

    string src= logfile;
    string dest = src + "_" + timetag + ".log";
    int ret = rename(src.c_str(), dest.c_str());

    if (ret != 0){
        cout<<"fail to rename " + src + " -> " + dest<<endl;
        return -1;
    }
    {
        cout<<"Push to logfileVect: "<<dest<<endl;
        unique_lock<mutex> lock(mtx);
        logfileVect.push_back(dest);
        cv.notify_all();
    }
    return 0;
}
int renameAndSignal(){
    string timetag = get_date();

    string src= gLogdir + "/" + gLogfile;
    string dest = src + "_" + timetag + ".log";
    int ret = rename(src.c_str(), dest.c_str());

    if (ret != 0){
        cout<<"fail to rename " + src + " -> " + dest<<endl;
        return -1;
    }

    cout<<"success rename: "<<src<<" -> "<<dest<<endl;
    return 0;
}

int main(int argc, char* argv[])
{
    char ch;
    while((ch = getopt(argc, argv, "f:d:t:h")) != -1){
        printf("optind = %d, optopt = %d\n", optind, optopt);
        switch(ch){
            case 'd':
                printf("optidx:%d, optkey:%c, outval:%s\n", optind, ch, optarg);
                gLogdir = optarg;
                break;
            case 'f':
                printf("optidx:%d, optkey:%c, outval:%s\n", optind, ch, optarg);
                gLogfile = optarg;
                break;
            case 't':
                printf("optidx:%d, optkey:%c, outval:%s\n", optind, ch, optarg);
                gTimerInterval = atol(optarg);
                break;
            case 'h':
                cout<<"-d dir: specify log dir"<<endl;
                cout<<"-f logfile: specify log filename prefix"<<endl;
                cout<<"-t time second: specify log upload interval"<<endl;
                cout<<"-n deviceName: specify deviceID"<<endl;

            default:
                break;
        }
    }
    cout<<__FUNCTION__<<": logdir name = " <<gLogdir<<endl;
    cout<<__FUNCTION__<<": logfile name = " <<gLogfile<<endl;
    cout<<__FUNCTION__<<": timer interval = " <<gTimerInterval<<endl;

    string cmd = "mkdir -p ";
    cmd += gLogdir;
    system(cmd.c_str());

    cmd = "mkdir -p ";
    cmd += gTmpdir;
    system(cmd.c_str());

	//get deviceid
	const char *id = getenv(ENV_XIAOMENG_DEVICEID);
	if (id == nullptr){
		cout<<__FUNCTION__<<": ENV_XIAOMENG_DEVICEID is not set, use default instead"<<endl;
	}
	else{
		gDeviceID = string(id);
	}
	cout<<__FUNCTION__<<" ENV_XIAOMENG_DEVICEID="<<gDeviceID<<endl;
    //system("rm -f ./logfile*");

    /*
    thread t = thread([&](){
        cout<<"log thread started"<<endl;
        while( true ){
            vector<string> todoVect;
            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [&](){ return (bExit || !logfileVect.empty()); });
                todoVect = move(logfileVect);
                logfileVect.clear();
            }
            //upload all logs before exit
            cout<<" Number of files to upload: " << todoVect.size()<<endl;
            for (auto s:todoVect){
                uploadLogFile(s);
            }
            if (bExit){
                cout<<"upload thread exit"<<endl;
                return;
            }
        }
        cout<<"log thread stopped"<<endl;
    });
    */
    thread t = thread([&](){
        cout<<"log thread started"<<endl;
        while( true ){
            vector<string> todoVect;
            bool exitFlag = false;
            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [&](){
                        todoVect.clear();
                        listDir(gLogdir, todoVect); //TODO logdirs
                        exitFlag = bExit;
                        cout<<"##@@## wait"<<endl;
                        return (bExit || todoVect.size()>0);
                        });
            }
            cout<<" Number of files to upload: " << todoVect.size()<<endl;
            //upload all logs before exit
            for (auto s:todoVect){
                gzipAndUploadLog(s);
            }
            if (exitFlag){//could not use BExit here, to avoid race-condition, also we need handle the last saved logs.
                cout<<"upload thread exit"<<endl;
                return;
            }
        }
        cout<<"log thread stopped"<<endl;
    });

    setTimer(gTimerInterval);

	ofstream ofs(gLogdir+"/"+gLogfile, ofstream::out);

	long cnt = 0;
	string word;
    cout<<"start to getline"<<endl;
    while(getline(cin, word)){
        cnt++;
        //cout<<"start to write line."<<cnt<<endl;
        //cout<<word<<endl;
		ofs << word << endl;

        if (e_flag){
            e_flag = 0;

            ofs.flush();
            ofs.close();

            {
                std::unique_lock<std::mutex> lock(mtx);
                renameAndSignal();
                cv.notify_all();
            }
            //renameAndPushVector(gLogfile);
            ofs.open(gLogdir+"/"+gLogfile, ofstream::out);

		}
	}

    cancelTimer();
    cout<<"No more data from stdin, quit log daemon"<<endl;
    {
        ofs.flush();
        ofs.close();
        std::unique_lock<std::mutex> lock(mtx);
        renameAndSignal();
        //renameAndPushVector(gLogfile);
    }
    cout<<"request thread join"<<endl;

    {
        std::unique_lock<std::mutex> lock(mtx);
        bExit = true;
        cv.notify_all();
    }

	if (t.joinable())
		t.join();
    cout<<"thread join done"<<endl;

    return 0;
}
