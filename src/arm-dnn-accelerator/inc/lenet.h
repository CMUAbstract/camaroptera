void zero( mat_t *buffer );
void normalize( mat_t *input_buffer, mat_t *dest_buffer );
void relu( mat_t *src_buffer, uint16_t threshold );
void pooling( mat_t *src_buffer, mat_t *dest_buffer, uint8_t type, uint8_t kernel_size, uint8_t stride  );

void conv_dense(mat_t *weight, mat_t *bias, mat_t *src_buffer, mat_t *dest_buffer, uint16_t stride);

void conv_sparse(mat_t *weight, mat_t *bias, mat_t *src_buffer, mat_t *dest_buffer, uint16_t stride, bool depthwise, uint16_t depth_id, bool fc);
