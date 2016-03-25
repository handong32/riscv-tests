#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <getopt.h>
#include <time.h>

#include <sys/types.h>
#include <unistd.h>

#include <xfiles.h>
#include <fixedfann.h>
#include <fann_train.h>

enum XD_LearningTypes{ FEED_FORWARD_TRANS = 0 };

asid_nnid_table * table = NULL;
asid_type asid = 0;
nnid_type nnid = 0;
char * file_nn = "xorSigmoidSymmetric-fixed.16bin";
char * file_train = "xorSigmoidSymmetric-fixed.train";

int binary_point_width = 3, binary_point_offset = 7;
int binary_point = -1;
struct fann_train_data * data = NULL;

int binary_config_read_binary_point(char * file_nn, int binary_point_width) {
  FILE * fp;

  fp = fopen(file_nn, "rb");
  if (fp == NULL) {
    fprintf(stderr, "[ERROR] Unable to opn %s\n\n", file_nn);
    return -1;
  }

  int8_t tmp;
  fread(&tmp, sizeof(uint8_t), 1, fp);
  tmp &= ~(~0 << binary_point_width);

  fclose(fp);
  return tmp;
}

// Read the binary configuration format and return the number of
// connections
uint64_t binary_config_num_connections(char * file_nn) {
  FILE * fp;
  int i;

  fp = fopen(file_nn, "rb");
  if (fp == NULL) {
    fprintf(stderr, "[ERROR] Unable to opn %s\n\n", file_nn);
    return 0;
  }

  uint64_t connections = 0;
  uint16_t total_layers, layer_offset, ptr;
  uint32_t tmp;
  uint16_t layer_0, layer_1;
  fseek(fp, 6, SEEK_SET);
  fread(&total_layers, sizeof(uint16_t), 1, fp);
  fread(&layer_offset, sizeof(uint16_t), 1, fp);
  fseek(fp, layer_offset, SEEK_SET);
  fread(&ptr, sizeof(uint16_t), 1, fp);
  ptr &= ~((~0)<<12);
  fseek(fp, ptr + 2, SEEK_SET);
  fread(&layer_0, sizeof(uint16_t), 1, fp);
  layer_0 &= ~((~0)<<8);
  fseek(fp, layer_offset, SEEK_SET);
  fread(&tmp, sizeof(uint32_t), 1, fp);
  layer_1 = (tmp & (~((~0)<<10))<<12)>>12;
  connections += (layer_0 + 1) * layer_1;

  for (i = 1; i < total_layers; i++) {
    layer_0 = layer_1;
    fseek(fp, layer_offset + 4 * i, SEEK_SET);
    fread(&tmp, sizeof(uint32_t), 1, fp);
    layer_1 = (tmp & (~((~0)<<10))<<12)>>12;
    connections += (layer_0 + 1) * layer_1;
  }

  fclose(fp);
  return connections;
}

int  main(int argc, char **argv)
{
    printf("%s:  START\n", argv[0]);
    printf("pid: %d\n", getpid());
    
    nnid_type mynnid = 0;
    int ltype = FEED_FORWARD_TRANS;
    element_type nto = 0;
    
    tid_type tt=0;
    
    xfiles_dana_id(!0);
    
    tt=new_write_request(mynnid, ltype, nto);
    
    printf("tt=0x%x\n", tt);

    /*printf("********* CREATE  ************\n");
    
    binary_point = binary_config_read_binary_point(file_nn, binary_point_width) +
	binary_point_offset;
    if (binary_point < binary_point_offset) {
	fprintf(stderr, "[ERROR] Binary point (%d) looks bad, exiting\n\n",
		binary_point);
	exit(0);
    }
    printf("[INFO] Found binary point %d\n", binary_point);

    asid_nnid_table_create(&table, asid * 2 + 1, nnid * 2 + 1);
    set_antp(table);

    int i;
    for (i = 0; i < nnid * 2 + 1; i++) {
	if (i == nnid) {
	    printf("attach_nn_configuration for nnid = %d\n", nnid);
	    if (attach_nn_configuration(&table, asid, file_nn) != nnid + 1) {
		printf("[ERROR] Failed to attach NN configuration 0x%x\n", nnid);
		exit(0);
	    }
	}
	else
	{
	    printf("attach_garbage for nnid = %d\n", nnid);
	    attach_garbage(&table, asid);
	}
    }
    set_asid(asid);
    
    uint64_t connections_per_epoch = binary_config_num_connections(file_nn);

    printf("connections_per_epoch = %ld\n", connections_per_epoch);
    
    // Read in data from the training file
    data = fann_read_train_from_file(file_train);
    if (data == NULL) {
	printf("fann_read_train_from_file error\n");
	exit(0);
    }
    
    size_t num_input = data->num_input;
    size_t num_output = data->num_output;
    printf("[INFO] Done reading input file\n");
    
    printf("num_input = %d num_output = %d\n", num_input, num_output);
    
    asid_nnid_table_destroy(&table);*/
    printf("************ %s:  END *************\n", argv[0]);
}
