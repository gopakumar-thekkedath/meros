extern void clear_intr(void);

/* Array to hold the device specific ISRs */
void (*dev_isr[MAX_ISR])();
