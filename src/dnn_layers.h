#pragma once

void dnn_init(void*);
uint8_t dnn_get_class_result();

#define CLASSES 2

void dnn_L0_pool();
void dnn_L1_conv();
void dnn_L2_relu();
void dnn_L3_pool();
void dnn_L4_conv();
void dnn_L5_conv();
void dnn_L6_conv();
void dnn_L7_relu();
void dnn_L8_pool();
void dnn_L9_conv();
void dnn_L10_conv();
void dnn_L11_conv();
void dnn_L12_relu();
void dnn_L14_fc();
void dnn_L15_relu();
void dnn_L16_fc();