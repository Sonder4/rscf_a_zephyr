#ifndef RSCF_EXT_UART_MUX_H_
#define RSCF_EXT_UART_MUX_H_

#include <stdint.h>

int RSCFExtUartMuxInit(void);
int RSCFExtUartMuxSelect(uint8_t channel);
uint8_t RSCFExtUartMuxCurrentChannel(void);
const struct device *RSCFExtUartMuxUart(void);

#endif /* RSCF_EXT_UART_MUX_H_ */
