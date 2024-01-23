#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <iomanip>

#include "err/err.h"
#include "opt/opt.h"
#include "read_words/read_words.h"

#include <sys/time.h> // gettimeofday
#include <cstring>

#include <NIDAQmx.h>

#define VERSION "1.0"

/* SPP interface to NI DAQ devices with NIDAQmx library */

/*********************************************************************/

// compare strings
bool
is_cmd(const std::string & cmd, const char *name){
  return strcasecmp(cmd.c_str(), name)==0;
}

void
print_tstamp(){
  struct timeval tv;
  gettimeofday(&tv, NULL);
  std::cout << tv.tv_sec << "." << std::setfill('0') << std::setw(6) << tv.tv_usec;
}

/*********************************************************************/

void
daqmx_err(const int32 res){
  char buf[2048]={'\0'};
  if (!DAQmxFailed(res)) return;
  DAQmxGetExtendedErrorInfo(buf,sizeof(buf));
  std::replace(buf, buf+sizeof(buf), '\n', ' ');
  throw Err() << "DAQmx: " <<  buf;
}

/*********************************************************************/
void
run_command(const std::vector<std::string> &args) {

  // print help
  if (is_cmd(args[0], "help")){
    if (args.size()!=1) throw Err() << "Usage: help";
    std::cout <<
      "help  -- Get list of commands.\n"
      "get_time -- Get current time.\n"
      "*idn? -- Get ID string: \"spp-nidaq " VERSION "\".\n"
      "\n"
      "task_create -- Create new task and return it's ID.\n"
      "task_start <task> -- Start task.\n"
      "task_stop <task> -- Stop task.\n"
      "task_wait <task> <timeout> -- Wait for a running task\n"
      "task_nchans <task> -- Print number of channels in the task\n"
      "task_is_done <task> -- Print 0 or 1 depending on the task status\n"
      "task_clear <task> -- Clear task.\n"
      "device_reset <name> -- Reset device, close all related tasks\n"
      "\n"
      "task_add_chan_aivolt <task> <chan> <vmin> <vmax>"
      "task_set_timing <task> <rate> <count>"
      "task_read_analog <task> <count> <timeout>"
    ;
    return;
  }

  // print time
  if (is_cmd(args[0], "get_time")){
    if (args.size()!=1) throw Err() << "Usage: get_time";
    print_tstamp();
    std::cout << "\n";
    return;
  }

  // print id
  if (is_cmd(args[0], "*idn?")){
    if (args.size()!=1) throw Err() << "Usage: *idn?";
    std::cout << "spp-nidaq " VERSION "\n";
    return;
  }

  /*****************************************/
  // low-level task interface

  if (is_cmd(args[0], "task_create")){
    if (args.size()!=1) throw Err() << "Usage: task_create";
    TaskHandle task = 0;
    daqmx_err(DAQmxCreateTask("", &task));
    std::cout << task << "\n";
    return;
  }

  if (is_cmd(args[0], "task_start")){
    if (args.size()!=2) throw Err()
      << "Usage: task_start <task>";
    TaskHandle task = str_to_type<TaskHandle>(args[1]);
    daqmx_err( DAQmxStartTask(task) );
    return;
  }

  if (is_cmd(args[0], "task_stop")){
    if (args.size()!=2) throw Err()
      << "Usage: task_stop <task>";
    TaskHandle task = str_to_type<TaskHandle>(args[1]);
    daqmx_err( DAQmxStopTask(task) );
    return;
  }

  if (is_cmd(args[0], "task_is_done")){
    if (args.size()!=2) throw Err()
      << "Usage: task_is_done <task>";
    TaskHandle task = str_to_type<TaskHandle>(args[1]);
    bool32 status;
    daqmx_err( DAQmxIsTaskDone(task, &status) );
    std::cout << (int)status << "\n";
    return;
  }

  if (is_cmd(args[0], "task_wait")){
    if (args.size()!=3) throw Err()
      << "Usage: task_wait <task> <timeout>";
    TaskHandle task = str_to_type<TaskHandle>(args[1]);
    float64 timeout = str_to_type<float64>(args[2]);
    bool32 status;
    daqmx_err( DAQmxWaitUntilTaskDone(task, timeout) );
    return;
  }

  if (is_cmd(args[0], "task_clear")){
    if (args.size()!=2) throw Err()
      << "Usage: task_clear <task>";
    TaskHandle task = str_to_type<TaskHandle>(args[1]);
    //daqmx_err( DAQmxStopTask(task) );
    daqmx_err( DAQmxClearTask(task) );
    return;
  }

  if (is_cmd(args[0], "task_nchans")){
    if (args.size()!=2) throw Err() << "Usage: task_nchans <task>";
    TaskHandle task = str_to_type<TaskHandle>(args[1]);
    uInt32 nch = 0;
    daqmx_err( DAQmxGetTaskAttribute(task, DAQmx_Task_NumChans, &nch) );
    std::cout << nch << "\n";
    return;
  }

  if (is_cmd(args[0], "device_reset")){
    if (args.size()!=2) throw Err()
      << "Usage: device_reset <name>";
    daqmx_err( DAQmxResetDevice(args[1].c_str()) );
    return;
  }


  if (is_cmd(args[0], "task_add_chan_aivolt")){
    if (args.size()!=5) throw Err()
      << "Usage: task_add_chan_aivolt <task> <chan> <vmin> <vmax>";

    TaskHandle task = str_to_type<TaskHandle>(args[1]);
    const char *chan = args[2].c_str();
    float64 vmin = str_to_type<float64>(args[3]);
    float64 vmax = str_to_type<float64>(args[4]);

    daqmx_err( DAQmxCreateAIVoltageChan(task, chan,
      "", DAQmx_Val_Cfg_Default, vmin, vmax, DAQmx_Val_Volts, NULL));
    return;
  }

  if (is_cmd(args[0], "task_set_timing")){
    if (args.size()!=4) throw Err()
      << "Usage: task_set_timing <task> <rate> <count>";
    TaskHandle task = str_to_type<TaskHandle>(args[1]);
    float64 rate = str_to_type<float64>(args[2]);
    uInt64 count = str_to_type<uInt64>(args[3]);
    daqmx_err( DAQmxCfgSampClkTiming(task, "",
      rate, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, count));
    return;
  }

  if (is_cmd(args[0], "task_read_analog")){
    if (args.size()!=4) throw Err()
      << "Usage: task_read_analog <task> <count_per_ch> <timeout>";
    TaskHandle task = str_to_type<TaskHandle>(args[1]);
    int32 count = str_to_type<int32>(args[2]);
    float64 timeout = str_to_type<float64>(args[3]);
    auto group = DAQmx_Val_GroupByChannel;

    uInt32 nch = 0;
    daqmx_err( DAQmxGetTaskAttribute(task, DAQmx_Task_NumChans, &nch) );

    float64 * data = new float64[nch*count];
    try {
      int32  read = 0;
      daqmx_err(DAQmxReadAnalogF64(task, count, timeout,
        group, data, nch*count, &read, NULL));
      for (int32 i = 0; i<read; i++){
        for (size_t ch = 0; ch < nch; ch++){
          std::cout << ' ' << data[i*ch + ch];
        }
        std::cout << "\n";
      }
    }
    catch (const Err & e){
      delete[] data;
      throw;
    }
    delete[] data;
    return;
  }

  throw Err() << "Unknown command: " << args[0];
}

/*********************************************************************/

void
print_help() {
  std::cout <<
  "spp-nidaq -- SPP interface to NI DAQ devices (via NIDAQmx library)\n"
  "Usage:\n"
  "   spp-nidaq -h -- Print help message and exit\n"
  "   spp-nidaq -i -- Interactive mode (SPP communication). Type \"help\" to print available commands\n"
  "   spp-nidaq <command> <arguments> ...  -- Execute a single command\n"
  ;
}

int
main(int argc, char *argv[]){

  /* parse  options */
  bool spp = false;
  while(1){
    opterr=0;
    int c = getopt(argc, argv, "hi");
    if (c==-1) break;
    switch (c){
      case '?':
      case ':': throw Err() << "incorrect options, see -h"; /* error msg is printed by getopt*/
      case 'i': spp = true; break;
      case 'h': print_help(); return 1;
    }
  }
  auto args = std::vector<std::string>(argv+optind, argv+argc);

  // Single command mode
  if (!spp){
    if (args.size()<1){
      print_help();
      return 1;
    }
    try {
      run_command(args);
    }
    catch (const Err & E){
      std::cout << "\n#Error: " << E.str() << "\n" << std::flush;
      return 1;
    }
    return 0;
  }

  // Interactive mode
  std::cout <<
    "#SPP001\n" // a command-line protocol, version 001.
    "Type help to see command list.\n"
    "#OK\n"
  ;

  while (!std::cin.eof()){
    try {
      auto args = read_words(std::cin);
      if (args.size()<1) continue;
      run_command(args);
      std::cout << "#OK\n" << std::flush;
    }
    catch (const Err & E){
      std::cout << "\n#Error: " << E.str() << "\n" << std::flush;
    }
  }

  return 0;
}
