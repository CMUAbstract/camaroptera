#pragma once

void dnn_init(uint8_t *);
uint8_t dnn_get_class_result();

#define CLASSES 2

void dnn_L0_pool();
void dnn_L1_conv();
void dnn_L2_conv();
void dnn_L3_conv();
void dnn_L4_relu();
void dnn_L5_pool();
void dnn_L6_conv();
void dnn_L7_conv();
void dnn_L8_conv();
void dnn_L9_relu();
void dnn_L10_pool();
void dnn_L12_fc();
void dnn_L13_relu();
void dnn_L14_fc();