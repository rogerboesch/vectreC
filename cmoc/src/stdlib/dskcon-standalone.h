/*  dskcon-standalone.h - CoCo 4-drive floppy disk sector read/write support.
    By Pierre Sarrazin <http://sarrazip.com/>

    This file is in the public domain.
*/

#ifndef _H_dskcon
#define _H_dskcon

#include <cmoc.h>


extern unsigned char DCOPC;  /* DSKCON OPERATION CODE 0-3 */
extern unsigned char DCDRV;  /* DSKCON DRIVE NUMBER 0—3 */
extern unsigned char DCTRK;  /* DSKCON TRACK NUMBER 0—34 */
extern unsigned char DCSEC;  /* DSKCON SECTOR NUMBER 1-18 */
extern unsigned char *DCBPT; /* DSKCON DATA POINTER */
extern unsigned char DCSTA;  /* DSKCON STATUS BYTE */

extern unsigned char RDYTMR;    /* MOTOR TURN OFF TIMER */
extern unsigned char DRGRAM;    /* RAM IMAGE OF DSKREG ($FF40) */
extern unsigned char DR0TRK[4];    /* CURRENT TRACK NUMBER, DRIVES 0,1,2,3 */
extern unsigned char NMIFLG;    /* NMI FLAG: 0=DON'T VECTOR <>0=YECTOR OUT */
extern void *DNMIVC;   /* NMI VECTOR: WHERE TO JUMP FOLLOWING AN NMI */

extern unsigned char dskcon_driveEnableMasks[4];


enum
{
    DSKREG = 0xFF40,   // DISK CONTROL REGISTER
    FDCREG = 0xFF48,   // 1793 CONTROL REGISTER
};


// Type used by dskcon_init().
//
typedef interrupt void (*dskcon_NmiServiceFunctionPointer)();


// Function to be called first.
// Initializes the floppy disk sector read/write support.
// Must be called while interrupts are masked.
//
// The IRQ service routine must be coded so that it invokes
// dskcon_irqService().  Call dskcon_processSector() to read/write a sector.
//
// newNMIService: Address of the nom-maskable interrupt service routine.
//                Typically, this is the address of dskcon_nmiService().
//
// Returns a 24-bit value that must be passed to dskcon_shutdown()
// to restore the original NMI service routine.
//
unsigned long dskcon_init(dskcon_NmiServiceFunctionPointer newNMIService);


// initReturnValue: Must be the value obtained from dskcon_init.
// Must be called while interrupts are masked.
//
void dskcon_shutdown(unsigned long initReturnValue);


// Function to be used as the NMI service routine.
//
interrupt void dskcon_nmiService();


// Equivalent of DSKCON.
// Fill DCOPC, etc., then call this.
// Upon exit, DCSTA contains these bits:
//
//  Bit  Error Type
//  ===  ==========
//   7   Drive Not Ready
//   6   Write Protect
//   5   Write Fault
//   4   Seek Error or Record Not Found
//   3   CRC Error
//   2   Lost Data
//
// Call dskcon_init() before calling this.
//
// Example:
//     DCOPC = 2;      // 2 = read, 3 = write
//     DCDRV = 0;      // 0..3
//     DCTRK = 17;     // >= 0
//     DCSEC = 3;      // >= 1
//     DCBPT = 0x500;  // address of 256-byte sector buffer
//     dskcon_processSector();
//     if (DCSTA != 0)
//         processError();
//
// The name was chosen to avoid clashing with the dskcon() defined
// by CMOC's standard library function dskcon().
//
void dskcon_processSector();


// Function to be called by the CoCo's IRQ service routine.
// This is why this function does not itself have the 'interrupt' keyword.
// Example:
// interrupt void irqService()
// {
//     if ((* (char *) 0xFF03) & 0x80)  // do nothing if 63.5 us interrupt
//     {
//         * (char *) 0xFF02;  // 60 Hz interrupt. Reset PIA0, port B interrupt flag.
//         dskcon_irqService();
//     }
// }
//
void dskcon_irqService();


#endif  /* _H_dskcon */
