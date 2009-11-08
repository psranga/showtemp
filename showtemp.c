// Released under the BSD license
// (C) Ranganathan Sankaralingam <ranga@curdrice.com>

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

static long long unsigned read_reg(int msrdev_fd, int reg_num)
{
 int  status;
 long long unsigned rsl = 0;

 status = lseek(msrdev_fd, reg_num, SEEK_SET);
 if (status == -1) {
  fprintf(stderr, "unable to seek.\n");
  exit(1);
 }

 status = read(msrdev_fd, &rsl, sizeof(long long unsigned));
 if (status == -1) {
  fprintf(stderr,
      "Error reading data from msr file.");
  exit(1);
 }
 return rsl;
}

// returns ptr to static buffer.
const char* get_filename_of_cpu(int cpu_num)
{
  static char msrdev_filename_format[] = "/dev/cpu/%d/msr";
  static char msrdev_filename[24] = "/dev/cpu/4294967296/msr";
  static int buf_size = 24;
  snprintf(msrdev_filename, buf_size, msrdev_filename_format, cpu_num);
  return msrdev_filename;
}

// dont bother parsing /proc/cpuinfo. Just keep opening the
// file we expect to find.
int compute_num_cpus()
{
  int rsl = 0;
  int cpu_num = 0;
  for (cpu_num = 0; cpu_num < INT_MAX; ++cpu_num) {
    const char* msrdev_filename = get_filename_of_cpu(cpu_num);
    int fd = open(msrdev_filename, O_RDONLY);
    if (fd != -1) {
      ++rsl;
      close(fd);
    } else {
      break;
    }
  }
  return rsl;
}

long long unsigned reg_val(int cpu_num, int reg_num)
{
  const char* msrdev_filename = get_filename_of_cpu(cpu_num);
  int fd = open(msrdev_filename, O_RDONLY);
  assert( fd != -1 );
  long long unsigned val = read_reg(fd, reg_num);
  close(fd);
  return val;
}

int temp_val( int cpu_num )
{
  // 19c is register for temperature
  long long unsigned raw_val = reg_val( cpu_num, 0x19c );
  // temp is in bits 16-24
  long long unsigned mask = 0x00ff0000;
  long long unsigned masked_val = raw_val & mask;
  long long unsigned temp = masked_val >> 16;
  assert( temp < 256 );
  return temp;
}

void print_rpt(int num_cpus, int Tjmax)
{
  int cpu_num = 0;
  for (cpu_num = 0; cpu_num < num_cpus; ++cpu_num) {
    int temp = Tjmax - temp_val( cpu_num );
    printf("%scpu %d temp %d", (cpu_num > 0 ? " | " : ""), cpu_num, temp);
  }
  printf("\n");
}

int main(int argc, char* argv[])
{
  int num_cpus = 0;
  int Tjmax = 100;
  int delay = 1;
  int opt;
  int Tjmax_set = 0;
  int num_prints = 1;
  int curr_iter = 0;
  int quiet = 0;

  while ((opt = getopt(argc, argv, "qt:d:n:")) != -1) {
    switch (opt) {
      case 'q':
        quiet = 1;
        break;
      case 't':
        Tjmax = atoi(optarg);
        Tjmax_set = 1;
        break;
      case 'd':
        delay = atoi(optarg);
        break;
      case 'n':
        num_prints = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Syntax: %s -t <Tjmax> -d <delay> -n <num_iters>\n", argv[0]);
        exit(1);
        break;
    }
  }
  
  if ( !Tjmax_set) {
    printf("Assuming Tjmax of %dC. This is correct for Core 2 Duo/Quad.\n", Tjmax);
  }

  curr_iter = 0;
  num_cpus = compute_num_cpus();
  while ( (num_prints == 0) || (curr_iter < num_prints) ) {
    ++curr_iter;
    print_rpt(num_cpus, Tjmax);
    if ( curr_iter != num_prints ) {
      sleep(delay);
    }
  }

  return 0;
}

