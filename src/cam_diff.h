#pragma once

uint8_t cam_diff(uint8_t *newf, uint8_t *oldf, size_t size, uint8_t thresh);
void camaroptera_diff(uint8_t *newf, uint8_t *oldf, size_t size, uint8_t thresh);

extern uint8_t camaroptera_state;
