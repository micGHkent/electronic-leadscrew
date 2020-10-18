/*
 * nextion_ti.h
 *
 *  Created on: Oct 18, 2020
 *      Author: kent
 */

#ifndef NEXTION_TI_H_
#define NEXTION_TI_H_

void scia_init();
//void transmitSCIAChar(uint16_t a);
//void transmitSCIAMessage(const unsigned char *msg);
//void initSCIAFIFO(void);

void scib_init();
void transmitSCIBChar(uint16_t a);
void transmitSCIBMessage(const unsigned char *msg);
void initSCIBFIFO(void);

#endif /* NEXTION_TI_H_ */
