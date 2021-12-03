#ifndef LIBMSP_PERIPH_H
#define LIBMSP_PERIPH_H

#include <msp430.h>

#define CONCAT_INNER(a, b) a ## b
#define CONCAT(a, b) CONCAT_INNER(a, b)

#define CONCAT3_INNER(a, b, c) a ## b ## c
#define CONCAT3(a, b, c) CONCAT3_INNER(a, b, c)

#define CONCAT4_INNER(a, b, c, d) a ## b ## c ## d
#define CONCAT4(a, b, c, d) CONCAT4_INNER(a, b, c, d)

#define STR_INNER(x) #x
#define STR(x) STR_INNER(x)

#define BIT_INNER(idx) BIT ## idx
#define BIT(idx) BIT_INNER(idx)

#define GPIO_INNER(port, reg) P ## port ## reg
#define GPIO(port, reg) GPIO_INNER(port, reg)

#define INTFLAG_INNER(port, pin) P ## port ## IV_ ## P ## port ## IFG ## pin
#define INTFLAG(port, pin) INTFLAG_INNER(port, pin)

#define INTVEC_INNER(port) P ## port ## IV
#define INTVEC(port) INTVEC_INNER(port)

#define INTVEC_RANGE_INNER(port) P ## port ## IV_P ## port ## IFG7
#define INTVEC_RANGE(port) INTVEC_RANGE_INNER(port)

#define TIMER_INNER(name, reg) T ## name ## reg
#define TIMER(name, reg) TIMER_INNER(name, reg)

#define TIMER_CC_INNER(name, ccridx, reg) T ## name ## reg ## ccridx
#define TIMER_CC(name, ccridx, reg) TIMER_CC_INNER(name, ccridx, reg)

// Legacy
#define TIMERA_INTFLAG_INNER(id, ccridx) T ## id ## IV_ ## TACCR ## ccridx
#define TIMERA_INTFLAG(id, ccridx) TIMERA_INTFLAG_INNER(id, ccridx)

#define TIMER_INTFLAG_INNER(type, idx, ccridx) T ## type ## idx ## IV_ ## T ## type ## CCR ## ccridx
#define TIMER_INTFLAG(type, idx, ccridx) TIMER_INTFLAG_INNER(type, idx, ccridx)

#define TIMER_1_INTFLAG_INNER(type, idx, flag) T ## type ## idx ## IV_T ## type ## flag
#define TIMER_1_INTFLAG(type, idx, flag) TIMER_1_INTFLAG_INNER(type, idx, flag)

// There is only a IV (flag vector) register for vector 1, not 0 for either timer type
#define TIMER_INTVEC_INNER(type, idx) T ## type ## idx ## IV
#define TIMER_INTVEC(type, idx) TIMER_INTVEC_INNER(type, idx)

// Same as above; defined for consistency in user code
#define TIMER_1_INTVEC_INNER(type, idx) T ## type ## idx ## IV
#define TIMER_1_INTVEC(type, idx) TIMER_1_INTVEC_INNER(type, idx)

#define UART_INNER(idx, reg) UCA ## idx ## reg
#define UART(idx, reg) UART_INNER(idx, reg)

#if defined(__CC430__)

#define UART_MCTL(idx) UART_INNER(idx, MCTL)
#define UART_BRS_INNER(brs) UCBRS_ ## brs
#define UART_BRS(brs) UART_BRS_INNER(brs)
#define UART_INTFLAG(flag) CONCAT(USCI_UC, flag)
#define UART_STAT STATW

#define UART_SET_BR(idx, br) do { \
    UART(idx, BR0) = br & 0xff; \
    UART(idx, BR1) = br >> 8; \
} while (0)

#elif defined(__MSP430F__)

#define UART_MCTL(idx) UART_INNER(idx, MCTL)
#define UART_BRS_INNER(brs) UCBRS_ ## brs
#define UART_BRS(brs) UART_BRS_INNER(brs)
#define UART_INTFLAG(flag) CONCAT(UC, flag)
#define UART_STAT STAT

#define UART_SET_BR(idx, br) do { \
    UART(idx, BR0) = br & 0xff; \
    UART(idx, BR1) = br >> 8; \
} while (0)

#elif defined(__MSP430FR__)

#define UART_MCTL(idx) UART_INNER(idx, MCTLW)
#define UART_BRS(brs) (brs << 8)
#define UART_INTFLAG(flag) CONCAT(USCI_UART_UC, flag)
#define UART_STAT STATW

#define UART_SET_BR(idx, br) do { \
    UART(idx, BRW) = br; \
} while (0)

#else // FAMILY
#error Unsupported MCU family
#endif // FAMILY

#if defined(__CC430F5137__) || defined(__MSP430F5340__)

#define UART_SET_SEL(port, pin_bits) do { \
    GPIO(port, SEL) |= (pin_bits); \
} while (0)

#elif defined(__MSP430FR5969__) || defined(__MSP430FR5949__) || \
      defined(__MSP430FR5994__)

#define UART_SET_SEL(port, pin_bits) do { \
    GPIO(port, SEL0) &= ~(pin_bits); \
    GPIO(port, SEL1) |= (pin_bits); \
} while (0)

#elif defined(__MSP430FR6989__)

#define UART_SET_SEL(port, pin_bits) do { \
    GPIO(port, SEL0) |= (pin_bits); \
    GPIO(port, SEL1) &= ~(pin_bits); \
} while (0)

#else // DEVICE
#error MCU not supported
#endif // DEVICE


#define UART_VECTOR_INNER(idx) USCI_A ## idx ## _VECTOR
#define UART_VECTOR(idx) UART_VECTOR_INNER(idx)

#define UART_ISR_INNER(idx) USCI_A ## idx ## _ISR
#define UART_ISR(idx) UART_ISR_INNER(idx)

#define BRS_BITS_INNER(brs) UCBRS_ ## brs
#define BRS_BITS(brs) BRS_BITS_INNER(brs)

#define BRF_BITS_INNER(brf) UCBRF_ ## brf
#define BRF_BITS(brf) BRF_BITS_INNER(brf)

#define DMA_INNER(name, reg) DMA ## name ## reg
#define DMA(name, reg) DMA_INNER(name, reg)

#define DMA_CTL_INNER(chan) DMACTL ## chan
#define DMA_CTL(chan) DMA_CTL_INNER(chan)

#define DMA_TRIG_INNER(chan, trig) DMA ## chan ## TSEL__ ## trig
#define DMA_TRIG(chan, trig) DMA_TRIG_INNER(chan, trig)

#define DMA_TRIG_UART_INNER(idx, dir) USCIA ## idx ## dir
#define DMA_TRIG_UART(idx, dir) DMA_TRIG_UART_INNER(idx, dir)

#define DMA_INTFLAG_INNER(name) DMAIV_DMA ## name ## IFG
#define DMA_INTFLAG(name) DMA_INTFLAG_INNER(name)

#define TIMER_DIV_BITS_INNER(div) ID__ ## div
#define TIMER_DIV_BITS(div) TIMER_DIV_BITS_INNER(div)

#define TIMER_DIVEX_BITS_INNER(type, div) T ## type ## IDEX__ ## div
#define TIMER_DIVEX_BITS(type, div) TIMER_DIVEX_BITS_INNER(type, div)

#define TIMER_CLK_SOURCE_BITS_INNER(mod, src) T ## mod ## SSEL__ ## src
#define TIMER_CLK_SOURCE_BITS(mod, src) TIMER_CLK_SOURCE_BITS_INNER(mod, src)

// Legacy: superceded by TIMER()
#define TIMER_CLR_INNER(mod)  T ## mod ## CLR
#define TIMER_CLR(mod) TIMER_CLR_INNER(mod)

#define GPIO_ISR_INNER(port) PORT ## port ## _ISR
#define GPIO_ISR(port) GPIO_ISR_INNER(port)

#define GPIO_VECTOR_INNER(port) PORT ## port ## _VECTOR
#define GPIO_VECTOR(port) GPIO_VECTOR_INNER(port)

// On MSP430, CCR0 gets a dedicated vector, the rest share one vector
#define TIMER_CCR_VECTOR_0 0
#define TIMER_CCR_VECTOR_1 1
#define TIMER_CCR_VECTOR_2 1
#define TIMER_CCR_VECTOR_3 1
#define TIMER_CCR_VECTOR_4 1
#define TIMER_CCR_VECTOR_5 1
#define TIMER_CCR_VECTOR_6 1
#define TIMER_CCR_VECTOR_7 1

#define TIMER_CCR_VECTOR_INNER_INNER(ccr) TIMER_CCR_VECTOR_ ## ccr
#define TIMER_CCR_VECTOR_INNER(ccr) TIMER_CCR_VECTOR_INNER_INNER(ccr)
#define TIMER_CCR_VECTOR(ccr) TIMER_CCR_VECTOR_INNER(ccr)

#define TIMER_VECTOR_INNER_INNER(type, idx, vect) TIMER ## idx ## _ ## type ## vect ## _VECTOR
#define TIMER_VECTOR_INNER(type, idx, vect) TIMER_VECTOR_INNER_INNER(type, idx, vect)
#define TIMER_VECTOR(type, idx, ccr) TIMER_VECTOR_INNER(type, idx, TIMER_CCR_VECTOR(ccr))

#define TIMER_1_VECTOR_INNER_INNER(type, idx) TIMER ## idx ## _ ## type ## 1_VECTOR
#define TIMER_1_VECTOR_INNER(type, idx) TIMER_1_VECTOR_INNER_INNER(type, idx)
#define TIMER_1_VECTOR(type, idx) TIMER_1_VECTOR_INNER(type, idx)

#define TIMER_ISR_INNER_INNER(type, idx, vect) TIMER ## idx ## _ ## type ## vect ## _ISR
#define TIMER_ISR_INNER(type, idx, vect) TIMER_ISR_INNER_INNER(type, idx, vect)
#define TIMER_ISR(type, idx, ccr) TIMER_ISR_INNER(type, idx, TIMER_CCR_VECTOR(ccr))

#define TIMER_1_ISR_INNER(type, idx) TIMER ## idx ## _ ## type ## 1_ISR
#define TIMER_1_ISR(type, idx) TIMER_1_ISR_INNER(type, idx)

#define DCO_FREQ_RANGE_BITS_INNER(r) DCORSEL_ ## r;
#define DCO_FREQ_RANGE_BITS(r) DCO_FREQ_RANGE_BITS_INNER(r)

#define FLL_D_BITS_INNER(d) FLLD_ ## d
#define FLL_D_BITS(d) FLL_D_BITS_INNER(d)

#define FLL_REF_DIV_BITS_INNER(div) FLLREFDIV__ ## div
#define FLL_REF_DIV_BITS(div) FLL_REF_DIV_BITS_INNER(div)

#define CLK_DIV_BITS_INNER(clk, div) DIV ## clk ## __ ## div
#define CLK_DIV_BITS(clk, div) CLK_DIV_BITS_INNER(clk, div)

#define COMP_INNER(type, arg) C ## type ## arg
#define COMP(type, arg) COMP_INNER(type, arg)

#define COMP2_INNER(type, arg1, arg2) C ## type ## arg1 ## arg2
#define COMP2(type, arg1, arg2) COMP2_INNER(type, arg1, arg2)

#define COMP_VECTOR_INNER(type) COMP_ ## type ## _VECTOR
#define COMP_VECTOR(type) COMP_VECTOR_INNER(type)

#define COMP_INTFLAG_INNER(type, flag) C ## type ## IV_ ## flag
#define COMP_INTFLAG(type, flag) COMP_INTFLAG_INNER(type, flag)

#define COMP_INTFLAG2_INNER(type, flag) C ## type ## IV_ ## C ## type ## flag
#define COMP_INTFLAG2(type, flag) COMP_INTFLAG2_INNER(type, flag)

// Compat to handle inconsistency in TI's header
#define CBIV_IFG  CBIV_CBIFG
#define CBIV_IIFG CBIV_CBIIFG

// Legacy. Superceded by COMP() and COMP2().
#define COMP_REF_INNER(level_idx) CEREF0_ ## level_idx
#define COMP_REF(level_idx) COMP_REF_INNER(level_idx)

#define COMP_CHAN_SEL_INNER(idx) CEIPSEL_ ## idx
#define COMP_CHAN_SEL(idx) COMP_CHAN_SEL_INNER(idx)

#define COMP_PIN_INPUT_BUF_DISABLE_INNER(chan_idx) CEPD ## chan_idx
#define COMP_PIN_INPUT_BUF_DISABLE(chan_idx) COMP_PIN_INPUT_BUF_DISABLE_INNER(chan_idx)

#define WATCHDOG_BITS_INNER(clk, interval) (WDTSSEL__ ## clk | WDTIS__ ## interval)
#define WATCHDOG_BITS(clk, interval) WATCHDOG_BITS_INNER(clk, interval)

#define REF_INNER(lvl) REFVSEL_ ## lvl
#define REF(lvl) REF_INNER(lvl)

#define ISR_NAME(vect) CONCAT(ISR_, vect)

// The macro for defining an ISR is simple:
//    ISR(vector) { ... }
// where 'vector' is usually generated by one of the *_VECT() macros from above.
//
// Despite both GCC and Clang using the same linker script, for Clang, we need
// to manually create a symbol and allocate it in the per-vector section (named
// with a name that matches the names that the linker script expects). Note that
// Clang (at least v3.8 and earlier) creates __isr_* symbols automatically, but
// these are only interpreted by the deprecated MSPGCC compiler/linker, not by
// TI GCC for MSP. See more on this below.
//
// Implementation internals:
//
// * The VNUM macro: The 'vect' argument given to the
// ISR macro by the caller will be a *_VECTOR from TI GCC headers (not under
// our control). The *_VECTOR macros are defined not as plain numbers, like
// '53' but as numbers in parenthesis, like '(53)'. Ask TI if you dare wonder
// why. So, to be able to  use the number as a token, we need to get rid of the
// nasty parenthesis, which by passing 'vect' to the VNUM macro without adding
// parenthesis: 'VNUM vect', which expands to 'VNUM (53)' which expands to 53.

#define VNUM(x) x

// * The *2 factor in interrupt attribute: Clang (v3.8 and earlier) creates
// __isr_* symbols for MSPGCC (see lib/CodeGen/TargetInfo.cpp
// MSP430TargetCodeGenInfo::SetTargetAttributes), these are not relevant for TI
// GCC, but they are not entirely harmless. They cause a problem because Clang
// generates __isr_N where N is divided by 2 (because MSPGCC wants it that
// way, I guess). So, if we pass a vector number, the odd and even vector numbers
// will generate the same symbol and therefore linking will fail with a
// multiply-defined symbol error. As a workaround we pass the doubled value.
//
// There is one more problem that so far requires a patch to Clang: it rejects
// vector numbers that are odd or above 30. So, this problem with the above
// problem form a catch-22 situation: if we pass N/2 then the atttribute is
// accepted, but we get multiply-defined values; if we pass N, it's rejected;
// and if we pass N*2, it's rejected. The patch is in lib/Sema/SemaDeclAttr.cpp
// to change the 'if (Num & 1) || Num > 30)' to 'if (Num == 0)' (zero is not a
// valid vector).

#ifdef __clang__

#define ISR(vect) \
    void ISR_NAME(VNUM vect)(void); \
    __attribute__((section("__interrupt_vector_" STR(VNUM vect)), aligned(2))) \
    void(*CONCAT(__vector_, VNUM vect))(void) = ISR_NAME(VNUM vect); \
    __attribute__ ((interrupt(vect * 2))) void ISR_NAME(VNUM vect)(void)

#else // !__clang__

#define ISR(vect) \
    __attribute__ ((interrupt(vect))) void ISR_NAME(VNUM vect)(void)

#endif // !__clang__

// Fill-in value-keyd macros that are missing from TI's headers in older TI MSPGCC
#if GCC_VERSION_MAJOR < 6 /* tested: v4.9 (MSPGCC v3) does not define these,
                              v6.4 (MSPGCC v5) does define these */
#define TAIDEX__1 TAIDEX_0
#define TAIDEX__2 TAIDEX_1
#define TAIDEX__3 TAIDEX_2
#define TAIDEX__4 TAIDEX_3
#define TAIDEX__5 TAIDEX_4
#define TAIDEX__6 TAIDEX_5
#define TAIDEX__7 TAIDEX_6
#define TAIDEX__8 TAIDEX_7
#endif // GCC_VERSION_MAJOR < 6

// These are missing from latest TI MSPGCC (v5) and older ones (v3)
#if defined(__MSP430F5340__)
// UCAxIFG flags
#define UCTXCPTIFG  0x0008
#endif // __MSP430F5340__

#if defined(__MSP430F5340__) || \
    defined(__CC430F5137__) || \
    defined(__MSP430FR6989) || \
    defined(__MSP430FR5949) || \
    defined(__MSP430FR5969) || \
    defined(__MSP430FR5994)
// Ugly workaround to make the pretty GPIO macro work for OUT register
// (a control bit for TAxCCTLx uses the name 'OUT'): rename to OUT to TOUT.
enum {
	TOUT = OUT,
};
#undef OUT
#endif

#endif // LIBMSP_PERIPH_H
