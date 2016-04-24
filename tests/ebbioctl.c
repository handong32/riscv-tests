#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "usr/include/fixedfann.h"
#include "src/main/c/xfiles-user.h"

#define handle_error(msg)                                                      \
    do {								\
	perror(msg);							\
	exit(EXIT_FAILURE);						\
    } while (0)

#define read_csr(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define BUFFER_LENGTH 256           ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH]; ///< The receive buffer from the LKM

#include "/home/handong/github/riscv-tests/tests/dev_ioctl.h"

typedef struct {
  int cycles;
  int last;
  int mse;
  int performance;
  int ant_info;
  int incremental;
  int bit_fail;
  int ignore_limits;
  int percent_correct;
  int watch_for_errors;
  int incremental_fake;
  int verbose;
} flags_t;

flags_t flags;
uint64_t cycles, connections_per_epoch;
int32_t learn_rate, weight_decay;
asid_type asid;
nnid_type nnid;
int max_epochs, exit_code, num_bits_failing, batch_items, num_correct,
    binary_point_width, binary_point_offset, binary_point,
    mse_reporting_period, bit_fail_reporting_period,
    percent_correct_reporting_period, epoch;
double bit_fail_limit, mse_fail_limit, learning_rate, weight_decay_lambda,
    mse, error;
char * file_nn, * file_train, * file_video_string, id[100];
FILE * file_video;
asid_nnid_table * table;
element_type * outputs, * outputs_old;
struct fann_train_data * data;
size_t num_input, num_output;
double multiplier;
// Train on the provided data
tid_type tid;

void writedev(int fd, char *s, int len) {
  int ret = write(fd, s, len); // Send the string to the LKM
  if (ret < 0) {
    perror("Failed to write the message to the device.");
    exit(0);
  }
}

double to_float(element_type value) {
  return value / multiplier;
}

int bit_fail(size_t item, size_t idx) {
  double diff = to_float(outputs[idx] - data->output[item][idx]);
  return fabs(diff > bit_fail_limit);
}

double compute_err(size_t item, size_t idx) {
  return to_float(outputs[idx] - data->output[item][idx]);
}

double compute_mse_change(size_t item, size_t idx) {
  double new_value = to_float(outputs[idx]);
  double old_value = to_float(outputs[item * num_output + idx]);
  return fabs(new_value - old_value);
}

void test_init(int bp_w, int bp_o) {
  // Zero all the flags
  flags = (const flags_t){ 0 };
  
  asid = 0;
  nnid = 0;
  binary_point_width = bp_w;
  binary_point_offset = bp_o;
  exit_code = 0;
  max_epochs = 10000;
  num_bits_failing = -1;
  batch_items = -1;
  binary_point = -1;
  mse_reporting_period = 1;
  bit_fail_reporting_period = 1;
  percent_correct_reporting_period = 1;
  // Double inits
  bit_fail_limit = 0.05;
  mse_fail_limit = -1.0;
  learning_rate = 0.7;
  weight_decay_lambda = 0.0;
  mse = 0.0;
  // Files
  file_video_string = NULL;
  // Strings
  file_nn = NULL;
  file_train = NULL;
  file_video = NULL;
  id[0] = '0';
  id[1] = 0;
  // Other
  table = NULL;
  outputs = NULL;
  outputs_old = NULL;
  data = NULL;
}

int binary_config_read_binary_point(xlen_t *addr, int binary_point_width) {
  int8_t tmp;
  memcpy(&tmp, (void*)addr, sizeof(uint8_t));
  tmp &= ~(~0 << binary_point_width);

  return tmp;
}

int xfiles_eval_batch(int item_start, int item_stop) {
  if (item_start < 0 || item_stop > batch_items)
    return -1;

  num_bits_failing = 0;
  num_correct = 0;
  mse = 0;

  for (int item = 0; item < batch_items; item++) {
    tid = new_write_request(nnid, FEEDFORWARD, 0);
    if (tid < 0) return tid;
    exit_code =
        write_data(tid, (element_type *) data->input[item], num_input);
    if (exit_code)
      return exit_code;
    read_data_spinlock(tid, outputs, num_output);

    if (flags.verbose) {
      printf("[INFO] ");
      for (int i = 0; i < num_input; i++)
        printf("%8.5f ", to_float(data->input[item][i]));
    }

    int correct = 1;
    for (int i = 0; i < num_output; i++) {
      double output_float = to_float(outputs[i]);
      if (flags.verbose)
        printf("%8.5f ", output_float);

      int bit_failure = bit_fail(item, i);
      num_bits_failing += bit_failure;
      if (bit_failure)
        correct = 0;

      if (flags.mse || mse_fail_limit != -1) {
        error = compute_err(item, i);
        mse += error * error;
      }

      if (flags.watch_for_errors && epoch > 0) {
        double change = compute_mse_change(item, i);
        if (change > 0.1)
          printf("\n[ERROR] Epoch %d: Output changed by > 0.1 (%0.5f)",
                 epoch, change);
      }
      if (file_video)
        fprintf(file_video, "%f ", output_float);
      outputs_old[item * num_output + i] = outputs[i];
    }
    num_correct += correct;
    if (file_video)
      fprintf(file_video, "\n");
    if (flags.verbose) {
      if (item < batch_items - 1)
        printf("\n");
    }
  }

  if (flags.verbose)
    printf("%5d\n\n", epoch);

  if (flags.mse || mse_fail_limit != -1) {
    mse /= batch_items * num_output;
    if (flags.mse && (epoch % mse_reporting_period == 0))
      printf("[STAT] epoch %d id %s bp %d mse %8.8f\n", epoch, id,
             binary_point, mse);
  }

  if (flags.bit_fail && (epoch % bit_fail_reporting_period == 0))
    printf("[STAT] epoch %d id %s bp %d bfp %8.8f\n", epoch, id,
           binary_point, 1 - (double) num_bits_failing /
           num_output / batch_items);

  if (flags.percent_correct &&
      (epoch % percent_correct_reporting_period == 0))
    printf("[STAT] epoch %d id %s bp %d perc %8.8f\n", epoch, id,
           binary_point,
           (double) num_correct / batch_items);

  if (!flags.ignore_limits &&
      (num_bits_failing == 0 || mse < mse_fail_limit))
    return 1;

  return 0;
}

xlen_t my_write_data(tid_type tid, element_type * data, size_t count) {
    printf("\nmy_write_data\n");
  const size_t shift = sizeof(xlen_t) * 8 - RESP_CODE_WIDTH;
  xlen_t out;

  // There are two types of writes available to users determined by
  // whether or not "isLast" (bit 2) is set. We write all but the last
  // data value with "isLast" deasserted (funct == 1). The tid goes in
  // rs1 and data goes in rs2.
  int write_index = 0;
  while (write_index != count - 1) {
    asm volatile ("custom0 %[out], %[rs1], %[rs2], %[type]"
                  : [out] "=r" (out)
                  : [rs1] "r" (tid), [rs2] "r" (data[write_index]),
                   [type] "i" (WRITE_DATA));
    printf("1 data = %p\n", data[write_index]);
    int exit_code = out >> shift;
    switch (exit_code) {
      case resp_OK: write_index++; continue;
      case resp_QUEUE_ERR: continue;
      default: return exit_code;
    }
  }

  printf("write_index = %d shift = %d\n", write_index, (int)shift);

  // Finally, we write the last data value with "isLast" set (funct ==
  // 5). When the X-Files Arbiter sees this "isLast" bit, it enables
  // execution of the transaction.
  while (1) {
    asm volatile ("custom0 %[out], %[rs1], %[rs2], %[type]"
                  : [out] "=r" (out)
                  : [rs1] "r" (tid), [rs2] "r" (data[write_index]),
                   [type] "i" (WRITE_DATA_LAST));
    printf("2 data = %p\n", data[write_index]);
    int exit_code = out >> shift;
    switch (exit_code) {
      case resp_OK: return 0;
      case resp_QUEUE_ERR: continue;
      default: return exit_code;
    }
  }

  printf("QUIT\n");
}

xlen_t my_write_data_train_incremental(tid_type tid, element_type * input,
                                    element_type * output, size_t count_input,
                                    size_t count_output) {
  // Simply write the exepcted outputs followed by the inputs.
  xlen_t out = 0;
  if ((out = my_write_data(tid, output, count_output))) return out;
  if ((out = my_write_data(tid, input, count_input))) return out;
  return 0;
}


int xfiles_train_batch(int item_start, int item_stop) {
  if (item_start < 0 || item_stop > batch_items)
    return -1;
  
  printf("xfiles_train_batch\n");
  
  tid = new_write_request(nnid, TRAIN_BATCH, 0);
  printf("new_write_request\n");
  if (tid < 0) return tid;
  write_register(tid, xfiles_reg_batch_items, batch_items);
  printf("xfiles_reg_batch_items\n");
  write_register(tid, xfiles_reg_learning_rate, learn_rate);
  printf("xfiles_reg_learning_rate\n");
  write_register(tid, xfiles_reg_weight_decay_lambda, weight_decay);
  printf("xfiles_reg_weight_decay_lambda\n");

  printf("%d %d\n", item_start, item_stop);
  for (int i = item_start; i < item_stop; ++i) {
      /* printf("tid=%ld, num_input=%ld num_output=%ld\n", (long unsigned int)tid, (long unsigned int)num_input, (long unsigned int)num_output); */
      /* for(int j = 0; j < num_output; j++) */
      /* { */
      /* 	  printf("%d\n", ((int*)data->output[i])[j]); */
      /* } */

    // Write the output and input data
    if ((exit_code = my_write_data_train_incremental(
            tid, (element_type *) data->input[i],
            (element_type *) data->output[i], num_input, num_output)))
      return exit_code;
    // Blocking read
    printf("blocking spinlock\n");
    read_data_spinlock(tid, outputs, num_output);
    printf("blocking spinlock done\n");
  }
  return 0;
}

int xfiles_batch_verbose() {
  cycles = read_csr(0xc00);
  printf("cycles: %ld\n", cycles);

  for (epoch = 0; epoch < max_epochs; epoch++) {
    if ((exit_code = xfiles_train_batch(0, batch_items)))
      return exit_code;
    if ((exit_code = xfiles_eval_batch(0, batch_items)))
      return exit_code;
  }
  cycles = read_csr(0xc00) - cycles;
  return exit_code;
}

int main() {
  int ret, fd;
  char s[BUFFER_LENGTH];

  test_init(3, 7);
  max_epochs = 1;
  flags.mse = 1;
  mse_reporting_period = 10;
  flags.incremental = 0;
  
  // set xs bits
  xfiles_dana_id(1);

  printf("Starting device test code example...\n");
  fd = open("/dev/ebbchar", O_RDWR); // Open the device with read/write access
  if (fd < 0) {
    perror("Failed to open the device...");
    return errno;
  }

  strcpy(s, "createant");
  writedev(fd, s, strlen(s));

  strcpy(s, "showant");
  writedev(fd, s, strlen(s));

  struct stat stat;
  int nfd;
  xlen_t fbytes, file_size;
  nfd = open("xorSigmoidSymmetric-fixed.16bin", O_RDONLY);
  fstat(nfd, &stat);

  fbytes = (xlen_t)stat.st_size;
  file_size = stat.st_size / sizeof(xlen_t);
  file_size += (stat.st_size % sizeof(xlen_t)) ? 1 : 0;
  // printf("file_size %d\n", file_size);

  xlen_t *addr;
  addr = (xlen_t *)mmap(NULL, fbytes, PROT_READ, MAP_SHARED | MAP_LOCKED, nfd, 0);
  if (addr == MAP_FAILED) handle_error("mmap");
 
  ret = ioctl(fd, IOCTL_SET_FILESIZE, &fbytes);
  if (ret < 0) handle_error("IOCTL_SET_FILESIZE");

  printf("virt addr: %p\n", addr);
  ret = ioctl(fd, IOCTL_SET_NN, &addr);
  if (ret < 0) handle_error("IOCTL_SET_NN");

  // Read the binary point and make sure its sane
  binary_point =
      binary_config_read_binary_point(addr, binary_point_width) +
      binary_point_offset;
  if (binary_point < binary_point_offset) {
      handle_error("[ERROR] Binary point looks bad, exiting\n\n");
  }
  printf("[INFO] Found binary point %d\n", binary_point);
  
  data = fann_read_train_from_file("xorSigmoidSymmetric-fixed.train");
  if (data == NULL) handle_error("fann_read_train_from_file");
  
  num_input = data->num_input;
  num_output = data->num_output;
  printf("num data = %d\n", data->num_data);

  data->input[0][0] = -16384;
  data->input[0][1] = -16384;
  data->output[0][0] = -16384;
  
  data->input[1][0] = -16384;
  data->input[1][1] = 16384;
  data->output[1][0] = 16384;

  data->input[2][0] = 16384;
  data->input[2][1] = -16384;
  data->output[2][0] = 16384;

  data->input[3][0] = 16384;
  data->input[3][1] = 16384;
  data->output[3][0] = -16384;

  for(int k = 0; k < data->num_data; k++)
  {
      printf("\ninput:\n");
      for(int ni = 0; ni < data->num_input; ni++)
      {
	  printf("input[%d][%d] = %d ", k, ni, data->input[k][ni]);
      }

      printf("\noutput:\n");
      for(int no = 0; no < data->num_output; no++)
      {
	  printf("output[%d][%d]%d ", k, no, data->output[k][no]);
      }

  }

  printf("\n[INFO] Done reading input file, %ld %ld\n", num_input, num_output);

  multiplier = pow(2, binary_point);

  outputs = (element_type *) malloc(num_output * sizeof(element_type));
  if (batch_items == -1)
    batch_items = fann_length_train_data(data);
  if (batch_items > fann_length_train_data(data))
    batch_items = fann_length_train_data(data);
  outputs_old = (element_type *)
    malloc(num_output * batch_items * sizeof(element_type));

  learn_rate = (int32_t) (learning_rate * multiplier);

  weight_decay = (int32_t) (weight_decay_lambda * multiplier);
  if (!flags.incremental) {
    learn_rate /= batch_items;
    weight_decay /= batch_items;
  }
  
  // weight_decay = 1;
  if (learn_rate == 0) {
    printf("[ERROR] Number of batch items forces learning rate increase\n");
  }
  printf("[INFO] Computed learning rate is 0x%x\n", learn_rate);
  
  ret = ioctl(fd, IOCTL_SHOW_ANT);
  if (ret < 0) handle_error("IOCTL_SHOW_ANT");

  ret = ioctl(fd, IOCTL_PHYS_ADDR);
  if (ret < 0) handle_error("IOCTL_PHYS_ADDR");

  xfiles_batch_verbose();

  if (exit_code) {
      printf("[ERROR] Saw exit code %d\n", exit_code);
  }

  close(nfd);
  close(fd);

  printf("End of the program\n");
  return 0;
}

/*

  ssize_t s2;

    s2 = write(STDOUT_FILENO, addr, fbytes);
    if (s2 != fbytes) {
        if (s2 == -1)
            handle_error("write");

        fprintf(stderr, "partial write");
        exit(EXIT_FAILURE);
    }


  xlen_t *buf;
  buf = (xlen_t *) malloc(file_size * sizeof(xlen_t));

  // Open the file and find out how big it is so that we can allocate
  // the correct amount of space
  if (!(fp = fopen("xorSigmoidSymmetric-fixed.16bin", "rb"))) {
  return -1;
  }
  // Write the configuration
  fread(buf, sizeof(xlen_t), file_size, fp);
  fclose(fp);

  int* faddr;
  faddr = &fbytes;
  writedev(fd, (char*)faddr, sizeof(int*));


  printf("Type in a short string to send to the kernel module:\n");
  scanf("%[^\n]%*c", stringToSend);                // Read in a string (with
  spaces)
  printf("Writing message to the device [%s].\n", stringToSend);
  ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the
  LKM
  if (ret < 0){
  perror("Failed to write the message to the device.");
  return errno;
  }

  printf("Press ENTER to read back from the device...\n");
  getchar();

  printf("Reading from the device...\n");
  ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the
  LKM
  if (ret < 0){
  perror("Failed to read the message from the device.");
  return errno;
  }
  printf("The received message is: [%s]\n", receive);*/

  //uint16_t total_layers, layer_offset, ptr;
  //uint64_t test;
  /*memcpy(&total_layers, (void*)addr, sizeof(total_layers));
  printf("total_layers = %p\n", total_layers);

  memcpy(&test, (void*)addr, sizeof(test));
  printf("test = %p\n", test);

  memcpy(&total_layers, (void*)addr+2, sizeof(total_layers));
  printf("total_layers = %p\n", total_layers);
  
  memcpy(&total_layers, (void*)addr+4, sizeof(total_layers));
  printf("total_layers = %p\n", total_layers);
  */
  
  /* memcpy(&total_layers, (void*)addr+6, sizeof(uint16_t)); */
  /* printf("total_layers = %d\n", total_layers); */
  
  /* memcpy(&layer_offset, (void*)addr+6+sizeof(uint16_t), sizeof(uint16_t)); */
  /* printf("layer_offset = %d\n", layer_offset); */

  
  /* uint64_t block_64; */
  /* memcpy(&block_64, addr, sizeof(block_64)); */
  /* printf("block_64 = %p\n", block_64); */
  
  //block_64 = (block_64 >> 4) & 3;
  //printf("elements_per_block = %d\n", 1 << (block_64+2));

  /*ssize_t s2;
  s2 = write(STDOUT_FILENO, addr, fbytes);
  if (s2 != fbytes) {
      if (s2 == -1)
          handle_error("write");

      fprintf(stderr, "partial write");
      exit(EXIT_FAILURE);
  }
  */
