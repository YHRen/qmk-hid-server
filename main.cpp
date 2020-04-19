#include <vector>
#include <iostream> 
#include <string>
#include <future>
#include <chrono>
#include <thread>
#include <cstring>
#include <cmath>

/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/*
 * Ugly hack to work around failing compilation on systems that don't
 * yet populate new version of hidraw.h to userspace.
 */
#ifndef HIDIOCSFEATURE
#warning Please have your distro update the userspace kernel headers
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif


/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* libudev */
//#include <libudev.h>

using namespace std;

std::string exec(const char* cmd);

char const * cpu = "grep 'cpu ' /proc/stat | awk '{usage=($2+$4)/($2+$4+$5)*100} END {print usage}'";
char const * gpu = "nvidia-smi --query-gpu=utilization.gpu --format=csv | tail -n 1 | cut -f1 -d' '";
char const * gem = "nvidia-smi --query-gpu=memory.used,memory.total --format=csv | tail -n 1 | tr ',' ' ' | awk '{ print $1/$3*100}'";
char const * mem = "free | grep 'Mem' | awk '{usage=($3/$2)*100} END {print usage}'";
char const * net = "cat /proc/net/dev | awk '/wlp7s0/ {rx[1]=$2; tx[1]=$10};\
                    END {print rx[1] , tx[1]}'";
char const * net_rcv = "cat /proc/net/dev | grep 'wlp7s0' | tr -s ' ' | cut -d' ' -f2";
char const * net_snd = "cat /proc/net/dev | grep 'wlp7s0' | tr -s ' ' | cut -d' ' -f10";

int str2int(const string & s);
int get_cpu();
int get_mem();
int get_gpu();
int get_gem();
int get_net(int);
vector<uint8_t> quantile(int x);

const char *bus_str(int bus);
int print_info(const char * device);

int main( int argc, char * argv[] ){
  auto start = chrono::steady_clock::now();
  const char * device = "/dev/hidraw3";
  int fd = print_info(device);
  if(fd==-1) return 1;
  int flag = 100;
  int off_set = 2;
  vector<int> vals(5,0);
  vector<vector<uint8_t>> chunks(5, vector<uint8_t>(5,0));
  char msg[32]={0};
  while(1){
    future<int> fnet = async(get_net, 100);
    vals[0] = get_cpu();
    vals[1] = get_mem();
    vals[2] = get_gpu();
    vals[3] = get_gem();
    vals[4] = fnet.get();
    for(int i = 0; i < 5; ++i){
      chunks[i] = quantile(vals[i]);
    }
    for(int i = 0; i < 5; ++i){
      for(int j = 0; j < 5; ++j){
        msg[off_set+i*5+j] = chunks[j][i];
      }
    }
    int res = write(fd, msg, 32);
    if (res < 0) {
        printf("Error: %d\n", errno);
        perror("write");
    } else {
        //printf("write() wrote %d bytes\n", res);
    }
    sleep(1);
    flag --;
  }
  auto end = chrono::steady_clock::now();
  chrono::duration<double, milli> dif = end - start;
  cout << " duration(ms) " << dif.count()  << '\n'; 
  return 0;
}


std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}


int print_info(const char * device){
  // [linux hidraw example](https://github.com/torvalds/linux/blob/master/samples/hidraw/hid-example.c)
	int fd;
	int i, res, desc_size = 0;
	char buf[256];
	struct hidraw_report_descriptor rpt_desc;
	struct hidraw_devinfo info;
	fd = open(device, O_RDWR|O_NONBLOCK);

	if (fd < 0) {
		perror("Unable to open device");
		return -1;
	}

	memset(&rpt_desc, 0x0, sizeof(rpt_desc));
	memset(&info, 0x0, sizeof(info));
	memset(buf, 0x0, sizeof(buf));

	/* Get Report Descriptor Size */
	res = ioctl(fd, HIDIOCGRDESCSIZE, &desc_size);
	if (res < 0)
		perror("HIDIOCGRDESCSIZE");
	else
		printf("Report Descriptor Size: %d\n", desc_size);

	/* Get Report Descriptor */
	rpt_desc.size = desc_size;
	res = ioctl(fd, HIDIOCGRDESC, &rpt_desc);
	if (res < 0) {
		perror("HIDIOCGRDESC");
	} else {
		printf("Report Descriptor:\n");
		for (i = 0; i < rpt_desc.size; i++)
			printf("%hhx ", rpt_desc.value[i]);
		puts("\n");
	}
    
	/* Get Physical Location */
	res = ioctl(fd, HIDIOCGRAWPHYS(256), buf);
	if (res < 0)
		perror("HIDIOCGRAWPHYS");
	else
		printf("Raw Phys: %s\n", buf);
    
	/* Get Raw Info */
	res = ioctl(fd, HIDIOCGRAWINFO, &info);
	if (res < 0) {
		perror("HIDIOCGRAWINFO");
	} else {
		printf("Raw Info:\n");
		printf("\tbustype: %d (%s)\n",
			info.bustype, bus_str(info.bustype));
		printf("\tvendor: 0x%04hx\n", info.vendor);
		printf("\tproduct: 0x%04hx\n", info.product);
	}
	return fd;
}

int str2int(const string & s){
  int res = int(stof(s));
  return  min(res, 100);
}

int get_cpu(){ return str2int(exec(cpu)); }
int get_mem(){ return str2int(exec(mem)); }
int get_gpu(){ return str2int(exec(gpu)); }
int get_gem(){ return str2int(exec(gem)); }
int get_net(int interval=100){ 
  auto r1 = stoull(exec(net_rcv));
  auto s1 = stoull(exec(net_snd));
  this_thread::sleep_for(chrono::milliseconds(interval));
  auto r2 = stoull(exec(net_rcv));
  auto s2 = stoull(exec(net_snd));
  double res = (double)((r2-r1)+(s2-s1))/interval; // kB/s
  // kB to MB to 1,100
  return min(int(res*0.1), 100);
}

vector<uint8_t> quantile(int x){
  x /= 5; // 
  vector<uint8_t> ans(5, 0);
  for(int i = 0; i < 5; ++i){
    ans[i] = (uint8_t)max(0, x-(4-i)*4);
    ans[i] = (uint8_t)min((int)ans[i], 4);
  }
  return ans;
}

const char * bus_str(int bus)
{
	switch (bus) {
	case BUS_USB:
		return "USB";
		break;
	case BUS_HIL:
		return "HIL";
		break;
	case BUS_BLUETOOTH:
		return "Bluetooth";
		break;
	case BUS_VIRTUAL:
		return "Virtual";
		break;
	default:
		return "Other";
		break;
	}
}
